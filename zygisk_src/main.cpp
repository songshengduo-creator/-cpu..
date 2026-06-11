#include <sys/types.h>
#include "zygisk.hpp"
#include <sys/mount.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <string>
#include <vector>
#include <android/log.h>

#define TAG "CpuinfoSpoofer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define CONF_PATH "/data/adb/modules/cpuinfo_spoofer/packages.conf"
#define FAKE_CPUINFO "/data/adb/modules/cpuinfo_spoofer/cpuinfo_spoof"
#define REAL_CPUINFO "/proc/cpuinfo"

static std::vector<std::string> loadPackages() {
    std::vector<std::string> pkgs;
    FILE *f = fopen(CONF_PATH, "r");
    if (!f) return pkgs;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        std::string s(line);
        // 去掉换行和空格
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
            s.pop_back();
        // 跳过空行和注释
        if (s.empty() || s[0] == '#') continue;
        pkgs.push_back(s);
    }
    fclose(f);
    return pkgs;
}

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

        auto pkgs = loadPackages();
        bool match = false;
        for (auto &p : pkgs) {
            if (p == pkg) { match = true; break; }
        }
        env->ReleaseStringUTFChars(args->nice_name, pkg);

        if (!match) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        if (access(FAKE_CPUINFO, R_OK) != 0) {
            LOGE("fake cpuinfo not found");
            return;
        }
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
