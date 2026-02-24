#pragma once

#include <aidl/android/hardware/gnss/BnGnss.h>
#include <aidl/android/hardware/gnss/IGnssCallback.h>
#include <aidl/android/hardware/gnss/GnssConstellationType.h>
#include <aidl/android/hardware/gnss/ElapsedRealtime.h>
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <map>

namespace aidl::android::hardware::gnss::implementation {

using ::aidl::android::hardware::gnss::GnssLocation;
using ::aidl::android::hardware::gnss::GnssConstellationType;
using ::aidl::android::hardware::gnss::ElapsedRealtime;
using GnssSvInfo = ::aidl::android::hardware::gnss::IGnssCallback::GnssSvInfo;
using GnssSvFlags = ::aidl::android::hardware::gnss::IGnssCallback::GnssSvFlags;

using LocationCallback = std::function<void(const GnssLocation&)>;
using NmeaCallback = std::function<void(int64_t, const std::string&)>;
using SvStatusCallback = std::function<void(const std::vector<GnssSvInfo>&)>;

class NmeaReader {
public:
    NmeaReader(const std::string& device, int baudRate,
               LocationCallback locationCb, NmeaCallback nmeaCb, SvStatusCallback svCb);
    ~NmeaReader();
    
    bool start();
    void stop();
    void setMinInterval(int32_t intervalMs);

private:
    void readerThreadFunc();
    bool openUart();
    void closeUart();
    
    void processNmeaSentence(const std::string& sentence);
    bool parseGGA(const std::string& sentence);
    bool parseRMC(const std::string& sentence);
    bool parseGSV(const std::string& sentence);
    bool parseGSA(const std::string& sentence);
    bool parseVTG(const std::string& sentence);
    
    double nmeaToDecimal(double nmeaCoord, char direction);
    bool validateChecksum(const std::string& sentence);
    std::vector<std::string> splitString(const std::string& str, char delimiter);
    int64_t getCurrentTimestampMs();
    GnssConstellationType getConstellationType(const std::string& talkerId, int svid);

    std::string mDevice;
    int mUartFd;
    
    std::thread mReaderThread;
    std::atomic<bool> mRunning;
    std::atomic<int32_t> mMinIntervalMs;
    
    int64_t mLastLocationReportMs;
    int64_t mLastSvReportMs;
    
    LocationCallback mLocationCallback;
    NmeaCallback mNmeaCallback;
    SvStatusCallback mSvStatusCallback;
    
    std::mutex mLocationMutex;
    GnssLocation mCurrentLocation;
    bool mHasValidFix;
    int mFixQuality;
    int mNumSatellites;
    
    std::mutex mSvMutex;
    std::map<int, GnssSvInfo> mSatellites;
    std::vector<int> mSatsUsedInFix;
    
    // NAPRAWA: Musi byc 4096, tak jak w pliku .cpp!
    static constexpr int READ_BUFFER_SIZE = 4096;
    static constexpr int NMEA_MAX_SIZE = 256;
    static constexpr int SV_REPORT_INTERVAL_MS = 1000;
};

}
