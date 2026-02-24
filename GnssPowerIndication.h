#pragma once
#include <aidl/android/hardware/gnss/BnGnssPowerIndication.h>
namespace aidl::android::hardware::gnss::implementation {
class GnssPowerIndication : public BnGnssPowerIndication {
public:
    ndk::ScopedAStatus setCallback(const std::shared_ptr<IGnssPowerIndicationCallback>&) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus requestGnssPowerStats() override { return ndk::ScopedAStatus::ok(); }
};
}
