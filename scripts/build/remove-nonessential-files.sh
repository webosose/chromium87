#!/bin/sh

files_giving_yocto_warning="
      ./src/third_party/perfetto/test/data
      ./src/third_party/perfetto/ui/src/gen
      ./src/third_party/skia/tools/gyp
      ./src/buildtools/third_party/libc++/trunk/test/std/input.output/filesystems/Inputs/static_test_env/bad_symlink"

all_git_dirs=`find . -type d -name .git`
all_cipd_dirs_files=`find . -iname ".cipd*"`

files_over_github_size_limit=`find . -size +99M`

deprecated_nonessential_dirs="
      ./src/third_party/WebKit/LayoutTests/
      ./src/third_party/android_async_task/
      ./src/third_party/apple_sample_code
      ./src/third_party/javax_inject
      ./src/tools/gyp"

nonessential_dirs_removed_earlier="
      ./src/android_webview/tools/system_webview_shell/layout_tests/
      ./src/build/fuchsia/layout_test_proxy/
      ./src/chrome/test/data/extensions/perf_tests/
      ./src/chrome/test/data/printing/layout_tests/
      ./src/ios/
      ./src/native_client*
      ./src/third_party/blink/manual_tests
      ./src/third_party/blink/perf_tests
      ./src/third_party/angle/src/tests/perf_tests
      ./src/third_party/catapult/telemetry/third_party/WebKit/PerformanceTests
      ./src/third_party/openh264/src/autotest/performanceTest
      ./src/third_party/hunspell_dictionaries
      ./src/third_party/android_data_chart/
      ./src/third_party/android_deps/
      ./src/third_party/android_media/
      ./src/third_party/android_opengl/
      ./src/third_party/android_platform/
      ./src/third_party/android_protobuf/
      ./src/third_party/android_sdk/
      ./src/third_party/android_support_test_runner/
      ./src/third_party/android_swipe_refresh/
      ./src/third_party/android_system_sdk/
      ./src/third_party/apk-patch-size-estimator
      ./src/third_party/arcore-android-sdk
      ./src/third_party/fuchsia-sdk
      ./src/third_party/gvr-android-keyboard
      ./src/third_party/gvr-android-sdk
      ./src/third_party/win_build_output
      ./src/android_webview
      ./src/chromecast
      ./src/build/linux/debian_sid_*
      ./src/webrunner
      ./src/tools/win/
      ./src/third_party/webgl/src/conformance-suites
      ./src/third_party/webgl/src/sdk/tests"

fuchsia_paths=`find . -path '*fuchsia*' -type d`

essential_files="
      ./src/chromecast/common/mojom/
      ./src/third_party/apple_apsl/
      ./src/chrome/test/data/webui/i18n_process_css_test.html
      ./src/chrome/test/data/webui/web_ui_test.mojom
      ./src/v8/test/torque/test-torque.tq
      ./src/v8/test/BUILD.gn
      ./src/chrome/test/data/BUILD.gn
      ./src/v8/test/cctest/BUILD.gn
      ./src/chrome/test/data/webui/BUILD.gn
      ./src/chrome/test/data/media/engagement/preload/BUILD.gn
      ./src/v8/test
      ./src/content/test/data/lite_js_test.mojom
      ./src/content/test/data/mojo_layouttest_test.mojom
      ./src/tools/perf/clear_system_cache/
      ./src/tools/perf/chrome_telemetry_build/
      ./src/tools/perf/BUILD.gn
      ./src/tools/traffic_annotation/auditor/
      ./src/components/test/BUILD.gn
      "

new_nonessential_dirs="
    ./src/third_party/blink/web_tests/
    ./src/third_party/hunspell/tests
    ./src/third_party/liblouis/src/tests/harness
    ./src/third_party/sqlite/src/test/
    ./src/third_party/swiftshader/third_party/llvm-7.0/llvm/test/
    ./src/third_party/xdg-utils/tests
    ./src/third_party/yasm/source/patched-yasm/modules/arch/x86/tests
    ./src/third_party/yasm/source/patched-yasm/modules/dbgfmts/dwarf2/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/bin/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/coff/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/elf/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/macho/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/rdf/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/win32/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/win64/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/xdf/tests
    ./src/third_party/angle/third_party/deqp/src/external/vulkancts/mustpass
    ./src/third_party/angle/third_party/deqp/src/android
    ./src/third_party/catapult/third_party/vinn/third_party/v8/mac
    ./src/third_party/catapult/third_party/vinn/third_party/v8/win
    ./src/third_party/angle/third_party/deqp
    ./src/third_party/sqlite/sqlite-src-3250200/test
    ./src/third_party/sqlite/sqlite-src-3250300/test
    ./src/tools/perf
    ./src/tools/traffic_annotation
    ./src/components/test
    ./src/components/policy/android
    ./src/third_party/chromite/
    "
nonessential_test_dirs=" 
    ./src/chrome/test/data
    ./src/content/test/data
    ./src/courgette/testdata
    ./src/extensions/test/data
    ./src/media/test/data
    ./src/third_party/breakpad/breakpad/src/processor/testdata
    ./src/third_party/catapult/tracing/test_data
"

new_nonessential_found_in_chr79="
    ./src/third_party/swiftshader/tests
    ./src/third_party/sqlite/sqlite-src-3290000/test
    ./src/third_party/sqlite/sqlite-src-3300100/test
    ./src/third_party/sqlite/patched/test
    ./src/third_party/catapult
    ./src/buildtools/third_party/libc++/trunk/test
    ./src/chrome/android
    ./src/third_party/swiftshader/third_party/llvm-7.0/llvm/test/
    ./src/third_party/swiftshader/third_party/llvm-7.0/llvm/unittests/
    ./src/third_party/swiftshader/third_party/llvm-7.0/llvm/docs/
    ./src/third_party/swiftshader/third_party/llvm-7.0/llvm/examples/
    ./src/third_party/swiftshader/third_party/llvm-7.0/llvm/utils/lit/tests/
    ./src/third_party/swiftshader/third_party/llvm-7.0/llvm/utils/unittest/
    ./src/third_party/grpc/
    ./src/depot_tools/testing_support
    ./src/depot_tools/tests
    ./src/depot_tools/win_toolchain
    ./src/depot_tools/external_bin
    ./src/depot_tools/recipes/recipe_modules/bot_update/
    ./src/depot_tools/recipes/recipe_modules/gerrit/
    ./src/depot_tools/recipes/recipe_modules/gsutil/
    ./src/depot_tools/recipes/recipe_modules/infra_paths/
    ./src/depot_tools/recipes/recipe_modules/osx_sdk/
    ./src/depot_tools/recipes/recipe_modules/presubmit/
    ./src/depot_tools/recipes/recipe_modules/windows_sdk/
    ./src/depot_tools/man/
"

essential_found_in_chr79="
    ./src/third_party/depot_tools/
    ./src/third_party/llvm-build/
"

essential_found_in_chr87="
    ./src/tools/android/
    ./src/chrome/common/extensions/docs/server2/BUILD.gn
    ./src/chrome/test/data/cast/BUILD.gn
    ./src/chrome/test/data/pdf/BUILD.gn
    ./src/chrome/test/data/webui/
"

#new_nonessential_third_party_test_in_chr79=`find src/third_party/ -type d -not -path "src/third_party/blink/*" -not -path "src/third_party/webrtc/*" -not -path "src/third_party/boringssl/*" -iname test`

#new_nonessential_third_party_tests_in_chr79=`find src/third_party/ -type d -not -path "src/third_party/blink/*" -not -path "src/third_party/webrtc/*" -not -path "src/third_party/boringssl/*" -not -path "src/third_party/angle/*" -not -path "src/third_party/swiftshader/*" -iname tests`

new_nonessential_depot_tools_files="
    ./depot_tools/bootstrap-3.8.0.chromium.8_bin
    ./depot_tools/testing_support
    ./depot_tools/tests
    ./depot_tools/win_toolchain
    ./depot_tools/external_bin
    ./depot_tools/recipes/recipe_modules/bot_update/
    ./depot_tools/recipes/recipe_modules/gerrit/
    ./depot_tools/recipes/recipe_modules/gsutil/
    ./depot_tools/recipes/recipe_modules/infra_paths/
    ./depot_tools/recipes/recipe_modules/osx_sdk/
    ./depot_tools/recipes/recipe_modules/presubmit/
    ./depot_tools/recipes/recipe_modules/windows_sdk/
    ./depot_tools/man/
"

v8_dir="./src/v8"
new_paths="
"

#rm -Rf $new_paths
#rm -Rf $v8_dir $all_cipd_dirs_files $files_giving_yocto_warning $all_git_dirs $files_over_github_size_limit $nonessential_dirs_removed_earlier $new_nonessential_dirs $nonessential_test_dirs $fuchsia_paths $new_nonessential_found_in_chr79 $new_nonessential_third_party_test_in_chr79 $new_nonessential_third_party_tests_in_chr79 $new_nonessential_depot_tools_files
rm -Rf $v8_dir $all_cipd_dirs_files $files_giving_yocto_warning $all_git_dirs $files_over_github_size_limit $nonessential_dirs_removed_earlier $new_nonessential_dirs $nonessential_test_dirs $fuchsia_paths $new_nonessential_found_in_chr79 $new_nonessential_third_party_test_in_chr79 $new_nonessential_third_party_tests_in_chr79
#$1 $files_over_github_size_limit
#-rw-rw-r-- 1 lokesh lokesh 210M Jun  3 12:30 ./src/third_party/instrumented_libraries/binaries/msan-chained-origins-trusty.tgz
#-rw-rw-r-- 1 lokesh lokesh 188M Jun  3 12:30 ./src/third_party/instrumented_libraries/binaries/msan-no-origins-trusty.tgz
