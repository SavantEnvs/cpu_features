// Fuzzes cpu_features' x86 detection on Linux: GetX86Info(),
// GetX86CacheInfo(), GetX86Microarchitecture() and FillX86BrandString() decode
// CPUID leaves (hypervisor-controlled in a VM: vendor, family/model, feature
// bits, cache descriptors of leaf 2 and leaf 4, brand string) and the "flags"
// line of /proc/cpuinfo. Upstream's CPU_FEATURES_MOCK_CPUID_X86 seam (as used
// by test/cpuinfo_x86_test.cc) lets the harness answer CPUID from the input.
//
// Input layout:
//   [0..4)   XCR0 low 32 bits (little endian)
//   [4]      N = number of CPUID records
//   N * 24   records {leaf, subleaf, eax, ebx, ecx, edx}, u32 little endian
//   rest     /proc/cpuinfo contents
// Missing bytes read as zero; a leaf with no record reads as all zero.
#include "fuzz_mock.h"
#include "cpuinfo_x86.h"
#include "internal/cpuid_x86.h"

#define MAX_LEAVES 255
typedef struct {
  uint32_t leaf, subleaf;
  Leaf value;
} LeafRecord;
static LeafRecord g_leaves[MAX_LEAVES];
static size_t g_num_leaves;
static uint32_t g_xcr0;

static uint32_t le32(const uint8_t* p, size_t avail) {
  uint32_t v = 0;
  for (size_t i = 0; i < 4 && i < avail; ++i) v |= (uint32_t)p[i] << (8 * i);
  return v;
}

Leaf GetCpuidLeaf(uint32_t leaf_id, int ecx) {
  for (size_t i = 0; i < g_num_leaves; ++i) {
    if (g_leaves[i].leaf == leaf_id && g_leaves[i].subleaf == (uint32_t)ecx)
      return g_leaves[i].value;
  }
  Leaf zero = {0, 0, 0, 0};
  return zero;
}

uint32_t GetXCR0Eax(void) { return g_xcr0; }

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  size_t off = 0;
  g_xcr0 = le32(data, size);
  off = size < 4 ? size : 4;
  size_t n = off < size ? data[off++] : 0;
  g_num_leaves = 0;
  for (size_t i = 0; i < n && i < MAX_LEAVES; ++i) {
    uint32_t w[6];
    for (int j = 0; j < 6; ++j) {
      w[j] = le32(data + off, size - off);
      off = (size - off) < 4 ? size : off + 4;
    }
    LeafRecord r = {w[0], w[1], {w[2], w[3], w[4], w[5]}};
    g_leaves[g_num_leaves++] = r;
  }
  // The rest of the input is /proc/cpuinfo; no hwcaps or platform header.
  static const char* const kFiles[] = {"/proc/cpuinfo", NULL};
  g_mock_names = kFiles;
  memset(g_mock_files, 0, sizeof(g_mock_files));
  g_mock_files[0].data = data + off;
  g_mock_files[0].size = size - off;
  g_mock_files[0].present = 1;

  const X86Info info = GetX86Info();
  mock_sink_bytes(&info, sizeof(info));
  const CacheInfo cache = GetX86CacheInfo();
  mock_sink_bytes(&cache, sizeof(cache));
  const X86Microarchitecture uarch = GetX86Microarchitecture(&info);
  g_mock_sink += (uintptr_t)GetX86MicroarchitectureName(uarch);
  char brand[49];
  FillX86BrandString(brand);
  mock_sink_bytes(brand, sizeof(brand));
  for (int i = 0; i < X86_LAST_; ++i) {
    g_mock_sink += (uintptr_t)GetX86FeaturesEnumValue(&info.features, (X86FeaturesEnum)i);
    g_mock_sink += (uintptr_t)GetX86FeaturesEnumName((X86FeaturesEnum)i);
  }
  return 0;
}
