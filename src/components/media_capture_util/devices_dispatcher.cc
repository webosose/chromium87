// Copyright 2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "components/media_capture_util/devices_dispatcher.h"

#include "base/callback.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/media_capture_devices.h"

using content::BrowserThread;

namespace media_capture_util {
namespace {

// Finds a device in |devices| that has |device_id|, or nullptr if not found.
const blink::MediaStreamDevice* FindDeviceWithId(
    const blink::MediaStreamDevices& devices,
    const std::string& device_id) {
  blink::MediaStreamDevices::const_iterator iter = devices.begin();
  for (; iter != devices.end(); ++iter) {
    if (iter->id == device_id) {
      return &(*iter);
    }
  }
  return nullptr;
}

const blink::MediaStreamDevice* GetAudioDevice(
    const std::string& requested_audio_device_id) {
  const blink::MediaStreamDevices& audio_devices =
      content::MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices();

  return FindDeviceWithId(audio_devices, requested_audio_device_id);
}

const blink::MediaStreamDevice* GetVideoDevice(
    const std::string& requested_video_device_id) {
  const blink::MediaStreamDevices& video_devices =
      content::MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices();
  return FindDeviceWithId(video_devices, requested_video_device_id);
}

}  // namespace

DevicesDispatcher* DevicesDispatcher::GetInstance() {
  return base::Singleton<DevicesDispatcher>::get();
}

void DevicesDispatcher::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    bool accepts_video,
    bool accepts_audio,
    content::MediaResponseCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  blink::MediaStreamDevices devices;
  std::unique_ptr<content::MediaStreamUI> ui;

  if (request.audio_type ==
          blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE &&
      accepts_audio) {
    const blink::MediaStreamDevice* device =
        GetAudioDevice(request.requested_audio_device_id);
    if (device)
      devices.push_back(*device);
  }
  if (request.video_type ==
          blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE &&
      accepts_video) {
    const blink::MediaStreamDevice* device =
        GetVideoDevice(request.requested_video_device_id);
    if (device)
      devices.push_back(*device);
  }

  std::move(callback).Run(
      devices,
      devices.empty() ? blink::mojom::MediaStreamRequestResult::NO_HARDWARE
                      : blink::mojom::MediaStreamRequestResult::OK,
      std::move(ui));
}

}  // namespace media_capture_util
