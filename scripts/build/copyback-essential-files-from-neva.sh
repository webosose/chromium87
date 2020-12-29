#!/bin/sh

mkdir -p src/chromecast/common/mojom
mkdir -p src/chrome/test/data/webui
mkdir -p src/chrome/test/data/media/engagement/preload
mkdir -p src/content/test/data
mkdir -p src/tools/perf
mkdir -p src/tools/traffic_annotation
mkdir -p src/components/test

cp ../chromium-neva-with-gclient-run/src/chromecast/common/mojom/. src/chromecast/common/mojom/ -a
cp ../chromium-neva-with-gclient-run/src/chrome/test/data/webui/i18n_process_css_test.html src/chrome/test/data/webui/i18n_process_css_test.html
cp ../chromium-neva-with-gclient-run/src/chrome/test/data/webui/web_ui_test.mojom src/chrome/test/data/webui/web_ui_test.mojom
cp ../chromium-neva-with-gclient-run/src/chrome/test/data/BUILD.gn src/chrome/test/data/BUILD.gn
cp ../chromium-neva-with-gclient-run/src/chrome/test/data/webui/BUILD.gn src/chrome/test/data/webui/BUILD.gn
cp ../chromium-neva-with-gclient-run/src/chrome/test/data/media/engagement/preload/BUILD.gn src/chrome/test/data/media/engagement/preload/BUILD.gn
cp ../chromium-neva-with-gclient-run/src/content/test/data/lite_js_test.mojom src/content/test/data/lite_js_test.mojom
cp ../chromium-neva-with-gclient-run/src/content/test/data/mojo_layouttest_test.mojom src/content/test/data/mojo_layouttest_test.mojom
cp -a ../chromium-neva-with-gclient-run/src/tools/perf/clear_system_cache src/tools/perf/clear_system_cache
cp -a ../chromium-neva-with-gclient-run/src/tools/perf/chrome_telemetry_build src/tools/perf/chrome_telemetry_build
cp ../chromium-neva-with-gclient-run/src/tools/perf/BUILD.gn src/tools/perf/BUILD.gn
cp -a ../chromium-neva-with-gclient-run/src/tools/traffic_annotation/auditor src/tools/traffic_annotation/auditor
cp ../chromium-neva-with-gclient-run/src/components/test/BUILD.gn src/components/test/BUILD.gn
