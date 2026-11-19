// Fuzzes cpu_features' Aarch64 detection on Linux: GetAarch64Info() parses
// /proc/cpuinfo and combines it with AT_HWCAP/AT_HWCAP2 (see fuzz_mock.h for
// the input layout). Built with -include arch_shim.h -D__aarch64__ so the Aarch64
// implementation compiles on an x86_64 host.
#include "fuzz_mock.h"
#include "cpuinfo_aarch64.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  mock_setup(data, size, kCpuinfoOnly);
  const Aarch64Info info = GetAarch64Info();
  mock_sink_bytes(&info, sizeof(info));
  for (int i = 0; i < AARCH64_LAST_; ++i) {
    g_mock_sink += (uintptr_t)GetAarch64FeaturesEnumValue(&info.features, (Aarch64FeaturesEnum)i);
    g_mock_sink += (uintptr_t)GetAarch64FeaturesEnumName((Aarch64FeaturesEnum)i);
  }

  return 0;
}
