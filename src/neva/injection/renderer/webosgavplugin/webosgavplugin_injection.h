// Copyright 2020 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NEVA_INJECTION_RENDERER_WEBOSGAVPLUGIN_WEBOSGAVPLUGIN_INJECTION_H_
#define NEVA_INJECTION_RENDERER_WEBOSGAVPLUGIN_WEBOSGAVPLUGIN_INJECTION_H_

#include <map>
#include <string>

#include "content/public/common/neva/frame_video_window_factory.mojom.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "neva/injection/renderer/injection_browser_control_base.h"
#include "neva/injection/renderer/webosgavplugin/videowindow_impl.h"
#include "neva/injection/renderer/webosgavplugin/webosgavplugin_datamanager.h"
#include "v8/include/v8.h"

namespace blink {
class WebLocalFrame;
}  // namespace blink

namespace injections {

class WebOSGAVInjection : public VideoWindowClientOwner,
                          public gin::Wrappable<WebOSGAVInjection>,
                          public InjectionBrowserControlBase {
 public:
  static gin::WrapperInfo kWrapperInfo;
  static const char kInjectionObjectName[];
  static void Install(blink::WebLocalFrame* frame);
  static void Uninstall(blink::WebLocalFrame* frame);

  explicit WebOSGAVInjection(blink::WebLocalFrame* frame);
  WebOSGAVInjection(const WebOSGAVInjection&) = delete;
  WebOSGAVInjection& operator=(const WebOSGAVInjection&) = delete;

  std::string gavGetMediaId();
  bool gavRequestMediaLayer(const std::string& mid, int type);

  void gavUpdateMediaLayerBounds(const std::string& mid,
                                 int src_x,
                                 int src_y,
                                 int src_w,
                                 int src_h,
                                 int dst_x,
                                 int dst_y,
                                 int dst_w,
                                 int dst_h);

  void gavDestroyMediaLayer(const std::string& mid);
  void gavUpdateMediaCropBounds(const std::string& mid,
                                int ori_x,
                                int ori_y,
                                int ori_w,
                                int ori_h,
                                int src_x,
                                int src_y,
                                int src_w,
                                int src_h,
                                int dst_x,
                                int dst_y,
                                int dst_w,
                                int dst_h);
  void gavSetMediaProperty(const std::string& mid,
                           const std::string& name,
                           const std::string& value);

  void ReloadInjectionData();
  void UpdateInjectionData(const std::string& key, const std::string& value);

  // Implements VideoWindowClientOwner
  void OnVideoWindowCreated(const std::string& id,
                            const std::string& video_window_id,
                            int type) final;
  void OnVideoWindowDestroyed(const std::string& id) final;

 private:
  static v8::MaybeLocal<v8::Object> CreateWebOSGAVObject(
      blink::WebLocalFrame* frame,
      v8::Isolate* isolate,
      v8::Local<v8::Object> global);

  static const char kPluginName[];
  static const char kOnCreatedMediaLayerName[];
  static const char kWillDestroyMediaLayer[];

  void gavMediaLayerDestroyed(const std::string& id);

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;

  std::string GetInjectionData(const std::string& name);
  WebOSGAVDataManager data_manager_;
  content::mojom::FrameVideoWindowFactory* video_window_factory_;
  unsigned int next_plugin_media_id_ = 0;
  std::map<std::string, VideoWindowImpl> id_to_window_;
};

}  // namespace injections

#endif  // NEVA_INJECTION_RENDERER_WEBOSGAVPLUGIN_WEBOSGAVPLUGIN_INJECTION_H_
