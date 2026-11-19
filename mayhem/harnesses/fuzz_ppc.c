// Fuzzes cpu_features' PPC detection on Linux: GetPPCInfo() parses
// /proc/cpuinfo and combines it with AT_HWCAP/AT_HWCAP2 (see fuzz_mock.h for
// the input layout). Built with -include arch_shim.h -D__powerpc__ so the PPC
// implementation compiles on an x86_64 host.
#include "fuzz_mock.h"
#include "cpuinfo_ppc.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  mock_setup(data, size, kCpuinfoOnly);
  const PPCInfo info = GetPPCInfo();
  mock_sink_bytes(&info, sizeof(info));
  for (int i = 0; i < PPC_LAST_; ++i) {
    g_mock_sink += (uintptr_t)GetPPCFeaturesEnumValue(&info.features, (PPCFeaturesEnum)i);
    g_mock_sink += (uintptr_t)GetPPCFeaturesEnumName((PPCFeaturesEnum)i);
  }
  const PPCPlatformStrings strings = GetPPCPlatformStrings();
  mock_sink_bytes(&strings, sizeof(strings));
  return 0;
}
