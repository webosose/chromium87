// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/mock_render_thread.h"

#include <memory>

#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/unguessable_token.h"
#include "build/build_config.h"
#include "content/common/associated_interfaces.mojom.h"
#include "content/common/frame_messages.h"
#include "content/common/render_message_filter.mojom.h"
#include "content/common/view_messages.h"
#include "content/public/renderer/render_thread_observer.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/render_view_impl.h"
#include "ipc/ipc_message_utils.h"
#include "ipc/ipc_sync_message.h"
#include "ipc/message_filter.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/dom_storage/session_storage_namespace_id.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "third_party/blink/public/web/web_script_controller.h"

namespace content {

namespace {

// Some tests hard-code small numbers for routing IDs, so make reasonably sure
// that the IDs generated by MockRenderThread will not clash.
constexpr int32_t kFirstGeneratedRoutingId = 313337000;

static const blink::UserAgentMetadata kUserAgentMetadata;

class MockRenderMessageFilterImpl : public mojom::RenderMessageFilter {
 public:
  MockRenderMessageFilterImpl() = default;
  ~MockRenderMessageFilterImpl() override = default;

  // mojom::RenderMessageFilter:
  void GenerateRoutingID(GenerateRoutingIDCallback callback) override {
    NOTREACHED();
    std::move(callback).Run(MSG_ROUTING_NONE);
  }

  void HasGpuProcess(HasGpuProcessCallback callback) override {
    std::move(callback).Run(false);
  }

#if defined(OS_LINUX) || defined(OS_CHROMEOS)
  void SetThreadPriority(int32_t platform_thread_id,
                         base::ThreadPriority thread_priority) override {}
#endif
};

// Some tests require that a valid mojo::RouteProvider* be accessed to send
// messages over. The RouteProvider does not need to be bound to any real
// implementation, so we simply bind it to a pipe that we'll forget about, as to
// drain all messages sent over the remote.
mojom::RouteProvider* GetStaticRemoteRouteProvider() {
  static mojo::Remote<mojom::RouteProvider> remote;
  if (!remote) {
    ignore_result(remote.BindNewPipeAndPassReceiver());
  }
  return remote.get();
}

}  // namespace

MockRenderThread::MockRenderThread()
    : next_routing_id_(kFirstGeneratedRoutingId),
      mock_render_message_filter_(new MockRenderMessageFilterImpl()) {
  RenderThreadImpl::SetRenderMessageFilterForTesting(
      mock_render_message_filter_.get());
}

MockRenderThread::~MockRenderThread() {
  while (!filters_.empty()) {
    scoped_refptr<IPC::MessageFilter> filter = filters_.back();
    filters_.pop_back();
    filter->OnFilterRemoved();
  }
}

// Called by the Widget. Used to send messages to the browser.
// We short-circuit the mechanism and handle the messages right here on this
// class.
bool MockRenderThread::Send(IPC::Message* msg) {
  // We need to simulate a synchronous channel, thus we are going to receive
  // through this function messages, messages with reply and reply messages.
  // We can only handle one synchronous message at a time.
  if (msg->is_reply()) {
    if (reply_deserializer_) {
      reply_deserializer_->SerializeOutputParameters(*msg);
      reply_deserializer_.reset();
    }
  } else {
    if (msg->is_sync()) {
      // We actually need to handle deleting the reply deserializer for sync
      // messages.
      reply_deserializer_.reset(
          static_cast<IPC::SyncMessage*>(msg)->GetReplyDeserializer());
    }
    if (msg->routing_id() == MSG_ROUTING_CONTROL)
      OnControlMessageReceived(*msg);
    else
      OnMessageReceived(*msg);
  }
  delete msg;
  return true;
}

IPC::SyncChannel* MockRenderThread::GetChannel() {
  return nullptr;
}

std::string MockRenderThread::GetLocale() {
  return "en-US";
}

IPC::SyncMessageFilter* MockRenderThread::GetSyncMessageFilter() {
  return nullptr;
}

scoped_refptr<base::SingleThreadTaskRunner>
MockRenderThread::GetIOTaskRunner() {
  return scoped_refptr<base::SingleThreadTaskRunner>();
}

void MockRenderThread::BindHostReceiver(mojo::GenericPendingReceiver receiver) {
}

void MockRenderThread::AddRoute(int32_t routing_id, IPC::Listener* listener) {}

void MockRenderThread::RemoveRoute(int32_t routing_id) {}

int MockRenderThread::GenerateRoutingID() {
  NOTREACHED();
  return MSG_ROUTING_NONE;
}

void MockRenderThread::AddFilter(IPC::MessageFilter* filter) {
  filter->OnFilterAdded(&sink());
  // Add this filter to a vector so the MockRenderThread::RemoveFilter function
  // can check if this filter is added.
  filters_.push_back(base::WrapRefCounted(filter));
}

void MockRenderThread::RemoveFilter(IPC::MessageFilter* filter) {
  // Emulate the IPC::ChannelProxy::OnRemoveFilter function.
  for (size_t i = 0; i < filters_.size(); ++i) {
    if (filters_[i].get() == filter) {
      filter->OnFilterRemoved();
      filters_.erase(filters_.begin() + i);
      return;
    }
  }
  NOTREACHED() << "filter to be removed not found";
}

void MockRenderThread::AddObserver(RenderThreadObserver* observer) {
  observers_.AddObserver(observer);
}

void MockRenderThread::RemoveObserver(RenderThreadObserver* observer) {
  observers_.RemoveObserver(observer);
}

mojom::RouteProvider* MockRenderThread::GetRemoteRouteProvider(
    util::PassKey<AgentSchedulingGroup>) {
  return GetStaticRemoteRouteProvider();
}

void MockRenderThread::SetResourceDispatcherDelegate(
    ResourceDispatcherDelegate* delegate) {
}

void MockRenderThread::RecordAction(const base::UserMetricsAction& action) {
}

void MockRenderThread::RecordComputedAction(const std::string& action) {
}

void MockRenderThread::RegisterExtension(
    std::unique_ptr<v8::Extension> extension) {
  blink::WebScriptController::RegisterExtension(std::move(extension));
}

int MockRenderThread::PostTaskToAllWebWorkers(base::RepeatingClosure closure) {
  return 0;
}

bool MockRenderThread::ResolveProxy(const GURL& url, std::string* proxy_list) {
  return false;
}

base::WaitableEvent* MockRenderThread::GetShutdownEvent() {
  return nullptr;
}

int32_t MockRenderThread::GetClientId() {
  return 1;
}

bool MockRenderThread::IsOnline() {
  return true;
}

void MockRenderThread::SetRendererProcessType(
    blink::scheduler::WebRendererProcessType type) {}

blink::WebString MockRenderThread::GetUserAgent() {
  return blink::WebString();
}

const blink::UserAgentMetadata& MockRenderThread::GetUserAgentMetadata() {
  return kUserAgentMetadata;
}

bool MockRenderThread::IsUseZoomForDSF() {
  return zoom_for_dsf_;
}

#if defined(OS_WIN)
void MockRenderThread::PreCacheFont(const LOGFONT& log_font) {
}

void MockRenderThread::ReleaseCachedFonts() {
}
#endif

void MockRenderThread::SetFieldTrialGroup(const std::string& trial_name,
                                          const std::string& group_name) {}

void MockRenderThread::SetUseZoomForDSFEnabled(bool zoom_for_dsf) {
  zoom_for_dsf_ = zoom_for_dsf;
}

int32_t MockRenderThread::GetNextRoutingID() {
  return next_routing_id_++;
}

mojo::PendingReceiver<service_manager::mojom::InterfaceProvider>
MockRenderThread::TakeInitialInterfaceProviderRequestForFrame(
    int32_t routing_id) {
  auto it = frame_routing_id_to_initial_interface_provider_receivers_.find(
      routing_id);
  if (it == frame_routing_id_to_initial_interface_provider_receivers_.end())
    return mojo::NullReceiver();
  auto interface_provider_receiver = std::move(it->second);
  frame_routing_id_to_initial_interface_provider_receivers_.erase(it);
  return interface_provider_receiver;
}

mojo::PendingReceiver<blink::mojom::BrowserInterfaceBroker>
MockRenderThread::TakeInitialBrowserInterfaceBrokerReceiverForFrame(
    int32_t routing_id) {
  auto it =
      frame_routing_id_to_initial_browser_broker_receivers_.find(routing_id);
  if (it == frame_routing_id_to_initial_browser_broker_receivers_.end())
    return mojo::NullReceiver();
  auto browser_broker_receiver = std::move(it->second);
  frame_routing_id_to_initial_browser_broker_receivers_.erase(it);
  return browser_broker_receiver;
}

void MockRenderThread::PassInitialInterfaceProviderReceiverForFrame(
    int32_t routing_id,
    mojo::PendingReceiver<service_manager::mojom::InterfaceProvider>
        interface_provider_receiver) {
  bool did_insertion = false;
  std::tie(std::ignore, did_insertion) =
      frame_routing_id_to_initial_interface_provider_receivers_.emplace(
          routing_id, std::move(interface_provider_receiver));
  DCHECK(did_insertion);
}

// The Frame expects to be returned a valid route_id different from its own.
void MockRenderThread::OnCreateChildFrame(
    const FrameHostMsg_CreateChildFrame_Params& params,
    FrameHostMsg_CreateChildFrame_Params_Reply* params_reply) {
  params_reply->child_routing_id = GetNextRoutingID();
  mojo::PendingRemote<service_manager::mojom::InterfaceProvider>
      interface_provider;
  frame_routing_id_to_initial_interface_provider_receivers_.emplace(
      params_reply->child_routing_id,
      interface_provider.InitWithNewPipeAndPassReceiver());
  params_reply->new_interface_provider =
      interface_provider.PassPipe().release();

  mojo::PendingRemote<blink::mojom::BrowserInterfaceBroker>
      browser_interface_broker;
  frame_routing_id_to_initial_browser_broker_receivers_.emplace(
      params_reply->child_routing_id,
      browser_interface_broker.InitWithNewPipeAndPassReceiver());
  params_reply->browser_interface_broker_handle =
      browser_interface_broker.PassPipe().release();

  params_reply->frame_token = base::UnguessableToken::Create();
  params_reply->devtools_frame_token = base::UnguessableToken::Create();
}

bool MockRenderThread::OnControlMessageReceived(const IPC::Message& msg) {
  for (auto& observer : observers_) {
    if (observer.OnControlMessageReceived(msg))
      return true;
  }
  return OnMessageReceived(msg);
}

bool MockRenderThread::OnMessageReceived(const IPC::Message& msg) {
  // Save the message in the sink.
  sink_.OnMessageReceived(msg);

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MockRenderThread, msg)
    IPC_MESSAGE_HANDLER(FrameHostMsg_CreateChildFrame, OnCreateChildFrame)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

// The View expects to be returned a valid route_id different from its own.
void MockRenderThread::OnCreateWindow(
    const mojom::CreateNewWindowParams& params,
    mojom::CreateNewWindowReply* reply) {
  reply->route_id = GetNextRoutingID();
  reply->main_frame_route_id = GetNextRoutingID();
  reply->main_frame_interface_bundle =
      mojom::DocumentScopedInterfaceBundle::New();
  frame_routing_id_to_initial_interface_provider_receivers_.emplace(
      reply->main_frame_route_id,
      reply->main_frame_interface_bundle->interface_provider
          .InitWithNewPipeAndPassReceiver());
  mojo::PendingRemote<blink::mojom::BrowserInterfaceBroker>
      browser_interface_broker;
  frame_routing_id_to_initial_browser_broker_receivers_.emplace(
      reply->main_frame_route_id,
      browser_interface_broker.InitWithNewPipeAndPassReceiver());
  reply->main_frame_interface_bundle->browser_interface_broker =
      std::move(browser_interface_broker);

  reply->main_frame_frame_token = base::UnguessableToken::Create();
  reply->main_frame_widget_route_id = GetNextRoutingID();
  reply->cloned_session_storage_namespace_id =
      blink::AllocateSessionStorageNamespaceId();

  mojo::AssociatedRemote<blink::mojom::FrameWidget> blink_frame_widget;
  mojo::PendingAssociatedReceiver<blink::mojom::FrameWidget>
      blink_frame_widget_receiver =
          blink_frame_widget.BindNewEndpointAndPassDedicatedReceiver();

  mojo::AssociatedRemote<blink::mojom::FrameWidgetHost> blink_frame_widget_host;
  mojo::PendingAssociatedReceiver<blink::mojom::FrameWidgetHost>
      blink_frame_widget_host_receiver =
          blink_frame_widget_host.BindNewEndpointAndPassDedicatedReceiver();

  reply->frame_widget = std::move(blink_frame_widget_receiver);
  reply->frame_widget_host = blink_frame_widget_host.Unbind();
}

}  // namespace content
