// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/memory_pressure_listener.h"

#include "base/observer_list_threadsafe.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/trace_event/base_tracing.h"

namespace base {

namespace {

// This class is thread safe and internally synchronized.
class MemoryPressureObserver {
 public:
  // There is at most one MemoryPressureObserver and it is never deleted.
  ~MemoryPressureObserver() = delete;

  void AddObserver(MemoryPressureListener* listener, bool sync) {
    // TODO(crbug.com/1063868): DCHECK instead of silently failing when a
    // MemoryPressureListener is created in a non-sequenced context. Tests will
    // need to be adjusted for that to work.
    if (SequencedTaskRunnerHandle::IsSet()) {
      async_observers_->AddObserver(listener);
    }

    if (sync) {
      AutoLock lock(sync_observers_lock_);
      sync_observers_.AddObserver(listener);
    }
  }

  void RemoveObserver(MemoryPressureListener* listener) {
    async_observers_->RemoveObserver(listener);
    AutoLock lock(sync_observers_lock_);
    sync_observers_.RemoveObserver(listener);
  }

  void Notify(
      MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
    async_observers_->Notify(FROM_HERE, &MemoryPressureListener::Notify,
                             memory_pressure_level);
    AutoLock lock(sync_observers_lock_);
    for (auto& observer : sync_observers_)
      observer.SyncNotify(memory_pressure_level);
  }

 private:
  const scoped_refptr<ObserverListThreadSafe<MemoryPressureListener>>
      async_observers_ =
          base::MakeRefCounted<ObserverListThreadSafe<MemoryPressureListener>>(
              ObserverListPolicy::EXISTING_ONLY);
  ObserverList<MemoryPressureListener>::Unchecked sync_observers_;
  Lock sync_observers_lock_;
};

// Gets the shared MemoryPressureObserver singleton instance.
MemoryPressureObserver* GetMemoryPressureObserver() {
  static auto* const observer = new MemoryPressureObserver();
  return observer;
}

subtle::Atomic32 g_notifications_suppressed = 0;

}  // namespace

MemoryPressureListener::MemoryPressureListener(
    const base::Location& creation_location,
    const MemoryPressureListener::MemoryPressureCallback& callback)
    : callback_(callback), creation_location_(creation_location) {
  GetMemoryPressureObserver()->AddObserver(this, false);
}

MemoryPressureListener::MemoryPressureListener(
    const base::Location& creation_location,
    const MemoryPressureListener::MemoryPressureCallback& callback,
    const MemoryPressureListener::SyncMemoryPressureCallback&
        sync_memory_pressure_callback)
    : callback_(callback),
      sync_memory_pressure_callback_(sync_memory_pressure_callback),
      creation_location_(creation_location) {
  GetMemoryPressureObserver()->AddObserver(this, true);
}

MemoryPressureListener::~MemoryPressureListener() {
  GetMemoryPressureObserver()->RemoveObserver(this);
}

void MemoryPressureListener::Notify(MemoryPressureLevel memory_pressure_level) {
  TRACE_EVENT2("base", "MemoryPressureListener::Notify",
               "listener_creation_info", creation_location_.ToString(), "level",
               memory_pressure_level);
  callback_.Run(memory_pressure_level);
}

void MemoryPressureListener::SyncNotify(
    MemoryPressureLevel memory_pressure_level) {
  if (!sync_memory_pressure_callback_.is_null()) {
    sync_memory_pressure_callback_.Run(memory_pressure_level);
  }
}

// static
void MemoryPressureListener::NotifyMemoryPressure(
    MemoryPressureLevel memory_pressure_level) {
#if !defined(USE_NEVA_APPRUNTIME)
  DCHECK_NE(memory_pressure_level, MEMORY_PRESSURE_LEVEL_NONE);
#endif
  TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("memory-infra"),
                       "MemoryPressureListener::NotifyMemoryPressure",
                       TRACE_EVENT_SCOPE_THREAD, "level",
                       memory_pressure_level);
  if (AreNotificationsSuppressed())
    return;
  DoNotifyMemoryPressure(memory_pressure_level);
}

// static
bool MemoryPressureListener::AreNotificationsSuppressed() {
  return subtle::Acquire_Load(&g_notifications_suppressed) == 1;
}

// static
void MemoryPressureListener::SetNotificationsSuppressed(bool suppress) {
  subtle::Release_Store(&g_notifications_suppressed, suppress ? 1 : 0);
}

// static
void MemoryPressureListener::SimulatePressureNotification(
    MemoryPressureLevel memory_pressure_level) {
  // Notify all listeners even if regular pressure notifications are suppressed.
  DoNotifyMemoryPressure(memory_pressure_level);
}

// static
void MemoryPressureListener::DoNotifyMemoryPressure(
    MemoryPressureLevel memory_pressure_level) {
#if !defined(USE_NEVA_APPRUNTIME)
  DCHECK_NE(memory_pressure_level, MEMORY_PRESSURE_LEVEL_NONE);
#endif

  GetMemoryPressureObserver()->Notify(memory_pressure_level);
}

}  // namespace base
