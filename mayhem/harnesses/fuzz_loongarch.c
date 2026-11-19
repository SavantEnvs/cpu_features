// Fuzzes cpu_features' LoongArch detection on Linux: GetLoongArchInfo() parses
// /proc/cpuinfo and combines it with AT_HWCAP/AT_HWCAP2 (see fuzz_mock.h for
// the input layout). Built with -include arch_shim.h -D__loongarch64 so the LoongArch
// implementation compiles on an x86_64 host.
#include "fuzz_mock.h"
#include "cpuinfo_loongarch.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  mock_setup(data, size, kCpuinfoOnly);
  const LoongArchInfo info = GetLoongArchInfo();
  mock_sink_bytes(&info, sizeof(info));
  for (int i = 0; i < LOONGARCH_LAST_; ++i) {
    g_mock_sink += (uintptr_t)GetLoongArchFeaturesEnumValue(&info.features, (LoongArchFeaturesEnum)i);
    g_mock_sink += (uintptr_t)GetLoongArchFeaturesEnumName((LoongArchFeaturesEnum)i);
  }

  return 0;
}
