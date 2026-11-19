// Fuzzes cpu_features' S390X detection on Linux: GetS390XInfo() parses
// /proc/cpuinfo and combines it with AT_HWCAP/AT_HWCAP2 (see fuzz_mock.h for
// the input layout). Built with -include arch_shim.h -D__s390x__ so the S390X
// implementation compiles on an x86_64 host.
#include "fuzz_mock.h"
#include "cpuinfo_s390x.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  mock_setup(data, size, kCpuinfoOnly);
  const S390XInfo info = GetS390XInfo();
  mock_sink_bytes(&info, sizeof(info));
  for (int i = 0; i < S390X_LAST_; ++i) {
    g_mock_sink += (uintptr_t)GetS390XFeaturesEnumValue(&info.features, (S390XFeaturesEnum)i);
    g_mock_sink += (uintptr_t)GetS390XFeaturesEnumName((S390XFeaturesEnum)i);
  }
  const S390XPlatformStrings strings = GetS390XPlatformStrings();
  mock_sink_bytes(&strings, sizeof(strings));
  return 0;
}
