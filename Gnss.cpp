#define LOG_TAG "GnssHal"
#include "Gnss.h"
#include <android-base/logging.h>

namespace aidl::android::hardware::gnss::implementation {

Gnss::Gnss() { LOG(INFO) << "GNSS HAL initialized"; }
Gnss::~Gnss() { if (mNmeaReader) mNmeaReader->stop(); }

ndk::ScopedAStatus Gnss::setCallback(const std::shared_ptr<IGnssCallback>& callback) {
    mCallback = callback;
    if (mCallback) {
        mCallback->gnssSetCapabilitiesCb(IGnssCallback::CAPABILITY_SCHEDULING);
        IGnssCallback::GnssSystemInfo info; info.yearOfHw = 2024; info.name = "RPi5";
        mCallback->gnssSetSystemInfoCb(info);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::start() {
    if (mIsActive) return ndk::ScopedAStatus::ok();
    mNmeaReader = std::make_unique<NmeaReader>(UART_DEVICE, 115200,
        [this](const GnssLocation& loc) { if (mCallback) mCallback->gnssLocationCb(loc); },
        [this](int64_t ts, const std::string& nmea) { if (mCallback) mCallback->gnssNmeaCb(ts, nmea); },
        [this](const std::vector<IGnssCallback::GnssSvInfo>& sv) { if (mCallback) mCallback->gnssSvStatusCb(sv); }
    );
    if (!mNmeaReader->start()) return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    mIsActive = true;
    if (mCallback) mCallback->gnssStatusCb(IGnssCallback::GnssStatusValue::SESSION_BEGIN);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::stop() {
    if (!mIsActive) return ndk::ScopedAStatus::ok();
    if (mNmeaReader) { mNmeaReader->stop(); mNmeaReader.reset(); }
    mIsActive = false;
    if (mCallback) mCallback->gnssStatusCb(IGnssCallback::GnssStatusValue::SESSION_END);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::close() { stop(); mCallback = nullptr; return ndk::ScopedAStatus::ok(); }

ndk::ScopedAStatus Gnss::setPositionMode(const PositionModeOptions& options) {
    if (mNmeaReader) mNmeaReader->setMinInterval(options.minIntervalMs);
    return ndk::ScopedAStatus::ok();
}

}
