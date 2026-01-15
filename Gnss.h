#pragma once
#include <aidl/android/hardware/gnss/BnGnss.h>
#include <aidl/android/hardware/gnss/IGnssCallback.h>
#include "NmeaReader.h"
#include <memory>

namespace aidl::android::hardware::gnss::implementation {

class Gnss : public BnGnss {
public:
    Gnss();
    ~Gnss() override;
    ndk::ScopedAStatus setCallback(const std::shared_ptr<IGnssCallback>& callback) override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus start() override;
    ndk::ScopedAStatus stop() override;
    ndk::ScopedAStatus injectTime(int64_t, int64_t, int32_t) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus injectLocation(const GnssLocation&) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus injectBestLocation(const GnssLocation&) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus deleteAidingData(GnssAidingData) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus setPositionMode(const PositionModeOptions& options) override;
    ndk::ScopedAStatus startSvStatus() override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus stopSvStatus() override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus startNmea() override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus stopNmea() override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionPsds(std::shared_ptr<IGnssPsds>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionGnssConfiguration(std::shared_ptr<IGnssConfiguration>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionGnssMeasurement(std::shared_ptr<IGnssMeasurementInterface>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionGnssPowerIndication(std::shared_ptr<IGnssPowerIndication>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionGnssBatching(std::shared_ptr<IGnssBatching>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionGnssGeofence(std::shared_ptr<IGnssGeofence>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionGnssNavigationMessage(std::shared_ptr<IGnssNavigationMessageInterface>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionAGnss(std::shared_ptr<IAGnss>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionAGnssRil(std::shared_ptr<IAGnssRil>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionGnssDebug(std::shared_ptr<IGnssDebug>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionGnssVisibilityControl(std::shared_ptr<IGnssVisibilityControl>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionGnssAntennaInfo(std::shared_ptr<IGnssAntennaInfo>*) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus getExtensionMeasurementCorrections(std::shared_ptr<IMeasurementCorrectionsInterface>*) override { return ndk::ScopedAStatus::ok(); }

private:
    std::shared_ptr<IGnssCallback> mCallback;
    std::unique_ptr<NmeaReader> mNmeaReader;
    bool mIsActive = false;
    static constexpr char UART_DEVICE[] = "/dev/ttyAMA0";
};

}
