// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/memory/userspace_swap/userfaultfd.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#if defined(__NR_userfaultfd)
#define HAS_USERFAULTFD
#include <linux/userfaultfd.h>
#endif

#include "base/bind.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/memory/aligned_memory.h"
#include "base/memory/ptr_util.h"
#include "base/posix/eintr_wrapper.h"
#include "base/sequenced_task_runner.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/platform_thread.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace chromeos {
namespace memory {
namespace userspace_swap {

UserfaultFD::~UserfaultFD() {
  // We need to make sure we stop receiving events before we shutdown.
  CloseAndStopWaitingForEvents();
}

UserfaultFD::UserfaultFD(base::ScopedFD fd) : fd_(std::move(fd)) {}

// static
bool UserfaultFD::KernelSupportsUserfaultFD() {
#if defined(HAS_USERFAULTFD)
  static bool supported = []() -> bool {
    // Invoke the syscall with invalid arguments. If it's not supported the
    // kernel will return ENOSYS, if it's supported we will get invalid
    // arguments EINVAL. Doing this will never actually create a userfaultfd.
    int ret = syscall(__NR_userfaultfd, ~(O_CLOEXEC | O_NONBLOCK));
    CHECK(ret == -1) << "Syscall succeeded unexpectedly";
    DPCHECK(errno == EINVAL || errno == ENOSYS)
        << "Syscall returned an unexpected errno";
    return errno == EINVAL;
  }();

  return supported;
#else  // defined(HAS_USERFAULTFD)
  // We didn't even build chrome with support for it in this case.
  errno = ENOSYS;
  return false;
#endif
}

bool UserfaultFD::RegisterRange(RegisterMode mode,
                                uintptr_t range_start,
                                uint64_t len) {
#if defined(HAS_USERFAULTFD)
  CHECK(base::IsPageAligned(range_start));
  CHECK(base::IsPageAligned(len));

  uffdio_register reg = {};
  reg.range.start = range_start;
  reg.range.len = len;

  reg.mode = 0;
  if (mode & kRegisterMissing)
    reg.mode |= UFFDIO_REGISTER_MODE_MISSING;

  if (HANDLE_EINTR(ioctl(fd_.get(), UFFDIO_REGISTER, &reg)) == -1) {
    return false;
  }

  // To be forward compatible we make sure that the ioctls we were expecting (at
  // compile time) at minimum are in the ioctls returned from the kernel.
  CHECK((reg.ioctls & UFFD_API_RANGE_IOCTLS) == UFFD_API_RANGE_IOCTLS);

  return true;
#else  // defined(HAS_USERFAULTFD)
  errno = ENOSYS;
  return false;
#endif
}

bool UserfaultFD::UnregisterRange(uintptr_t range_start, uint64_t len) {
#if defined(HAS_USERFAULTFD)
  CHECK(base::IsPageAligned(range_start));
  CHECK(base::IsPageAligned(len));

  uffdio_range range = {};
  range.start = range_start;
  range.len = len;

  if (HANDLE_EINTR(ioctl(fd_.get(), UFFDIO_UNREGISTER, &range)) < 0) {
    return false;
  }

  return true;
#else  // defined(HAS_USERFAULTFD)
  errno = ENOSYS;
  return false;
#endif
}

bool UserfaultFD::CopyToRange(uintptr_t dest_range_start,
                              uint64_t len,
                              uintptr_t src_range_start,
                              int64_t* copied) {
#if defined(HAS_USERFAULTFD)
  // NOTE: The source doesn't need to be page aligned.
  CHECK(base::IsPageAligned(dest_range_start));
  CHECK(base::IsPageAligned(len));
  CHECK(copied);

  uffdio_copy fault_copy = {};
  fault_copy.dst = dest_range_start;
  fault_copy.len = len;
  fault_copy.mode = 0;  // WAKE
  fault_copy.src = src_range_start;
  fault_copy.copy = 0;
  *copied = 0;

  int res = HANDLE_EINTR(ioctl(fd_.get(), UFFDIO_COPY, &fault_copy));
  *copied = fault_copy.copy;

  return res >= 0;
#else  // defined(HAS_USERFAULTFD)
  errno = ENOSYS;
  return false;
#endif
}

bool UserfaultFD::ZeroRange(uintptr_t range_start,
                            uint64_t len,
                            int64_t* zeroed) {
#if defined(HAS_USERFAULTFD)
  CHECK(base::IsPageAligned(range_start));
  CHECK(base::IsPageAligned(len));
  CHECK(zeroed);

  uffdio_zeropage zp = {};
  zp.range.start = range_start;
  zp.range.len = len;
  zp.mode = 0;  // WAKE
  zp.zeropage = 0;
  *zeroed = 0;

  int res = HANDLE_EINTR(ioctl(fd_.get(), UFFDIO_ZEROPAGE, &zp));
  *zeroed = zp.zeropage;

  return res >= 0;
#else  // defined(HAS_USERFAULTFD)
  errno = ENOSYS;
  return false;
#endif
}

bool UserfaultFD::WakeRange(uintptr_t range_start, uint64_t len) {
#if defined(HAS_USERFAULTFD)
  CHECK(base::IsPageAligned(range_start));
  CHECK(base::IsPageAligned(len));

  uffdio_range range = {};
  range.start = range_start;
  range.len = len;

  if (HANDLE_EINTR(ioctl(fd_.get(), UFFDIO_WAKE, &range)) < 0) {
    return false;
  }

  return true;
#else  // defined(HAS_USERFAULTFD)
  errno = ENOSYS;
  return false;
#endif
}

// Static
std::unique_ptr<UserfaultFD> UserfaultFD::WrapFD(base::ScopedFD fd) {
#if defined(HAS_USERFAULTFD)
  // Using new to access non-public constructor rather than make_unique.
  return base::WrapUnique(new UserfaultFD(std::move(fd)));
#else  // defined(HAS_USERFAULTFD)
  errno = ENOSYS;
  return nullptr;
#endif
}

// Static
std::unique_ptr<UserfaultFD> UserfaultFD::Create(Features features) {
#if defined(HAS_USERFAULTFD)
  base::ScopedFD fd(syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK));
  if (!fd.is_valid()) {
    // We likely received ENOSYS in this situation for no kernel support, we
    // don't need to do anything special although the caller can still check
    // errno. Although we do log an error since the caller should have checked
    // that it's supported before attempting to create a userfaultfd.
    PLOG(ERROR) << "Unable to create userfaultfd";
    return nullptr;
  }

  uffdio_api uffdio_api = {};
  uffdio_api.api = UFFD_API;

  if (features & kFeatureRemap)
    uffdio_api.features |= UFFD_FEATURE_EVENT_REMAP;

  if (features & kFeatureUnmap)
    uffdio_api.features |= UFFD_FEATURE_EVENT_UNMAP;

  if (features & kFeatureRemove)
    uffdio_api.features |= UFFD_FEATURE_EVENT_REMOVE;

  if (features & kFeatureThreadID)
    uffdio_api.features |= UFFD_FEATURE_THREAD_ID;

  if (HANDLE_EINTR(ioctl(fd.get(), UFFDIO_API, &uffdio_api)) < 0) {
    PLOG(ERROR) << "UFFDIO_API ioctl failed";
    return nullptr;
  }

  return WrapFD(std::move(fd));
#else  // defined(HAS_USERFAULTFD)
  errno = ENOSYS;
  return nullptr;
#endif
}

void UserfaultFD::DispatchMessage(const uffd_msg& msg) {
#if defined(HAS_USERFAULTFD)
  if (msg.event == UFFD_EVENT_UNMAP) {
    handler_->Unmapped(msg.arg.remove.start, msg.arg.remove.end);
  } else if (msg.event == UFFD_EVENT_REMOVE) {
    handler_->Removed(msg.arg.remove.start, msg.arg.remove.end);
  } else if (msg.event == UFFD_EVENT_REMAP) {
    handler_->Remapped(msg.arg.remap.from, msg.arg.remap.to, msg.arg.remap.len);
  } else if (msg.event == UFFD_EVENT_PAGEFAULT) {
    handler_->Pagefault(msg.arg.pagefault.address,
                        msg.arg.pagefault.flags & UFFD_PAGEFAULT_FLAG_WRITE
                            ? UserfaultFDHandler::PagefaultFlags::kWriteFault
                            : UserfaultFDHandler::PagefaultFlags::kReadFault,
                        msg.arg.pagefault.feat.ptid);
  } else {
    DLOG(ERROR) << "Unknown userfaultfd event: " << msg.event;
  }

#endif
}

void UserfaultFD::UserfaultFDReadable() {
#if defined(HAS_USERFAULTFD)
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  constexpr int kMsgsToRead = 32;  // Try to read up to 32 messages in one shot.
  uffd_msg msgs[kMsgsToRead];
  int msgs_read = 0;

  // It's very important that messages are posted in the order they were read
  // otherwise, if another thread also attempted to handle the
  // UserfaultFDReadable event we could post the messages out of order which may
  // result in ambiguity. We protect the read and the posts by a mutex.
  base::ReleasableAutoLock read_locker(&read_lock_);

  do {
    memset(msgs, 0, sizeof(msgs));

    // We start by draining all messages and then we process them in order.
    int bytes_read = HANDLE_EINTR(read(fd_.get(), msgs, sizeof(msgs)));

    if (bytes_read <= 0) {
      if (errno == EAGAIN) {
        // No problems here.
        return;
      }

      // We either got an EOF or an EBADF to indicate that we're done.
      if (errno == EBADF || bytes_read == 0) {
        handler_->Closed(0);  // EBADF will indicate closed at this point.
      } else {
        PLOG(ERROR) << "Userfaultfd encountered an expected error";
        handler_->Closed(errno);
      }

      CloseAndStopWaitingForEvents();
      return;
    }

    // Partial reads CANNOT happen.
    CHECK_EQ(bytes_read % sizeof(msgs[0]), 0u);

    msgs_read = bytes_read / sizeof(msgs[0]);
    DCHECK(msgs_read);

    for (int i = 0; i < msgs_read; ++i) {
      DispatchMessage(msgs[i]);
    }
  } while (msgs_read == kMsgsToRead);
#endif
}

bool UserfaultFD::StartWaitingForEvents(
    std::unique_ptr<UserfaultFDHandler> handler) {
#if defined(HAS_USERFAULTFD)

  if (!fd_.is_valid() || !handler) {
    return false;
  }

  if (watcher_controller_) {
    LOG(WARNING) << "Fault handling has already started";
    return true;
  }

  handler_ = std::move(handler);

  watcher_controller_ = base::FileDescriptorWatcher::WatchReadable(
      fd_.get(), base::BindRepeating(&UserfaultFD::UserfaultFDReadable,
                                     base::Unretained(this)));

  return true;
#else  // defined(HAS_USERFAULTFD)
  errno = ENOSYS;
  return false;
#endif
}

void UserfaultFD::CloseAndStopWaitingForEvents() {
  watcher_controller_.reset();
  fd_.reset();
}

base::ScopedFD UserfaultFD::ReleaseFD() {
  return std::move(fd_);
}

}  // namespace userspace_swap
}  // namespace memory
}  // namespace chromeos
