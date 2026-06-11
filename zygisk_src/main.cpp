#include "zygisk.hpp"
#include <sys/mount.h>
#include <unistd.h>
#include <cstring>
#include <android/log.h>

#define TAG "CpuinfoSpoofer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static const char *TARGET_PKGS[] = {
    "com.tencent.tmgp.dfm",
    "com.tencent.mf.uam",
    "com.liuzh.deviceinfo",
    nullptr
};

static const char *FAKE_CPUINFO = "/data/adb/modules/cpuinfo_spoofer/cpuinfo_spoof";
static const char *REAL_CPUINFO = "/proc/cpuinfo";

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

class CpuinfoSpoofer : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        if (!args->nice_name) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        const char *pkg = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!pkg) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        bool match = false;
        for (int i = 0; TARGET_PKGS[i] != nullptr; i++) {
            if (strcmp(pkg, TARGET_PKGS[i]) == 0) { match = true; break; }
        }
        env->ReleaseStringUTFChars(args->nice_name, pkg);
        if (!match) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        if (access(FAKE_CPUINFO, R_OK) != 0) { LOGE("fake cpuinfo not found"); return; }
        if (mount(FAKE_CPUINFO, REAL_CPUINFO, nullptr, MS_BIND | MS_RDONLY, nullptr) == 0) {
            LOGI("mounted fake cpuinfo");
        } else {
            LOGE("mount failed: %s", strerror(errno));
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {}
    void preServerSpecialize(ServerSpecializeArgs *args) override {
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

private:
    Api    *api = nullptr;
    JNIEnv *env = nullptr;
};

REGISTER_ZYGISK_MODULE(CpuinfoSpoofer)
