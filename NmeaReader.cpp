/*
 * Copyright (C) 2024 Custom GNSS HAL for Raspberry Pi 5
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "GnssNmeaReader"

#include "NmeaReader.h"
#include <android-base/logging.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <errno.h>

namespace aidl::android::hardware::gnss::implementation {

static constexpr int READ_BUFFER_SIZE = 4096;

NmeaReader::NmeaReader(const std::string& device, int,
                       LocationCallback locationCb, NmeaCallback nmeaCb, SvStatusCallback svCb)
    : mDevice(device), mUartFd(-1), mRunning(false), mMinIntervalMs(1000),
      mLastLocationReportMs(0), mLastSvReportMs(0),
      mLocationCallback(std::move(locationCb)), mNmeaCallback(std::move(nmeaCb)),
      mSvStatusCallback(std::move(svCb)), mHasValidFix(false), mFixQuality(0), mNumSatellites(0) {
    memset(&mCurrentLocation, 0, sizeof(mCurrentLocation));
    LOG(INFO) << "NmeaReader BLOCKING FIX created";
}

NmeaReader::~NmeaReader() { stop(); }

bool NmeaReader::openUart() {
    // Tryb blokujacy (brak O_NONBLOCK)
    mUartFd = open(mDevice.c_str(), O_RDWR | O_NOCTTY);
    if (mUartFd < 0) {
        LOG(ERROR) << "Failed to open UART: " << strerror(errno);
        return false;
    }
    
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(mUartFd, &tty) != 0) { close(mUartFd); return false; }
    
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;
    
    // Czekaj na min. 1 znak
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;
    
    if (tcsetattr(mUartFd, TCSANOW, &tty) != 0) { close(mUartFd); return false; }
    
    tcflush(mUartFd, TCIOFLUSH);
    LOG(INFO) << "UART opened in BLOCKING mode: " << mDevice;
    return true;
}

void NmeaReader::closeUart() { if (mUartFd >= 0) { close(mUartFd); mUartFd = -1; } }

bool NmeaReader::start() {
    if (mRunning.load()) return true;
    if (!openUart()) return false;
    mRunning.store(true);
    mReaderThread = std::thread(&NmeaReader::readerThreadFunc, this);
    return true;
}

void NmeaReader::stop() {
    if (!mRunning.load()) return;
    mRunning.store(false);
    closeUart(); // Wymusza odblokowanie read()
    if (mReaderThread.joinable()) mReaderThread.join();
}

void NmeaReader::setMinInterval(int32_t intervalMs) { mMinIntervalMs.store(intervalMs); }

int64_t NmeaReader::getCurrentTimestampMs() {
    struct timeval tv; gettimeofday(&tv, nullptr);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void NmeaReader::readerThreadFunc() {
    char buffer[READ_BUFFER_SIZE];
    char nmeaBuffer[NMEA_MAX_SIZE];
    int nmeaPos = 0;
    
    LOG(INFO) << "Reader thread started (Blocking wait)...";
    
    while (mRunning.load()) {
        if (mUartFd < 0) break;

        // Czekaj na dane
        int bytesRead = read(mUartFd, buffer, sizeof(buffer) - 1);
        
        if (bytesRead > 0) {
            for (int i = 0; i < bytesRead; i++) {
                char c = buffer[i];
                if (c == '$') nmeaPos = 0;
                if (nmeaPos < NMEA_MAX_SIZE - 1) nmeaBuffer[nmeaPos++] = c;
                if (c == '\n' || c == '\r') {
                    if (nmeaPos > 10) {
                        nmeaBuffer[nmeaPos] = '\0';
                        processNmeaSentence(std::string(nmeaBuffer));
                    }
                    nmeaPos = 0;
                }
            }
            
            // Raportuj satelity
            int64_t now = getCurrentTimestampMs();
            if (now - mLastSvReportMs >= SV_REPORT_INTERVAL_MS) {
                mLastSvReportMs = now;
                if (mSvStatusCallback && !mSatellites.empty()) {
                    std::lock_guard<std::mutex> lock(mSvMutex);
                    std::vector<GnssSvInfo> svList;
                    for (const auto& [key, info] : mSatellites) svList.push_back(info);
                    if (!svList.empty()) mSvStatusCallback(svList);
                }
            }
        } else if (bytesRead < 0) {
            LOG(ERROR) << "UART Read Error: " << strerror(errno);
            usleep(1000000); 
        }
    }
}

void NmeaReader::processNmeaSentence(const std::string& sentence) {
    if (sentence.length() < 10) return;
    
    int64_t timestamp = getCurrentTimestampMs();
    if (mNmeaCallback) mNmeaCallback(timestamp, sentence);
    
    if (sentence.find("GGA") != std::string::npos) parseGGA(sentence);
    else if (sentence.find("RMC") != std::string::npos) parseRMC(sentence);
    else if (sentence.find("GSV") != std::string::npos) parseGSV(sentence);
    else if (sentence.find("GSA") != std::string::npos) parseGSA(sentence);
    else if (sentence.find("VTG") != std::string::npos) parseVTG(sentence);
    
    if (mHasValidFix && mLocationCallback) {
        int64_t now = getCurrentTimestampMs();
        if (now - mLastLocationReportMs >= mMinIntervalMs.load()) {
            mLastLocationReportMs = now;
            std::lock_guard<std::mutex> lock(mLocationMutex);
            mCurrentLocation.timestampMillis = now;
            mCurrentLocation.elapsedRealtime.flags = ElapsedRealtime::HAS_TIMESTAMP_NS | ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS;
            mCurrentLocation.elapsedRealtime.timestampNs = now * 1000000LL;
            mCurrentLocation.elapsedRealtime.timeUncertaintyNs = 1000000.0;
            mLocationCallback(mCurrentLocation);
        }
    }
}

bool NmeaReader::parseGGA(const std::string& sentence) {
    auto fields = splitString(sentence, ',');
    if (fields.size() < 15) return false;
    
    mFixQuality = 0;
    if (!fields[6].empty()) mFixQuality = std::atoi(fields[6].c_str());
    
    if (mFixQuality > 0) mHasValidFix = true;
    else { mHasValidFix = false; return false; }
    
    std::lock_guard<std::mutex> lock(mLocationMutex);
    if (!fields[2].empty()) mCurrentLocation.latitudeDegrees = nmeaToDecimal(std::atof(fields[2].c_str()), fields[3][0]);
    if (!fields[4].empty()) mCurrentLocation.longitudeDegrees = nmeaToDecimal(std::atof(fields[4].c_str()), fields[5][0]);
    if (!fields[7].empty()) mNumSatellites = std::atoi(fields[7].c_str());
    
    mCurrentLocation.horizontalAccuracyMeters = 5.0;
    if (!fields[8].empty()) {
        float hdop = std::atof(fields[8].c_str());
        if(hdop > 0) mCurrentLocation.horizontalAccuracyMeters = hdop * 4.0;
    }
    mCurrentLocation.gnssLocationFlags |= GnssLocation::HAS_HORIZONTAL_ACCURACY | GnssLocation::HAS_LAT_LONG;
    
    if (!fields[9].empty()) {
        mCurrentLocation.altitudeMeters = std::atof(fields[9].c_str());
        mCurrentLocation.gnssLocationFlags |= GnssLocation::HAS_ALTITUDE;
    }
    return true;
}

bool NmeaReader::parseRMC(const std::string& sentence) {
    auto fields = splitString(sentence, ',');
    if (fields.size() < 10) return false;
    if (fields[2].empty() || fields[2][0] != 'A') return false;
    
    { std::lock_guard<std::mutex> svLock(mSvMutex); mSatsUsedInFix.clear(); }
    std::lock_guard<std::mutex> lock(mLocationMutex);
    if (!fields[7].empty()) {
        mCurrentLocation.speedMetersPerSec = std::atof(fields[7].c_str()) * 0.514444;
        mCurrentLocation.gnssLocationFlags |= GnssLocation::HAS_SPEED;
    }
    if (!fields[8].empty()) {
        mCurrentLocation.bearingDegrees = std::atof(fields[8].c_str());
        mCurrentLocation.gnssLocationFlags |= GnssLocation::HAS_BEARING;
    }
    return true;
}

bool NmeaReader::parseGSV(const std::string& sentence) {
    auto fields = splitString(sentence, ',');
    if (fields.size() < 4) return false;
    std::string talkerId = sentence.substr(1, 2);
    
    std::lock_guard<std::mutex> lock(mSvMutex);
    for (size_t i = 4; i + 3 < fields.size(); i += 4) {
        if (fields[i].empty()) continue;
        int svid = std::atoi(fields[i].c_str());
        if (svid == 0) continue;
        GnssSvInfo sv; sv.svid = svid;
        sv.constellation = getConstellationType(talkerId, svid);
        if (!fields[i+1].empty()) sv.elevationDegrees = std::atof(fields[i+1].c_str());
        if (!fields[i+2].empty()) sv.azimuthDegrees = std::atof(fields[i+2].c_str());
        if (i+3 < fields.size() && !fields[i+3].empty()) {
            std::string snr = fields[i+3]; size_t star = snr.find('*');
            if (star != std::string::npos) snr = snr.substr(0, star);
            if (!snr.empty()) { sv.cN0Dbhz = std::atof(snr.c_str()); sv.basebandCN0DbHz = sv.cN0Dbhz; }
        }
        sv.svFlag = (sv.cN0Dbhz > 0) ? static_cast<int32_t>(GnssSvFlags::HAS_CARRIER_FREQUENCY) : 0;
        for (int used : mSatsUsedInFix) if (used == svid) { sv.svFlag |= static_cast<int32_t>(GnssSvFlags::USED_IN_FIX); break; }
        mSatellites[(static_cast<int>(sv.constellation) << 16) | svid] = sv;
    }
    return true;
}

bool NmeaReader::parseGSA(const std::string& sentence) {
    auto fields = splitString(sentence, ',');
    if (fields.size() < 18) return false;
    std::lock_guard<std::mutex> lock(mSvMutex);
    // Akumulujemy satelity z wielu GSA - clear w parseRMC
    for (int i = 3; i <= 14 && i < (int)fields.size(); i++) {
        if (!fields[i].empty()) {
            int svid = std::atoi(fields[i].c_str());
            if (svid > 0) mSatsUsedInFix.push_back(svid);
        }
    }
    for (auto& [key, sv] : mSatellites) {
        sv.svFlag &= ~static_cast<int32_t>(GnssSvFlags::USED_IN_FIX);
        for (int usedSvid : mSatsUsedInFix) if (sv.svid == usedSvid) { sv.svFlag |= static_cast<int32_t>(GnssSvFlags::USED_IN_FIX); break; }
    }
    return true;
}

bool NmeaReader::parseVTG(const std::string& sentence) {
    auto fields = splitString(sentence, ',');
    if (fields.size() < 8) return false;
    std::lock_guard<std::mutex> lock(mLocationMutex);
    if (!fields[1].empty()) { mCurrentLocation.bearingDegrees = std::atof(fields[1].c_str()); mCurrentLocation.gnssLocationFlags |= GnssLocation::HAS_BEARING; }
    if (!fields[7].empty()) { mCurrentLocation.speedMetersPerSec = std::atof(fields[7].c_str()) / 3.6; mCurrentLocation.gnssLocationFlags |= GnssLocation::HAS_SPEED; }
    return true;
}

GnssConstellationType NmeaReader::getConstellationType(const std::string& tid, int svid) {
    if (tid == "GP") return GnssConstellationType::GPS;
    if (tid == "GL") return GnssConstellationType::GLONASS;
    if (tid == "GA") return GnssConstellationType::GALILEO;
    if (tid == "GB" || tid == "BD") return GnssConstellationType::BEIDOU;
    // POPRAWKA LITERÓWKI PONIŻEJ:
    if (tid == "GQ" || tid == "QZ") return GnssConstellationType::QZSS;
    if (tid == "GN") {
        if (svid >= 1 && svid <= 32) return GnssConstellationType::GPS;
        if (svid >= 65 && svid <= 96) return GnssConstellationType::GLONASS;
        if (svid >= 201 && svid <= 237) return GnssConstellationType::BEIDOU;
        if (svid >= 301 && svid <= 336) return GnssConstellationType::GALILEO;
        return GnssConstellationType::GPS;
    }
    return GnssConstellationType::UNKNOWN;
}

double NmeaReader::nmeaToDecimal(double coord, char dir) {
    int deg = static_cast<int>(coord / 100);
    double dec = deg + (coord - deg * 100) / 60.0;
    return (dir == 'S' || dir == 'W') ? -dec : dec;
}

std::vector<std::string> NmeaReader::splitString(const std::string& str, char d) {
    std::vector<std::string> r; std::string c;
    for (char ch : str) { if (ch == d) { r.push_back(c); c.clear(); } else if (ch != '\r' && ch != '\n') c += ch; }
    r.push_back(c); return r;
}

}
