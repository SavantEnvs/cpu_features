// Fuzzes ndk_compat (the Android NDK cpufeatures replacement) built for
// 32-bit ARM: android_getCpuCount()/android_getCpuFeatures()/
// android_getCpuIdArm() parse /sys/devices/system/cpu/present and /possible
// (cpu mask lists such as "0-3,5") and then /proc/cpuinfo via GetArmInfo().
//
// Input: see fuzz_mock.h; the file payload is split on NUL bytes into
// present \0 possible \0 cpuinfo.
//
// ndk_compat caches its result behind a static pthread_once_t; the library
// source is included into this translation unit so that state can be reset
// before every input (each input's result depends only on that input). The
// library code itself is unchanged.
#include "fuzz_mock.h"
#include "../../ndk_compat/cpu-features.c"

static const char* const kNdkFiles[] = {"/sys/devices/system/cpu/present",
                                        "/sys/devices/system/cpu/possible",
                                        "/proc/cpuinfo", NULL};

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  mock_setup(data, size, kNdkFiles);
  g_once = (pthread_once_t)PTHREAD_ONCE_INIT;
  g_inited = 0;
  g_cpuFeatures = 0;
  g_cpuCount = 0;
  g_cpuIdArm = 0;
  g_mock_sink += (uintptr_t)android_getCpuFamily();
  g_mock_sink += (uintptr_t)android_getCpuCount();
  g_mock_sink += (uintptr_t)android_getCpuFeatures();
  g_mock_sink += (uintptr_t)android_getCpuIdArm();
  return 0;
}
