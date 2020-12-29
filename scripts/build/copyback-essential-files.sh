#!/bin/sh

# Usage
# ./copyback-essential-files.sh <path-to-original-upstream-chromium-with-gclient-run>
origin_dir=$1

mkdir -p src/chromecast/common/mojom
mkdir -p src/chrome/test/data/webui
mkdir -p src/chrome/test/data/media/engagement/preload
mkdir -p src/content/test/data
mkdir -p src/tools/perf/contrib/vr_benchmarks
mkdir -p src/tools/traffic_annotation
mkdir -p src/components/test

cp $origin_dir/src/chromecast/common/mojom/. src/chromecast/common/mojom/ -a
cp $origin_dir/src/chrome/test/data/webui/i18n_process_css_test.html src/chrome/test/data/webui/i18n_process_css_test.html
cp $origin_dir/src/chrome/test/data/webui/web_ui_test.mojom src/chrome/test/data/webui/web_ui_test.mojom
cp $origin_dir/src/chrome/test/data/BUILD.gn src/chrome/test/data/BUILD.gn
cp $origin_dir/src/chrome/test/data/webui/BUILD.gn src/chrome/test/data/webui/BUILD.gn
cp $origin_dir/src/chrome/test/data/media/engagement/preload/BUILD.gn src/chrome/test/data/media/engagement/preload/BUILD.gn
cp $origin_dir/src/content/test/data/lite_js_test.mojom src/content/test/data/lite_js_test.mojom
#cp $origin_dir/src/content/test/data/mojo_layouttest_test.mojom src/content/test/data/mojo_layouttest_test.mojom
cp -a $origin_dir/src/tools/perf/clear_system_cache src/tools/perf/clear_system_cache
cp -a $origin_dir/src/tools/perf/chrome_telemetry_build src/tools/perf/chrome_telemetry_build
cp $origin_dir/src/tools/perf/BUILD.gn src/tools/perf/BUILD.gn
cp $origin_dir/src/tools/perf/contrib/vr_benchmarks/BUILD.gn src/tools/perf/contrib/vr_benchmarks/
cp -a $origin_dir/src/tools/traffic_annotation/auditor src/tools/traffic_annotation/auditor
cp $origin_dir/src/components/test/BUILD.gn src/components/test/BUILD.gn

#Chromium79 more
#cp $origin_dir/src/chromecast/typemaps.gni src/chromecast/
mkdir -p src/fuchsia/mojom/
#cp $origin_dir/src/fuchsia/mojom/test_typemaps.gni src/fuchsia/mojom/
mkdir -p src/media/fuchsia/mojom/
#cp $origin_dir/src/media/fuchsia/mojom/typemaps.gni src/media/fuchsia/mojom/typemaps.gni
#cp $origin_dir/src/fuchsia/mojom/example.typemap src/fuchsia/mojom/example.typemap
#cp $origin_dir/src/media/fuchsia/mojom/cdm_request.typemap src/media/fuchsia/mojom/cdm_request.typemap
cp $origin_dir/src/fuchsia/mojom/example.mojom src/fuchsia/mojom/example.mojom
#cp $origin_dir/src/media/fuchsia/mojom/fuchsia_cdm_provider.mojom src/media/fuchsia/mojom/fuchsia_cdm_provider.mojom
mkdir src/chrome/test/data/webui/cr_components
cp $origin_dir/src/chrome/test/data/webui/cr_components/BUILD.gn src/chrome/test/data/webui/cr_components/BUILD.gn
cp $origin_dir/src/chrome/test/data/webui/cr_components/BUILD.gn src/chrome/test/data/webui/cr_components/BUILD.gn
mkdir src/chrome/test/data/webui/cr_elements/
cp $origin_dir/src/chrome/test/data/webui/cr_elements/BUILD.gn src/chrome/test/data/webui/cr_elements/BUILD.gn
cp $origin_dir/src/content/test/data/mojo_web_test_helper_test.mojom src/content/test/data/mojo_web_test_helper_test.mojom
#cp $origin_dir/src/content/test/data/lite_js_old_names_test.mojom src/content/test/data/lite_js_old_names_test.mojom
mkdir src/android_webview
cp $origin_dir/src/android_webview/OWNERS src/android_webview/OWNERS
mkdir -p src/third_party/swiftshader/tests/GLESUnitTests
cp $origin_dir/src/third_party/swiftshader/tests/GLESUnitTests/BUILD.gn src/third_party/swiftshader/tests/GLESUnitTests/
mkdir -p src/third_party/swiftshader/tests/SystemUnitTests
cp $origin_dir/src/third_party/swiftshader/tests/SystemUnitTests/BUILD.gn src/third_party/swiftshader/tests/SystemUnitTests/
mkdir -p src/third_party/catapult/third_party/polymer/components/polymer/
cp $origin_dir/src/third_party/catapult/third_party/polymer/. src/third_party/catapult/third_party/polymer/ -a
mkdir -p src/third_party/catapult/tracing
cp $origin_dir/src/third_party/catapult/tracing/. src/third_party/catapult/tracing/ -a
rm -rf src/third_party/catapult/tracing/test_data
cp $origin_dir/src/third_party/catapult/BUILD.gn src/third_party/catapult/
mkdir -p src/third_party/catapult/devil
cp $origin_dir/src/third_party/catapult/devil/BUILD.gn src/third_party/catapult/devil/
mkdir -p src/third_party/catapult/telemetry
cp $origin_dir/src/third_party/catapult/telemetry/BUILD.gn src/third_party/catapult/telemetry/
mkdir -p src/third_party/catapult/common
cp $origin_dir/src/third_party/catapult/common/. src/third_party/catapult/common/ -a
rm -rf src/third_party/catapult/common/py_vulcanize/third_party/rcssmin/tests
mkdir -p src/third_party/catapult/third_party/gsutil
cp $origin_dir/src/third_party/catapult/third_party/gsutil/BUILD.gn src/third_party/catapult/third_party/gsutil/
mkdir -p src/third_party/catapult/third_party/typ
cp $origin_dir/src/third_party/catapult/third_party/typ/BUILD.gn src/third_party/catapult/third_party/typ/
mkdir -p src/third_party/catapult/third_party/vinn
cp $origin_dir/src/third_party/catapult/third_party/vinn/BUILD.gn src/third_party/catapult/third_party/vinn/
mkdir -p src/chrome/android/modules
cp $origin_dir/src/chrome/android/channel.gni src/chrome/android/channel.gni
cp $origin_dir/src/chrome/android/modules/buildflags.gni src/chrome/android/modules/buildflags.gni
mkdir -p src/third_party/grpc/
cp $origin_dir/src/third_party/grpc/BUILD.gn src/third_party/grpc/BUILD.gn
mkdir -p src/third_party/llvm-build/Release+Asserts/bin/ 
cp $origin_dir/src/third_party/llvm-build/Release+Asserts/bin/clang src/third_party/llvm-build/Release+Asserts/bin/clang
mkdir -p src/third_party/SPIRV-Tools/src/test/fuzzers/
cp $origin_dir/src/third_party/SPIRV-Tools/src/test/fuzzers/BUILD.gn src/third_party/SPIRV-Tools/src/test/fuzzers/BUILD.gn
mkdir -p src/third_party/crashpad/crashpad/test/
cp $origin_dir/src/third_party/crashpad/crashpad/test/BUILD.gn src/third_party/crashpad/crashpad/test/BUILD.gn
mkdir -p src/third_party/zlib/contrib/tests/fuzzers
cp $origin_dir/src/third_party/zlib/contrib/tests/fuzzers/BUILD.gn src/third_party/zlib/contrib/tests/fuzzers/BUILD.gn

#Chromium84 more
#cp $origin_dir/src/media/fuchsia/mojom/fuchsia_media_resource_provider.typemap src/media/fuchsia/mojom/fuchsia_media_resource_provider.typemap
cp $origin_dir/src/media/fuchsia/mojom/fuchsia_media_resource_provider.mojom src/media/fuchsia/mojom/fuchsia_media_resource_provider.mojom
mkdir -p src/chromecast/media/mojom/
#cp $origin_dir/src/chromecast/media/mojom/decoder_config.typemap src/chromecast/media/mojom/decoder_config.typemap
cp $origin_dir/src/chromecast/media/mojom/media_types.mojom src/chromecast/media/mojom/media_types.mojom
mkdir -p src/third_party/dawn/src/tests/
cp $origin_dir/src/chrome/test/data/webui/namespace_rewrites.gni src/chrome/test/data/webui/namespace_rewrites.gni
mkdir -p src/tools/perf/core/perfetto_binary_roller/
cp $origin_dir/src/tools/perf/core/perfetto_binary_roller/BUILD.gn src/tools/perf/core/perfetto_binary_roller/BUILD.gn
mkdir -p src/chrome/test/data/webui/resources/
cp $origin_dir/src/chrome/test/data/webui/resources/BUILD.gn src/chrome/test/data/webui/resources/BUILD.gn
mkdir -p src/chrome/test/data/webui/settings/
cp $origin_dir/src/chrome/test/data/webui/settings/BUILD.gn src/chrome/test/data/webui/settings/BUILD.gn
cp -r $origin_dir/src/third_party/perfetto/src/trace_processor/importers/fuchsia src/third_party/perfetto/src/trace_processor/importers
