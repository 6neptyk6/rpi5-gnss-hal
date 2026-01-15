#define LOG_TAG "GnssHalService"
#include "Gnss.h"
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::gnss::implementation::Gnss;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Gnss> gnss = ndk::SharedRefBase::make<Gnss>();
    const std::string instance = std::string() + Gnss::descriptor + "/default";
    AServiceManager_addService(gnss->asBinder().get(), instance.c_str());
    LOG(INFO) << "GNSS HAL started";
    ABinderProcess_joinThreadPool();
    return 0;
}
