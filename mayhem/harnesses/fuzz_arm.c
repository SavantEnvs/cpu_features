// Fuzzes cpu_features' Arm detection on Linux: GetArmInfo() parses
// /proc/cpuinfo and combines it with AT_HWCAP/AT_HWCAP2 (see fuzz_mock.h for
// the input layout). Built with -include arch_shim.h -D__arm__ so the Arm
// implementation compiles on an x86_64 host.
#include "fuzz_mock.h"
#include "cpuinfo_arm.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  mock_setup(data, size, kCpuinfoOnly);
  const ArmInfo info = GetArmInfo();
  mock_sink_bytes(&info, sizeof(info));
  for (int i = 0; i < ARM_LAST_; ++i) {
    g_mock_sink += (uintptr_t)GetArmFeaturesEnumValue(&info.features, (ArmFeaturesEnum)i);
    g_mock_sink += (uintptr_t)GetArmFeaturesEnumName((ArmFeaturesEnum)i);
  }
  g_mock_sink += GetArmCpuId(&info);
  return 0;
}
