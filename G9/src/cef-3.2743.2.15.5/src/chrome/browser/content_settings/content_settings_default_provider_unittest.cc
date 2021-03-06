// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/message_loop/message_loop.h"
#include "chrome/browser/content_settings/content_settings_mock_observer.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/content_settings_default_provider.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/website_settings_info.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/content_settings/core/test/content_settings_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ::testing::_;

namespace content_settings {

class DefaultProviderTest : public testing::Test {
 public:
  DefaultProviderTest()
      : provider_(profile_.GetPrefs(), false) {
  }
  ~DefaultProviderTest() override { provider_.ShutdownOnUIThread(); }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  DefaultProvider provider_;
};

TEST_F(DefaultProviderTest, DefaultValues) {
  // Check setting defaults.
  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            TestUtils::GetContentSetting(&provider_, GURL(), GURL(),
                                         CONTENT_SETTINGS_TYPE_COOKIES,
                                         std::string(), false));
  provider_.SetWebsiteSetting(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_COOKIES,
      std::string(),
      new base::FundamentalValue(CONTENT_SETTING_BLOCK));
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            TestUtils::GetContentSetting(&provider_, GURL(), GURL(),
                                         CONTENT_SETTINGS_TYPE_COOKIES,
                                         std::string(), false));

  EXPECT_EQ(CONTENT_SETTING_ASK,
            TestUtils::GetContentSetting(&provider_, GURL(), GURL(),
                                         CONTENT_SETTINGS_TYPE_GEOLOCATION,
                                         std::string(), false));
  provider_.SetWebsiteSetting(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_GEOLOCATION,
      std::string(),
      new base::FundamentalValue(CONTENT_SETTING_BLOCK));
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            TestUtils::GetContentSetting(&provider_, GURL(), GURL(),
                                         CONTENT_SETTINGS_TYPE_GEOLOCATION,
                                         std::string(), false));

  std::unique_ptr<base::Value> value(TestUtils::GetContentSettingValue(
      &provider_, GURL("http://example.com/"), GURL("http://example.com/"),
      CONTENT_SETTINGS_TYPE_AUTO_SELECT_CERTIFICATE, std::string(), false));
  EXPECT_FALSE(value.get());
}

TEST_F(DefaultProviderTest, IgnoreNonDefaultSettings) {
  GURL primary_url("http://www.google.com");
  GURL secondary_url("http://www.google.com");

  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            TestUtils::GetContentSetting(&provider_, primary_url, secondary_url,
                                         CONTENT_SETTINGS_TYPE_COOKIES,
                                         std::string(), false));
  std::unique_ptr<base::Value> value(
      new base::FundamentalValue(CONTENT_SETTING_BLOCK));
  bool owned = provider_.SetWebsiteSetting(
      ContentSettingsPattern::FromURL(primary_url),
      ContentSettingsPattern::FromURL(secondary_url),
      CONTENT_SETTINGS_TYPE_COOKIES,
      std::string(),
      value.get());
  EXPECT_FALSE(owned);
  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            TestUtils::GetContentSetting(&provider_, primary_url, secondary_url,
                                         CONTENT_SETTINGS_TYPE_COOKIES,
                                         std::string(), false));
}

TEST_F(DefaultProviderTest, Observer) {
  MockObserver mock_observer;
  EXPECT_CALL(mock_observer,
              OnContentSettingChanged(
                  _, _, CONTENT_SETTINGS_TYPE_IMAGES, ""));
  provider_.AddObserver(&mock_observer);
  provider_.SetWebsiteSetting(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_IMAGES,
      std::string(),
      new base::FundamentalValue(CONTENT_SETTING_BLOCK));

  EXPECT_CALL(mock_observer,
              OnContentSettingChanged(
                  _, _, CONTENT_SETTINGS_TYPE_GEOLOCATION, ""));
  provider_.SetWebsiteSetting(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_GEOLOCATION,
      std::string(),
      new base::FundamentalValue(CONTENT_SETTING_BLOCK));
}


TEST_F(DefaultProviderTest, ObservePref) {
  PrefService* prefs = profile_.GetPrefs();

  provider_.SetWebsiteSetting(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_COOKIES,
      std::string(),
      new base::FundamentalValue(CONTENT_SETTING_BLOCK));
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            TestUtils::GetContentSetting(&provider_, GURL(), GURL(),
                                         CONTENT_SETTINGS_TYPE_COOKIES,
                                         std::string(), false));
  const WebsiteSettingsInfo* info = WebsiteSettingsRegistry::GetInstance()->Get(
      CONTENT_SETTINGS_TYPE_COOKIES);
  // Clearing the backing pref should also clear the internal cache.
  prefs->ClearPref(info->default_value_pref_name());
  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            TestUtils::GetContentSetting(&provider_, GURL(), GURL(),
                                         CONTENT_SETTINGS_TYPE_COOKIES,
                                         std::string(), false));
  // Reseting the pref to its previous value should update the cache.
  prefs->SetInteger(info->default_value_pref_name(), CONTENT_SETTING_BLOCK);
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            TestUtils::GetContentSetting(&provider_, GURL(), GURL(),
                                         CONTENT_SETTINGS_TYPE_COOKIES,
                                         std::string(), false));
}

TEST_F(DefaultProviderTest, OffTheRecord) {
  DefaultProvider otr_provider(profile_.GetPrefs(), true /* incognito */);

  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            TestUtils::GetContentSetting(
                &provider_, GURL(), GURL(), CONTENT_SETTINGS_TYPE_COOKIES,
                std::string(), false /* include_incognito */));
  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            TestUtils::GetContentSetting(
                &otr_provider, GURL(), GURL(), CONTENT_SETTINGS_TYPE_COOKIES,
                std::string(), true /* include_incognito */));

  // Changing content settings on the main provider should also affect the
  // incognito map.
  provider_.SetWebsiteSetting(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_COOKIES,
      std::string(),
      new base::FundamentalValue(CONTENT_SETTING_BLOCK));
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            TestUtils::GetContentSetting(
                &provider_, GURL(), GURL(), CONTENT_SETTINGS_TYPE_COOKIES,
                std::string(), false /* include_incognito */));

  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            TestUtils::GetContentSetting(
                &otr_provider, GURL(), GURL(), CONTENT_SETTINGS_TYPE_COOKIES,
                std::string(), true /* include_incognito */));

  // Changing content settings on the incognito provider should be ignored.
  std::unique_ptr<base::Value> value(
      new base::FundamentalValue(CONTENT_SETTING_ALLOW));
  bool owned = otr_provider.SetWebsiteSetting(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_COOKIES,
      std::string(),
      value.release());
  EXPECT_TRUE(owned);
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            TestUtils::GetContentSetting(
                &provider_, GURL(), GURL(), CONTENT_SETTINGS_TYPE_COOKIES,
                std::string(), false /* include_incognito */));

  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            TestUtils::GetContentSetting(
                &otr_provider, GURL(), GURL(), CONTENT_SETTINGS_TYPE_COOKIES,
                std::string(), true /* include_incognito */));

  // Check that new OTR DefaultProviders also inherit the correct value.
  DefaultProvider otr_provider2(profile_.GetPrefs(), true /* incognito */);
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            TestUtils::GetContentSetting(
                &otr_provider2, GURL(), GURL(), CONTENT_SETTINGS_TYPE_COOKIES,
                std::string(), true /* include_incognito */));

  otr_provider.ShutdownOnUIThread();
  otr_provider2.ShutdownOnUIThread();
}

}  // namespace content_settings
