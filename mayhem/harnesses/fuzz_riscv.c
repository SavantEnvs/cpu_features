// Fuzzes cpu_features' Riscv detection on Linux: GetRiscvInfo() parses
// /proc/cpuinfo and combines it with AT_HWCAP/AT_HWCAP2 (see fuzz_mock.h for
// the input layout). Built with -include arch_shim.h -D__riscv so the Riscv
// implementation compiles on an x86_64 host.
#include "fuzz_mock.h"
#include "cpuinfo_riscv.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  mock_setup(data, size, kCpuinfoOnly);
  const RiscvInfo info = GetRiscvInfo();
  mock_sink_bytes(&info, sizeof(info));
  for (int i = 0; i < RISCV_LAST_; ++i) {
    g_mock_sink += (uintptr_t)GetRiscvFeaturesEnumValue(&info.features, (RiscvFeaturesEnum)i);
    g_mock_sink += (uintptr_t)GetRiscvFeaturesEnumName((RiscvFeaturesEnum)i);
  }

  return 0;
}
