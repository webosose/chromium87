// Copyright 2020 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "neva/injection/renderer/webosgavplugin/webosgavplugin_injection.h"

#include <map>
#include <string>

#include "base/logging.h"
#include "base/values.h"
#include "content/public/renderer/render_frame.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "neva/injection/renderer/grit/injection_resources.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"

namespace injections {

namespace {

inline bool IsTrue(v8::Maybe<bool> maybe) {
  return maybe.IsJust() && maybe.FromJust();
}

}  // namespace

gin::WrapperInfo WebOSGAVInjection::kWrapperInfo = {gin::kEmbedderNativeGin};

const char WebOSGAVInjection::kPluginName[] = "webosgavplugin";
const char WebOSGAVInjection::kOnCreatedMediaLayerName[] =
    "onCreatedMediaLayer";
const char WebOSGAVInjection::kWillDestroyMediaLayer[] =
    "willDestroyMediaLayer";

WebOSGAVInjection::WebOSGAVInjection(blink::WebLocalFrame* frame)
    : InjectionBrowserControlBase(frame),
      data_manager_(CallFunction("initialize")),
      video_window_factory_(content::RenderFrame::FromWebFrame(frame)
                                ->GetFrameVideoWindowFactory()) {}

void WebOSGAVInjection::OnVideoWindowCreated(const std::string& id,
                                             const std::string& video_window_id,
                                             int type) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper))
    return;

  v8::Local<v8::Context> context = wrapper->CreationContext();
  v8::Local<v8::Object> global = context->Global();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> object_key = gin::StringToV8(isolate, kPluginName);

  v8::Local<v8::Object> plugin;
  if (!gin::Converter<v8::Local<v8::Object>>::FromV8(
          isolate, global->Get(context, object_key).ToLocalChecked(),
          &plugin)) {
    LOG(ERROR) << __func__ << " fail to get plugin object";
    return;
  }

  v8::Local<v8::String> callback_key =
      gin::StringToV8(isolate, kOnCreatedMediaLayerName);
  if (!IsTrue(plugin->Has(context, callback_key))) {
    LOG(ERROR) << __func__ << " No " << kOnCreatedMediaLayerName;
    return;
  }

  v8::Local<v8::Function> callback;
  if (!gin::Converter<v8::Local<v8::Function>>::FromV8(
          isolate, plugin->Get(context, callback_key).ToLocalChecked(),
          &callback)) {
    LOG(ERROR) << __func__ << " Convert to function error";
    return;
  }

  const int argc = 3;
  v8::Local<v8::Value> argv[] = {gin::StringToV8(isolate, id),
                                 gin::StringToV8(isolate, video_window_id),
                                 gin::ConvertToV8(isolate, type)};
  ALLOW_UNUSED_LOCAL(callback->Call(context, wrapper, argc, argv));
}

void WebOSGAVInjection::OnVideoWindowDestroyed(const std::string& id) {}

std::string WebOSGAVInjection::gavGetMediaId() {
  std::stringstream ss;
  ss << "plugin_media_" << next_plugin_media_id_++;
  std::string id = ss.str();
  VLOG(1) << __func__ << " id=" << id;
  return ss.str();
}

bool WebOSGAVInjection::gavRequestMediaLayer(const std::string& id, int type) {
  VLOG(1) << __func__ << " id=" << id << " type=" << type;
  if (id_to_window_.find(id) != id_to_window_.end()) {
    LOG(ERROR) << __func__ << " already requested";
    return false;
  }

  mojo::PendingRemote<ui::mojom::VideoWindow> window_remote;
  mojo::PendingRemote<ui::mojom::VideoWindowClient> client_remote;
  mojo::PendingReceiver<ui::mojom::VideoWindowClient> client_receiver(
      client_remote.InitWithNewPipeAndPassReceiver());

  // TODO(neva): setup disconnect_handler
  video_window_factory_->CreateVideoWindow(
      std::move(client_remote), window_remote.InitWithNewPipeAndPassReceiver(),
      {false, true});

  auto result = id_to_window_.emplace(
      std::piecewise_construct, std::forward_as_tuple(id),
      std::forward_as_tuple(this, id, type, std::move(window_remote),
                            std::move(client_receiver)));
  result.first->second.window_remote_.set_disconnect_handler(base::BindOnce(
      &WebOSGAVInjection::gavMediaLayerDestroyed, base::Unretained(this), id));

  return true;
}

// Destroyed from video window
void WebOSGAVInjection::gavMediaLayerDestroyed(const std::string& id) {
  auto it = id_to_window_.find(id);
  if (it == id_to_window_.end()) {
    return;
  }
  id_to_window_.erase(it);

  // Notify to plugin
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper))
    return;

  v8::Local<v8::Context> context = wrapper->CreationContext();
  v8::Local<v8::Object> global = context->Global();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> object_key = gin::StringToV8(isolate, kPluginName);

  v8::Local<v8::Object> plugin;
  if (!gin::Converter<v8::Local<v8::Object>>::FromV8(
          isolate, global->Get(context, object_key).ToLocalChecked(),
          &plugin)) {
    LOG(ERROR) << __func__ << " fail to get plugin object";
    return;
  }

  v8::Local<v8::String> callback_key =
      gin::StringToV8(isolate, kWillDestroyMediaLayer);
  if (!IsTrue(plugin->Has(context, callback_key))) {
    LOG(ERROR) << __func__ << " No " << kWillDestroyMediaLayer;
    return;
  }

  v8::Local<v8::Function> callback;
  if (!gin::Converter<v8::Local<v8::Function>>::FromV8(
          isolate, plugin->Get(context, callback_key).ToLocalChecked(),
          &callback)) {
    LOG(ERROR) << __func__ << " Convert to function error";
    return;
  }

  const int argc = 1;
  v8::Local<v8::Value> argv[] = {gin::StringToV8(isolate, id)};
  ALLOW_UNUSED_LOCAL(callback->Call(context, wrapper, argc, argv));
}

void WebOSGAVInjection::gavUpdateMediaLayerBounds(const std::string& id,
                                                  int src_x,
                                                  int src_y,
                                                  int src_w,
                                                  int src_h,
                                                  int dst_x,
                                                  int dst_y,
                                                  int dst_w,
                                                  int dst_h) {
  VLOG(1) << __func__ << " id=" << id << " src(" << src_x << "," << src_y
          << ") " << src_w << "x" << src_h << " dst(" << dst_x << "," << dst_y
          << ") " << dst_w << "x" << dst_h;
  auto it = id_to_window_.find(id);
  if (it == id_to_window_.end()) {
    LOG(ERROR) << __func__ << " failed to find id=" << id;
    return;
  }
  it->second.GetVideoWindow()->UpdateVideoWindowGeometry(
      gfx::Rect(src_x, src_y, src_w, src_h),
      gfx::Rect(dst_x, dst_y, dst_w, dst_h));
}

void WebOSGAVInjection::gavUpdateMediaCropBounds(const std::string& id,
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
                                                 int dst_h) {
  VLOG(1) << __func__ << " id=" << id << " ori(" << ori_x << "," << ori_y
          << ") " << ori_w << "x" << ori_h << " src(" << src_x << "," << src_y
          << ") " << src_w << "x" << src_h << " dst(" << dst_x << "," << dst_y
          << ") " << dst_w << "x" << dst_h;
  auto it = id_to_window_.find(id);
  if (it == id_to_window_.end()) {
    LOG(ERROR) << __func__ << " failed to find id=" << id;
    return;
  }
  it->second.GetVideoWindow()->UpdateVideoWindowGeometryWithCrop(
      gfx::Rect(ori_x, ori_y, ori_w, ori_h),
      gfx::Rect(src_x, src_y, src_w, src_h),
      gfx::Rect(dst_x, dst_y, dst_w, dst_h));
}

// Destroy video window from plugin
void WebOSGAVInjection::gavDestroyMediaLayer(const std::string& id) {
  VLOG(1) << __func__ << " id=" << id;
  // Erase will destruct VideoWindowImpl then mojo pipe will be closed.
  id_to_window_.erase(id);
}

void WebOSGAVInjection::gavSetMediaProperty(const std::string& id,
                                            const std::string& name,
                                            const std::string& value) {
  VLOG(1) << __func__ << " id=" << id << " name=" << name << " value=" << value;
  auto it = id_to_window_.find(id);
  if (it == id_to_window_.end()) {
    LOG(ERROR) << __func__ << " failed to find id=" << id;
    return;
  }
  it->second.GetVideoWindow()->SetProperty(name, value);
}

void WebOSGAVInjection::ReloadInjectionData() {
  data_manager_.SetInitializedStatus(false);
  const std::string json = CallFunction("initialize");
  data_manager_.DoInitialize(json);
}

void WebOSGAVInjection::UpdateInjectionData(const std::string& key,
                                            const std::string& value) {
  data_manager_.UpdateInjectionData(key, value);
}

gin::ObjectTemplateBuilder WebOSGAVInjection::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<WebOSGAVInjection>::GetObjectTemplateBuilder(isolate)
      .SetMethod("gavGetMediaId", &WebOSGAVInjection::gavGetMediaId)
      .SetMethod("gavRequestMediaLayer",
                 &WebOSGAVInjection::gavRequestMediaLayer)
      .SetMethod("gavUpdateMediaLayerBounds",
                 &WebOSGAVInjection::gavUpdateMediaLayerBounds)
      .SetMethod("gavDestroyMediaLayer",
                 &WebOSGAVInjection::gavDestroyMediaLayer)
      .SetMethod("gavUpdateMediaCropBounds",
                 &WebOSGAVInjection::gavUpdateMediaCropBounds)
      .SetMethod("gavSetMediaProperty", &WebOSGAVInjection::gavSetMediaProperty)
      .SetMethod("reloadInjectionData", &WebOSGAVInjection::ReloadInjectionData)
      .SetMethod("updateInjectionData",
                 &WebOSGAVInjection::UpdateInjectionData);
}

std::string WebOSGAVInjection::GetInjectionData(const std::string& name) {
  if (!data_manager_.GetInitialisedStatus()) {
    const std::string json = CallFunction("webosgavplugin,initialize");
    data_manager_.DoInitialize(json);
  }

  if (data_manager_.GetInitialisedStatus()) {
    std::string result;
    if (data_manager_.GetInjectionData(name, result))
      return result;
  }

  return CallFunction(name);
}

const char WebOSGAVInjection::kInjectionObjectName[] = "WebOSGAVInternal_";

void WebOSGAVInjection::Install(blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Local<v8::Object> global = context->Global();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Value> webosgavplugin_value =
      global->Get(context, gin::StringToV8(isolate, kInjectionObjectName))
          .ToLocalChecked();
  if (!webosgavplugin_value.IsEmpty() && webosgavplugin_value->IsObject())
    return;

  v8::Local<v8::Object> webosgavplugin;
  if (!CreateWebOSGAVObject(frame, isolate, global).ToLocal(&webosgavplugin))
    return;

  WebOSGAVInjection* webosgavplugin_injection = nullptr;
  if (gin::Converter<WebOSGAVInjection*>::FromV8(isolate, webosgavplugin,
                                                 &webosgavplugin_injection)) {
    const std::string extra_objects_js =
        ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
            IDR_WEBOSGAVPLUGIN_INJECTION_JS);

    v8::Local<v8::Script> script;
    if (v8::Script::Compile(context,
                            gin::StringToV8(isolate, extra_objects_js.c_str()))
            .ToLocal(&script))
      script->Run(context);
  }
}

// static
void WebOSGAVInjection::Uninstall(blink::WebLocalFrame* frame) {
  const std::string extra_objects_js =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_WEBOSGAVPLUGIN_ROLLBACK_JS);

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Script> local_script;
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(
      context, gin::StringToV8(isolate, extra_objects_js.c_str()));

  if (script.ToLocal(&local_script))
    local_script->Run(context);
}

// static
v8::MaybeLocal<v8::Object> WebOSGAVInjection::CreateWebOSGAVObject(
    blink::WebLocalFrame* frame,
    v8::Isolate* isolate,
    v8::Local<v8::Object> parent) {
  gin::Handle<WebOSGAVInjection> webosgavplugin =
      gin::CreateHandle(isolate, new WebOSGAVInjection(frame));
  parent
      ->Set(frame->MainWorldScriptContext(),
            gin::StringToV8(isolate, kInjectionObjectName),
            webosgavplugin.ToV8())
      .Check();
  return webosgavplugin->GetWrapper(isolate);
}

}  // namespace injections
