# Copyright 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from gpu_tests import pixel_test_pages
from gpu_tests import skia_gold_integration_test_base

from telemetry.util import image_util
from telemetry.util import rgba_color


class ExpectedColorTest(
    skia_gold_integration_test_base.SkiaGoldIntegrationTestBase):
  """Variant of a regular pixel test that only uses Gold to surface images.

  Instead of normal pixel comparison, correctness is verified by looking for
  expected colors in certain areas of a test image. This is meant for cases
  where a test produces images that are so noisy that it's impractical to use
  Gold normally for the test.
  """

  def RunActualGpuTest(self, options):
    raise NotImplementedError(
        'RunActualGpuTest must be overridden in a subclass')

  def GetGoldJsonKeys(self, page):
    keys = super(ExpectedColorTest, self).GetGoldJsonKeys(page)
    keys['expected_color_comment'] = (
        'This is an expected color test. Triaging in Gold will not affect test '
        'behavior.')
    return keys

  def _ValidateScreenshotSamplesWithSkiaGold(self, tab, page, screenshot,
                                             device_pixel_ratio):
    """Samples the given screenshot and verifies pixel color values.

    In case any of the samples do not match the expected color, it raises
    a Failure and uploads the image to Gold.

    Args:
      tab: the Telemetry Tab object that the test was run in.
      page: the GPU PixelTestPage object for the test.
      screenshot: the screenshot of the test page as a Telemetry Bitmap.
      device_pixel_ratio: the device pixel ratio for the test device as a float.
    """
    try:
      self._CompareScreenshotSamples(tab, screenshot, page, device_pixel_ratio)
    except Exception as comparison_exception:
      # An exception raised from self.fail() indicates a failure.
      image_name = self._UrlToImageName(page.name)
      # We want to report the screenshot comparison failure, not any failures
      # related to Gold.
      try:
        self._UploadTestResultToSkiaGold(image_name, screenshot, page)
      except Exception as gold_exception:  # pylint: disable=broad-except
        logging.error(str(gold_exception))
      # TODO(https://crbug.com/1043129): Switch this to just "raise" once these
      # tests are run with Python 3. Python 2's behavior with nested try/excepts
      # is weird and ends up re-raising the exception raised by
      # _UploadTestResultToSkiaGold instead of the one by
      # _CompareScreenshotSamples. See https://stackoverflow.com/q/28698622.
      raise comparison_exception

  def _CompareScreenshotSamples(self, tab, screenshot, page,
                                device_pixel_ratio):
    """Checks a screenshot for expected colors.

    Args:
      tab: the Telemetry Tab object that the test was run in.
      screenshot: the screenshot of the test page as a Telemetry Bitmap.
      page: the GPU PixelTestPage object for the test.
      device_pixel_ratio: the device pixel ratio for the test device as a float.

    Raises:
      AssertionError if the check fails for some reason.
    """

    def _CompareScreenshotWithExpectation(expectation):
      """Compares a portion of the screenshot to the given expectation.

      Fails the test if a the screenshot does not match within the tolerance.

      Args:
        expectation: A dict defining an expected color region. It must contain
            'location', 'size', and 'color' keys. See pixel_test_pages.py for
            examples.
      """
      location = expectation["location"]
      size = expectation["size"]
      x0 = int(location[0] * device_pixel_ratio)
      x1 = int((location[0] + size[0]) * device_pixel_ratio)
      y0 = int(location[1] * device_pixel_ratio)
      y1 = int((location[1] + size[1]) * device_pixel_ratio)
      for x in range(x0, x1):
        for y in range(y0, y1):
          if (x < 0 or y < 0 or x >= image_util.Width(screenshot)
              or y >= image_util.Height(screenshot)):
            self.fail(('Expected pixel location [%d, %d] is out of range on ' +
                       '[%d, %d] image') % (x, y, image_util.Width(screenshot),
                                            image_util.Height(screenshot)))

          actual_color = image_util.GetPixelColor(screenshot, x, y)
          expected_color = rgba_color.RgbaColor(
              expectation["color"][0], expectation["color"][1],
              expectation["color"][2],
              expectation["color"][3] if len(expectation["color"]) > 3 else 255)
          if not actual_color.IsEqual(expected_color, tolerance):
            self.fail('Expected pixel at ' + str(location) +
                      ' (actual pixel (' + str(x) + ', ' + str(y) + ')) ' +
                      ' to be ' + str(expectation["color"]) + " but got [" +
                      str(actual_color.r) + ", " + str(actual_color.g) + ", " +
                      str(actual_color.b) + ", " + str(actual_color.a) + "]")

    expected_colors = page.expected_colors
    tolerance = page.tolerance
    test_machine_name = self.GetParsedCommandLineOptions().test_machine_name

    # First scan through the expected_colors and see if there are any scale
    # factor overrides that would preempt the device pixel ratio. This
    # is mainly a workaround for complex tests like the Maps test.
    for expectation in expected_colors:
      if 'scale_factor_overrides' in expectation:
        for override in expectation['scale_factor_overrides']:
          # Require exact matches to avoid confusion, because some
          # machine models and names might be subsets of others
          # (e.g. Nexus 5 vs Nexus 5X).
          if ('device_type' in override
              and (tab.browser.platform.GetDeviceTypeName() ==
                   override['device_type'])):
            logging.warning('Overriding device_pixel_ratio ' +
                            str(device_pixel_ratio) + ' with scale factor ' +
                            str(override['scale_factor']) +
                            ' for device type ' + override['device_type'])
            device_pixel_ratio = override['scale_factor']
            break
          if (test_machine_name and 'machine_name' in override
              and override["machine_name"] == test_machine_name):
            logging.warning('Overriding device_pixel_ratio ' +
                            str(device_pixel_ratio) + ' with scale factor ' +
                            str(override['scale_factor']) +
                            ' for machine name ' + test_machine_name)
            device_pixel_ratio = override['scale_factor']
            break
        # Only support one "scale_factor_overrides" in the expectation format.
        break
    for expectation in expected_colors:
      if "scale_factor_overrides" in expectation:
        continue
      _CompareScreenshotWithExpectation(expectation)


class ExpectedColorPixelTestPage(pixel_test_pages.PixelTestPage):
  """Extension of PixelTestPage with expected color information."""

  def __init__(self, expected_colors, tolerance=2, *args, **kwargs):
    super(ExpectedColorPixelTestPage, self).__init__(*args, **kwargs)
    # The expected colors can be specified as a list of dictionaries. The format
    # is only defined by contract with _CompareScreenshotSamples in
    # expected_color_test.py.
    self.expected_colors = expected_colors
    # The tolerance when comparing against the reference image.
    self.tolerance = tolerance
