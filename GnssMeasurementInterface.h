/*
 * Copyright (C) 2024 Custom GNSS HAL for Raspberry Pi 5
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/gnss/BnGnssMeasurementInterface.h>

namespace aidl::android::hardware::gnss::implementation {

class GnssMeasurementInterface : public BnGnssMeasurementInterface {
public:
    ndk::ScopedAStatus setCallback(
        const std::shared_ptr<IGnssMeasurementCallback>&, bool, bool) override {
        return ndk::ScopedAStatus::ok();
    }
    
    // New method required by AIDL V3+
    ndk::ScopedAStatus setCallbackWithOptions(
        const std::shared_ptr<IGnssMeasurementCallback>&, const Options&) override {
        return ndk::ScopedAStatus::ok();
    }
    
    ndk::ScopedAStatus close() override { return ndk::ScopedAStatus::ok(); }
};

}  // namespace aidl::android::hardware::gnss::implementation
