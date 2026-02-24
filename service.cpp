#define LOG_TAG "android.hardware.gnss-service.rpi5"
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include "Gnss.h"

using aidl::android::hardware::gnss::implementation::Gnss;

int main() {
    LOG(INFO) << "GNSS HAL Service for Raspberry Pi 5 starting...";
    ABinderProcess_setThreadPoolMaxThreadCount(2);
    std::shared_ptr<Gnss> gnss = ndk::SharedRefBase::make<Gnss>();
    const std::string instance = std::string() + Gnss::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(gnss->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
        LOG(FATAL) << "Failed to register GNSS HAL service: " << status;
        return 1;
    }
    LOG(INFO) << "GNSS HAL Service registered successfully";
    ABinderProcess_startThreadPool();
    ABinderProcess_joinThreadPool();
    return 0;
}
