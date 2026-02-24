#pragma once
#include <aidl/android/hardware/gnss/BnGnssConfiguration.h>
namespace aidl::android::hardware::gnss::implementation {
class GnssConfiguration : public BnGnssConfiguration {
public:
    ndk::ScopedAStatus setSuplVersion(int32_t) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus setSuplMode(int32_t) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus setLppProfile(int32_t) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus setGlonassPositioningProtocol(int32_t) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus setEmergencySuplPdn(bool) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus setEsExtensionSec(int32_t) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus setBlocklist(const std::vector<BlocklistedSource>&) override { return ndk::ScopedAStatus::ok(); }
};
}
