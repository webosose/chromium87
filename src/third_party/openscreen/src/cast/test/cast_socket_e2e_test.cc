// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <openssl/evp.h>
#include <openssl/mem.h>

#include <atomic>
#include <chrono>

#include "cast/common/certificate/cast_trust_store.h"
#include "cast/common/certificate/testing/test_helpers.h"
#include "cast/common/channel/connection_namespace_handler.h"
#include "cast/common/channel/message_util.h"
#include "cast/common/channel/virtual_connection_manager.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/cast_socket.h"
#include "cast/receiver/channel/device_auth_namespace_handler.h"
#include "cast/receiver/channel/static_credentials.h"
#include "cast/receiver/public/receiver_socket_factory.h"
#include "cast/sender/public/sender_socket_factory.h"
#include "gtest/gtest.h"
#include "platform/api/serial_delete_ptr.h"
#include "platform/api/tls_connection_factory.h"
#include "platform/base/tls_connect_options.h"
#include "platform/base/tls_credentials.h"
#include "platform/base/tls_listen_options.h"
#include "platform/impl/logging.h"
#include "platform/impl/network_interface.h"
#include "platform/impl/platform_client_posix.h"
#include "util/crypto/certificate_utils.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

class SenderSocketsClient final
    : public SenderSocketFactory::Client,
      public VirtualConnectionRouter::SocketErrorHandler {
 public:
  explicit SenderSocketsClient(VirtualConnectionRouter* router)  // NOLINT
      : router_(router) {}
  ~SenderSocketsClient() = default;

  CastSocket* socket() const { return socket_; }

  // SenderSocketFactory::Client overrides.
  void OnConnected(SenderSocketFactory* factory,
                   const IPEndpoint& endpoint,
                   std::unique_ptr<CastSocket> socket) {
    OSP_CHECK(!socket_);
    OSP_LOG_INFO << "\tSender connected to endpoint: " << endpoint;
    socket_ = socket.get();
    router_->TakeSocket(this, std::move(socket));
  }

  void OnError(SenderSocketFactory* factory,
               const IPEndpoint& endpoint,
               Error error) override {
    OSP_NOTREACHED() << error;
  }

  // VirtualConnectionRouter::SocketErrorHandler overrides.
  void OnClose(CastSocket* socket) override {}
  void OnError(CastSocket* socket, Error error) override {
    OSP_NOTREACHED() << error;
  }

 private:
  VirtualConnectionRouter* const router_;
  std::atomic<CastSocket*> socket_{nullptr};
};

class ReceiverSocketsClient final
    : public ReceiverSocketFactory::Client,
      public VirtualConnectionRouter::SocketErrorHandler {
 public:
  explicit ReceiverSocketsClient(VirtualConnectionRouter* router)
      : router_(router) {}
  ~ReceiverSocketsClient() = default;

  const IPEndpoint& endpoint() const { return endpoint_; }
  CastSocket* socket() const { return socket_; }

  // ReceiverSocketFactory::Client overrides.
  void OnConnected(ReceiverSocketFactory* factory,
                   const IPEndpoint& endpoint,
                   std::unique_ptr<CastSocket> socket) override {
    OSP_CHECK(!socket_);
    OSP_LOG_INFO << "\tReceiver got connection from endpoint: " << endpoint;
    endpoint_ = endpoint;
    socket_ = socket.get();
    router_->TakeSocket(this, std::move(socket));
  }

  void OnError(ReceiverSocketFactory* factory, Error error) override {
    OSP_NOTREACHED() << error;
  }

  // VirtualConnectionRouter::SocketErrorHandler overrides.
  void OnClose(CastSocket* socket) override {}
  void OnError(CastSocket* socket, Error error) override {
    OSP_NOTREACHED() << error;
  }

 private:
  VirtualConnectionRouter* router_;
  IPEndpoint endpoint_;
  std::atomic<CastSocket*> socket_{nullptr};
};

class CastSocketE2ETest : public ::testing::Test {
 public:
  void SetUp() override {
    PlatformClientPosix::Create(std::chrono::milliseconds(10),
                                std::chrono::milliseconds(0));
    task_runner_ = PlatformClientPosix::GetInstance()->GetTaskRunner();

    sender_router_ = MakeSerialDelete<VirtualConnectionRouter>(
        task_runner_, &sender_vc_manager_);
    sender_client_ =
        std::make_unique<SenderSocketsClient>(sender_router_.get());
    sender_factory_ = MakeSerialDelete<SenderSocketFactory>(
        task_runner_, sender_client_.get(), task_runner_);
    sender_tls_factory_ = SerialDeletePtr<TlsConnectionFactory>(
        task_runner_,
        TlsConnectionFactory::CreateFactory(sender_factory_.get(), task_runner_)
            .release());
    sender_factory_->set_factory(sender_tls_factory_.get());

    ErrorOr<GeneratedCredentials> creds = GenerateCredentials("Device ID");
    ASSERT_TRUE(creds.is_value());
    credentials_ = std::move(creds.value());

    CastTrustStore::CreateInstanceForTest(credentials_.root_cert_der);
    auth_handler_ = MakeSerialDelete<DeviceAuthNamespaceHandler>(
        task_runner_, credentials_.provider.get());
    receiver_router_ = MakeSerialDelete<VirtualConnectionRouter>(
        task_runner_, &receiver_vc_manager_);
    receiver_router_->AddHandlerForLocalId(kPlatformReceiverId,
                                           auth_handler_.get());
    receiver_client_ =
        std::make_unique<ReceiverSocketsClient>(receiver_router_.get());
    receiver_factory_ = MakeSerialDelete<ReceiverSocketFactory>(
        task_runner_, receiver_client_.get(), receiver_router_.get());

    receiver_tls_factory_ = SerialDeletePtr<TlsConnectionFactory>(
        task_runner_, TlsConnectionFactory::CreateFactory(
                          receiver_factory_.get(), task_runner_)
                          .release());
  }

  void TearDown() override {
    OSP_LOG_INFO << "Shutting down";
    sender_router_.reset();
    receiver_router_.reset();
    receiver_tls_factory_.reset();
    receiver_factory_.reset();
    auth_handler_.reset();
    sender_tls_factory_.reset();
    sender_factory_.reset();
    CastTrustStore::ResetInstance();
    PlatformClientPosix::ShutDown();
  }

 protected:
  IPAddress GetLoopbackV4Address() {
    absl::optional<InterfaceInfo> loopback = GetLoopbackInterfaceForTesting();
    OSP_CHECK(loopback);
    IPAddress address = loopback->GetIpAddressV4();
    OSP_CHECK(address);
    return address;
  }

  IPAddress GetLoopbackV6Address() {
    absl::optional<InterfaceInfo> loopback = GetLoopbackInterfaceForTesting();
    OSP_CHECK(loopback);
    IPAddress address = loopback->GetIpAddressV6();
    return address;
  }

  void WaitForCastSocket() {
    int attempts = 1;
    constexpr int kMaxAttempts = 8;
    constexpr std::chrono::milliseconds kSocketWaitDelay(250);
    do {
      OSP_LOG_INFO << "\tChecking for CastSocket, attempt " << attempts << "/"
                   << kMaxAttempts;
      if (sender_client_->socket()) {
        break;
      }
      std::this_thread::sleep_for(kSocketWaitDelay);
    } while (attempts++ < kMaxAttempts);
    ASSERT_TRUE(sender_client_->socket());
  }

  void Connect(const IPAddress& address) {
    uint16_t port = 65321;
    OSP_LOG_INFO << "\tStarting socket factories";
    task_runner_->PostTask([this, &address, port]() {
      OSP_LOG_INFO << "\tReceiver TLS factory Listen()";
      receiver_tls_factory_->SetListenCredentials(credentials_.tls_credentials);
      receiver_tls_factory_->Listen(IPEndpoint{address, port},
                                    TlsListenOptions{1u});
    });

    task_runner_->PostTask([this, &address, port]() {
      OSP_LOG_INFO << "\tSender CastSocket factory Connect()";
      sender_factory_->Connect(IPEndpoint{address, port},
                               SenderSocketFactory::DeviceMediaPolicy::kNone,
                               sender_router_.get());
    });

    WaitForCastSocket();
  }

  TaskRunner* task_runner_;

  // NOTE: Sender components.
  VirtualConnectionManager sender_vc_manager_;
  SerialDeletePtr<VirtualConnectionRouter> sender_router_;
  std::unique_ptr<SenderSocketsClient> sender_client_;
  SerialDeletePtr<SenderSocketFactory> sender_factory_;
  SerialDeletePtr<TlsConnectionFactory> sender_tls_factory_;

  // NOTE: Receiver components.
  VirtualConnectionManager receiver_vc_manager_;
  SerialDeletePtr<VirtualConnectionRouter> receiver_router_;
  GeneratedCredentials credentials_;
  SerialDeletePtr<DeviceAuthNamespaceHandler> auth_handler_;
  std::unique_ptr<ReceiverSocketsClient> receiver_client_;
  SerialDeletePtr<ReceiverSocketFactory> receiver_factory_;
  SerialDeletePtr<TlsConnectionFactory> receiver_tls_factory_;
};

// These test the most basic setup of a complete CastSocket.  This means
// constructing both a SenderSocketFactory and ReceiverSocketFactory, making a
// TLS connection to a known port over the loopback device, and checking device
// authentication.
TEST_F(CastSocketE2ETest, ConnectV4) {
  OSP_LOG_INFO << "Getting loopback IPv4 address";
  IPAddress loopback_address = GetLoopbackV4Address();
  OSP_LOG_INFO << "Connecting CastSockets";
  Connect(loopback_address);
}

TEST_F(CastSocketE2ETest, ConnectV6) {
  OSP_LOG_INFO << "Getting loopback IPv6 address";
  IPAddress loopback_address = GetLoopbackV6Address();
  if (loopback_address) {
    OSP_LOG_INFO << "Connecting CastSockets";
    Connect(loopback_address);
  } else {
    OSP_LOG_WARN << "Test skipped due to missing IPv6 loopback address";
  }
}

}  // namespace cast
}  // namespace openscreen
