// Fuzzes cpu_features' Mips detection on Linux: GetMipsInfo() parses
// /proc/cpuinfo and combines it with AT_HWCAP/AT_HWCAP2 (see fuzz_mock.h for
// the input layout). Built with -include arch_shim.h -D__mips__ so the Mips
// implementation compiles on an x86_64 host.
#include "fuzz_mock.h"
#include "cpuinfo_mips.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  mock_setup(data, size, kCpuinfoOnly);
  const MipsInfo info = GetMipsInfo();
  mock_sink_bytes(&info, sizeof(info));
  for (int i = 0; i < MIPS_LAST_; ++i) {
    g_mock_sink += (uintptr_t)GetMipsFeaturesEnumValue(&info.features, (MipsFeaturesEnum)i);
    g_mock_sink += (uintptr_t)GetMipsFeaturesEnumName((MipsFeaturesEnum)i);
  }

  return 0;
}
