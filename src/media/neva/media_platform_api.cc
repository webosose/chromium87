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

#include "media/neva/media_platform_api.h"

namespace media {

MediaPlatformAPI::BufferQueue::BufferQueue() : data_size_(0) {}
MediaPlatformAPI::BufferQueue::~BufferQueue() {}
void MediaPlatformAPI::BufferQueue::Push(
    const scoped_refptr<DecoderBuffer>& buffer,
    FeedType type) {
  queue_.push(DecoderBufferPair(buffer, type));
  data_size_ += buffer->data_size();
}

const std::pair<scoped_refptr<DecoderBuffer>, FeedType>
MediaPlatformAPI::BufferQueue::Front() {
  return queue_.front();
}

void MediaPlatformAPI::BufferQueue::Pop() {
  std::pair<scoped_refptr<DecoderBuffer>, FeedType> f = queue_.front();
  data_size_ -= f.first->data_size();
  queue_.pop();
}

bool MediaPlatformAPI::BufferQueue::Empty() {
  return queue_.empty();
}

void MediaPlatformAPI::BufferQueue::Clear() {
  DecoderBufferQueue empty_queue;
  queue_.swap(empty_queue);
}

size_t MediaPlatformAPI::BufferQueue::DataSize() const {
  return data_size_;
}

MediaPlatformAPI::MediaPlatformAPI() {}

MediaPlatformAPI::~MediaPlatformAPI() {}

base::TimeDelta MediaPlatformAPI::GetCurrentTime() {
  base::AutoLock auto_lock(current_time_lock_);
  return current_time_;
}

void MediaPlatformAPI::UpdateCurrentTime(const base::TimeDelta& time) {
  base::AutoLock auto_lock(current_time_lock_);
  current_time_ = time;
}

void MediaPlatformAPI::SetMediaLayerId(const std::string& media_layer_id) {
  media_layer_id_ = media_layer_id;
}

void MediaPlatformAPI::SetMediaPreferences(const std::string& preferences) {
  preferences_ = preferences;
}

void MediaPlatformAPI::SetMediaCodecCapabilities(
    const std::vector<MediaCodecCapability>& capabilities) {
  capabilities_ = capabilities;
}

base::Optional<MediaCodecCapability>
MediaPlatformAPI::GetMediaCodecCapabilityForCodec(const std::string& codec) {
  auto it = std::find_if(capabilities_.begin(), capabilities_.end(),
                         [&codec](const MediaCodecCapability& capability) {
                           return codec == capability.codec;
                         });
  if (it != capabilities_.end())
    return *it;
  return base::nullopt;
}

}  // namespace media
