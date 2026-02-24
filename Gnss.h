/*
 * Copyright (C) 2024 Custom GNSS HAL for Raspberry Pi 5
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/gnss/BnGnss.h>
#include <aidl/android/hardware/gnss/IGnssCallback.h>
#include <aidl/android/hardware/gnss/visibility_control/IGnssVisibilityControl.h>
#include <aidl/android/hardware/gnss/measurement_corrections/IMeasurementCorrectionsInterface.h>

#include <atomic>
#include <mutex>
#include <memory>
#include <vector>

#include "GnssConfiguration.h"
#include "GnssPowerIndication.h"
#include "GnssMeasurementInterface.h"
#include "NmeaReader.h"

namespace aidl::android::hardware::gnss::implementation {

using GnssSvInfo = ::aidl::android::hardware::gnss::IGnssCallback::GnssSvInfo;

class Gnss : public BnGnss {
public:
    Gnss();
    ~Gnss();

    // Core interface
    ndk::ScopedAStatus setCallback(const std::shared_ptr<IGnssCallback>& callback) override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus start() override;
    ndk::ScopedAStatus stop() override;

    // Position mode
    ndk::ScopedAStatus setPositionMode(const PositionModeOptions& options) override;

    // Data injection
    ndk::ScopedAStatus injectTime(int64_t timeMs, int64_t timeReferenceMs, int32_t uncertaintyMs) override;
    ndk::ScopedAStatus injectLocation(const GnssLocation& location) override;
    ndk::ScopedAStatus injectBestLocation(const GnssLocation& location) override;
    ndk::ScopedAStatus deleteAidingData(GnssAidingData aidingDataFlags) override;

    // SV status and NMEA
    ndk::ScopedAStatus startSvStatus() override;
    ndk::ScopedAStatus stopSvStatus() override;
    ndk::ScopedAStatus startNmea() override;
    ndk::ScopedAStatus stopNmea() override;

    // Extensions
    ndk::ScopedAStatus getExtensionPsds(std::shared_ptr<IGnssPsds>* _aidl_return) override;
    ndk::ScopedAStatus getExtensionGnssConfiguration(std::shared_ptr<IGnssConfiguration>* _aidl_return) override;
    ndk::ScopedAStatus getExtensionGnssPowerIndication(std::shared_ptr<IGnssPowerIndication>* _aidl_return) override;
    ndk::ScopedAStatus getExtensionGnssMeasurement(std::shared_ptr<IGnssMeasurementInterface>* _aidl_return) override;
    ndk::ScopedAStatus getExtensionGnssBatching(std::shared_ptr<IGnssBatching>* _aidl_return) override;
    ndk::ScopedAStatus getExtensionGnssGeofence(std::shared_ptr<IGnssGeofence>* _aidl_return) override;
    ndk::ScopedAStatus getExtensionGnssNavigationMessage(std::shared_ptr<IGnssNavigationMessageInterface>* _aidl_return) override;
    ndk::ScopedAStatus getExtensionAGnss(std::shared_ptr<IAGnss>* _aidl_return) override;
    ndk::ScopedAStatus getExtensionAGnssRil(std::shared_ptr<IAGnssRil>* _aidl_return) override;
    ndk::ScopedAStatus getExtensionGnssDebug(std::shared_ptr<IGnssDebug>* _aidl_return) override;
    
    // Fixed types for namespaces
    ndk::ScopedAStatus getExtensionGnssVisibilityControl(
        std::shared_ptr<visibility_control::IGnssVisibilityControl>* _aidl_return) override;
        
    ndk::ScopedAStatus getExtensionGnssAntennaInfo(std::shared_ptr<IGnssAntennaInfo>* _aidl_return) override;
    
    // Fixed method name and type
    ndk::ScopedAStatus getExtensionMeasurementCorrections(
        std::shared_ptr<measurement_corrections::IMeasurementCorrectionsInterface>* _aidl_return) override;

private:
    void reportLocation(const GnssLocation& location);
    void reportNmea(int64_t timestamp, const std::string& nmea);
    void reportSvStatus(const std::vector<GnssSvInfo>& svInfoList);

    std::shared_ptr<IGnssCallback> mCallback;
    std::shared_ptr<GnssConfiguration> mGnssConfiguration;
    std::shared_ptr<GnssPowerIndication> mGnssPowerIndication;
    std::shared_ptr<GnssMeasurementInterface> mGnssMeasurement;
    std::unique_ptr<NmeaReader> mNmeaReader;
    std::mutex mMutex;
    std::atomic<bool> mIsActive;
    std::atomic<bool> mReportSvStatus;
    std::atomic<bool> mReportNmea;
    int32_t mMinIntervalMs;
};

}  // namespace aidl::android::hardware::gnss::implementation
