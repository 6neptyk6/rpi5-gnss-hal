/*
 * Copyright (C) 2024 Custom GNSS HAL for Raspberry Pi 5
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "GnssHalRpi5"

#include "Gnss.h"
#include <android-base/logging.h>

namespace aidl::android::hardware::gnss::implementation {

Gnss::Gnss()
    : mCallback(nullptr),
      mIsActive(false),
      mReportSvStatus(false),
      mReportNmea(false),
      mMinIntervalMs(1000) {
    LOG(INFO) << "GNSS HAL for Raspberry Pi 5 - Initializing";
    mGnssConfiguration = ndk::SharedRefBase::make<GnssConfiguration>();
    mGnssPowerIndication = ndk::SharedRefBase::make<GnssPowerIndication>();
    mGnssMeasurement = ndk::SharedRefBase::make<GnssMeasurementInterface>();

    mNmeaReader = std::make_unique<NmeaReader>(
        "/dev/ttyAMA0",
        115200,
        [this](const GnssLocation& loc) { reportLocation(loc); },
        [this](int64_t ts, const std::string& nmea) { reportNmea(ts, nmea); },
        [this](const std::vector<GnssSvInfo>& sv) { reportSvStatus(sv); }
    );
}

Gnss::~Gnss() {
    stop();
}

ndk::ScopedAStatus Gnss::setCallback(const std::shared_ptr<IGnssCallback>& callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    mCallback = callback;
    if (mCallback != nullptr) {
        int32_t capabilities =
            static_cast<int32_t>(IGnssCallback::CAPABILITY_SCHEDULING) |
            static_cast<int32_t>(IGnssCallback::CAPABILITY_SATELLITE_BLOCKLIST) |
            static_cast<int32_t>(IGnssCallback::CAPABILITY_SATELLITE_PVT) |
            static_cast<int32_t>(IGnssCallback::CAPABILITY_CORRELATION_VECTOR);

        mCallback->gnssSetCapabilitiesCb(capabilities);

        IGnssCallback::GnssSystemInfo systemInfo;
        systemInfo.yearOfHw = 2024;
        systemInfo.name = "Raspberry Pi 5 GNSS (LC29H)";
        mCallback->gnssSetSystemInfoCb(systemInfo);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::close() {
    stop();
    std::lock_guard<std::mutex> lock(mMutex);
    mCallback = nullptr;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::start() {
    LOG(INFO) << "Gnss::start() called";
    if (mIsActive.load()) {
        LOG(INFO) << "Already active";
        return ndk::ScopedAStatus::ok();
    }
    if (!mNmeaReader) {
        LOG(ERROR) << "mNmeaReader is NULL!";
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }
    LOG(INFO) << "Calling NmeaReader->start()...";
    if (mNmeaReader->start()) {
        LOG(INFO) << "NmeaReader started OK";
        mIsActive.store(true);
        std::lock_guard<std::mutex> lock(mMutex);
        if (mCallback) {
            mCallback->gnssStatusCb(IGnssCallback::GnssStatusValue::SESSION_BEGIN);
            mCallback->gnssStatusCb(IGnssCallback::GnssStatusValue::ENGINE_ON);
        }
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << "NmeaReader->start() FAILED!";
    return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
}

ndk::ScopedAStatus Gnss::stop() {
    if (!mIsActive.load()) return ndk::ScopedAStatus::ok();
    if (mNmeaReader) mNmeaReader->stop();
    mIsActive.store(false);
    std::lock_guard<std::mutex> lock(mMutex);
    if (mCallback) {
        mCallback->gnssStatusCb(IGnssCallback::GnssStatusValue::ENGINE_OFF);
        mCallback->gnssStatusCb(IGnssCallback::GnssStatusValue::SESSION_END);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::setPositionMode(const PositionModeOptions& options) {
    mMinIntervalMs = options.minIntervalMs;
    if (mNmeaReader) mNmeaReader->setMinInterval(options.minIntervalMs);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::startSvStatus() { mReportSvStatus.store(true); return ndk::ScopedAStatus::ok(); }
ndk::ScopedAStatus Gnss::stopSvStatus() { mReportSvStatus.store(false); return ndk::ScopedAStatus::ok(); }
ndk::ScopedAStatus Gnss::startNmea() { mReportNmea.store(true); return ndk::ScopedAStatus::ok(); }
ndk::ScopedAStatus Gnss::stopNmea() { mReportNmea.store(false); return ndk::ScopedAStatus::ok(); }

ndk::ScopedAStatus Gnss::injectTime(int64_t, int64_t, int32_t) { return ndk::ScopedAStatus::ok(); }
ndk::ScopedAStatus Gnss::injectLocation(const GnssLocation&) { return ndk::ScopedAStatus::ok(); }
ndk::ScopedAStatus Gnss::injectBestLocation(const GnssLocation&) { return ndk::ScopedAStatus::ok(); }
ndk::ScopedAStatus Gnss::deleteAidingData(GnssAidingData) { return ndk::ScopedAStatus::ok(); }

ndk::ScopedAStatus Gnss::getExtensionGnssConfiguration(std::shared_ptr<IGnssConfiguration>* r) {
    *r = mGnssConfiguration;
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus Gnss::getExtensionGnssPowerIndication(std::shared_ptr<IGnssPowerIndication>* r) {
    *r = mGnssPowerIndication;
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus Gnss::getExtensionGnssMeasurement(std::shared_ptr<IGnssMeasurementInterface>* r) {
    *r = mGnssMeasurement;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::getExtensionPsds(std::shared_ptr<IGnssPsds>* r) {
    *r = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Gnss::getExtensionGnssBatching(std::shared_ptr<IGnssBatching>* r) {
    *r = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Gnss::getExtensionGnssGeofence(std::shared_ptr<IGnssGeofence>* r) {
    *r = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Gnss::getExtensionGnssNavigationMessage(std::shared_ptr<IGnssNavigationMessageInterface>* r) {
    *r = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Gnss::getExtensionAGnss(std::shared_ptr<IAGnss>* r) {
    *r = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Gnss::getExtensionAGnssRil(std::shared_ptr<IAGnssRil>* r) {
    *r = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Gnss::getExtensionGnssDebug(std::shared_ptr<IGnssDebug>* r) {
    *r = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Gnss::getExtensionGnssAntennaInfo(std::shared_ptr<IGnssAntennaInfo>* r) {
    *r = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Gnss::getExtensionGnssVisibilityControl(std::shared_ptr<visibility_control::IGnssVisibilityControl>* r) {
    *r = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Gnss::getExtensionMeasurementCorrections(std::shared_ptr<measurement_corrections::IMeasurementCorrectionsInterface>* r) {
    *r = nullptr;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

void Gnss::reportLocation(const GnssLocation& location) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mCallback && mIsActive.load()) mCallback->gnssLocationCb(location);
}
void Gnss::reportNmea(int64_t timestamp, const std::string& nmea) {
    if (!mReportNmea.load()) return;
    std::lock_guard<std::mutex> lock(mMutex);
    if (mCallback && mIsActive.load()) mCallback->gnssNmeaCb(timestamp, nmea);
}
void Gnss::reportSvStatus(const std::vector<GnssSvInfo>& svInfoList) {
    if (!mReportSvStatus.load()) return;
    std::lock_guard<std::mutex> lock(mMutex);
    if (mCallback && mIsActive.load()) mCallback->gnssSvStatusCb(svInfoList);
}

}
