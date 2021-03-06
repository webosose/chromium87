# Copyright 2019 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from absl import flags
import os

from infra import ChromeEnterpriseTestCase
from chrome_ent_test.infra.core import before_all, category, environment, test


FLAGS = flags.FLAGS
flags.DEFINE_string(
    'enrollToken', None,
    'The enrollment token to use, it overwrites the default token')


@category("chrome_only")
@environment(file="../policy_test.asset.textpb")
class CloudManagementEnrollmentTokenTest(ChromeEnterpriseTestCase):
  """Test the CloudManagementEnrollmentToken policy:
  https://cloud.google.com/docs/chrome-enterprise/policies/?policy=CloudManagementEnrollmentToken."""

  @before_all
  def setup(self):
    self.InstallChrome('client2019')
    self.InstallWebDriver('client2019')

  @test
  def test_browser_enrolled_prod(self):
    token = FLAGS.enrollToken
    if token == None:
      path = "gs://%s/secrets/enrollToken" % self.gsbucket
      cmd = r'gsutil cat ' + path
      token = self.RunCommand('win2019-dc', cmd).rstrip()
    self.SetPolicy('win2019-dc', r'CloudManagementEnrollmentToken', token,
                   'String')
    self.RunCommand('client2019', 'gpupdate /force')

    local_dir = os.path.dirname(os.path.abspath(__file__))

    output = self.RunWebDriverTest(
        'client2019', os.path.join(local_dir, 'cloud_enrollment_webdriver.py'))
    # Verify CBCM status legend
    self.assertIn('Machine policies', output)
    self.assertIn('CLIENT2019', output)
    self.assertIn(token, output)
    self.assertIn('Policy cache OK', output)
