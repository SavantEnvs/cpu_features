// Mock environment for cpu_features fuzz harnesses.
//
// cpu_features reads untrusted data from the kernel: /proc/cpuinfo,
// /sys/devices/system/cpu/{present,possible}, AT_HWCAP/AT_HWCAP2 and the
// AT_PLATFORM/AT_BASE_PLATFORM strings. Upstream's own test seams
// (CPU_FEATURES_MOCK_FILESYSTEM and CPU_FEATURES_TEST, see
// test/filesystem_for_testing.cc and test/hwcaps_for_testing.cc) let the
// harness supply those; here they are served from the fuzz input.
//
// Input layout (missing bytes read as zero):
//   [0..8)   AT_HWCAP  (little endian)
//   [8..16)  AT_HWCAP2 (little endian)
//   [16]     P = length of AT_PLATFORM string (0 -> NULL pointer)
//   [17]     B = length of AT_BASE_PLATFORM string (0 -> NULL pointer)
//   P bytes, B bytes
//   rest:    file contents. Split on NUL bytes into sections; file k of
//            the harness's file list gets section k (missing -> open fails).
#ifndef MAYHEM_FUZZ_MOCK_H_
#define MAYHEM_FUZZ_MOCK_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "internal/filesystem.h"
#include "internal/hwcaps.h"

#define MOCK_MAX_FILES 4

typedef struct {
  const uint8_t* data;
  size_t size;
  int present;
} MockFile;

static const char* const* g_mock_names;  // harness's file list, NULL-ended
static MockFile g_mock_files[MOCK_MAX_FILES];
static size_t g_mock_pos[MOCK_MAX_FILES];
static HardwareCapabilities g_mock_hwcaps;
static char g_mock_platform[256];
static char g_mock_base_platform[256];
static int g_mock_has_platform, g_mock_has_base_platform;

static uint64_t mock_le64(const uint8_t* p, size_t avail) {
  uint64_t v = 0;
  for (size_t i = 0; i < 8 && i < avail; ++i) v |= (uint64_t)p[i] << (8 * i);
  return v;
}

// Splits `data` into the mock environment. `names` lists the files the
// library may open, in the order their sections appear in the input.
static void mock_setup(const uint8_t* data, size_t size,
                       const char* const* names) {
  size_t off = 0;
  g_mock_hwcaps.hwcaps = mock_le64(data, size);
  g_mock_hwcaps.hwcaps2 = size > 8 ? mock_le64(data + 8, size - 8) : 0;
  off = size < 16 ? size : 16;
  size_t plen = off < size ? data[off++] : 0;
  size_t blen = off < size ? data[off++] : 0;
  if (plen > size - off) plen = size - off;
  memcpy(g_mock_platform, data + off, plen);
  g_mock_platform[plen] = 0;
  g_mock_has_platform = plen != 0;
  off += plen;
  if (blen > size - off) blen = size - off;
  memcpy(g_mock_base_platform, data + off, blen);
  g_mock_base_platform[blen] = 0;
  g_mock_has_base_platform = blen != 0;
  off += blen;

  g_mock_names = names;
  memset(g_mock_files, 0, sizeof(g_mock_files));
  memset(g_mock_pos, 0, sizeof(g_mock_pos));
  size_t nfiles = 0;
  while (names[nfiles] && nfiles < MOCK_MAX_FILES) ++nfiles;
  for (size_t k = 0; k < nfiles && off <= size; ++k) {
    const uint8_t* start = data + off;
    size_t remain = size - off;
    // The last file takes everything that is left; earlier ones end at NUL.
    const uint8_t* nul =
        (k + 1 < nfiles && remain) ? memchr(start, 0, remain) : NULL;
    size_t len = nul ? (size_t)(nul - start) : remain;
    g_mock_files[k].data = start;
    g_mock_files[k].size = len;
    g_mock_files[k].present = 1;
    off += len + 1;  // skip the NUL separator
    if (!nul) break;
  }
}

int CpuFeatures_OpenFile(const char* filename) {
  for (int k = 0; g_mock_names && g_mock_names[k] && k < MOCK_MAX_FILES; ++k) {
    if (strcmp(filename, g_mock_names[k]) == 0) {
      if (!g_mock_files[k].present) return -1;
      g_mock_pos[k] = 0;
      return k;
    }
  }
  return -1;
}

void CpuFeatures_CloseFile(int fd) { (void)fd; }

int CpuFeatures_ReadFile(int fd, void* buffer, size_t buffer_size) {
  if (fd < 0 || fd >= MOCK_MAX_FILES || !g_mock_files[fd].present) return -1;
  size_t left = g_mock_files[fd].size - g_mock_pos[fd];
  size_t n = left < buffer_size ? left : buffer_size;
  memcpy(buffer, g_mock_files[fd].data + g_mock_pos[fd], n);
  g_mock_pos[fd] += n;
  return (int)n;
}

HardwareCapabilities CpuFeatures_GetHardwareCapabilities(void) {
  return g_mock_hwcaps;
}

const char* CpuFeatures_GetPlatformPointer(void) {
  return g_mock_has_platform ? g_mock_platform : NULL;
}

const char* CpuFeatures_GetBasePlatformPointer(void) {
  return g_mock_has_base_platform ? g_mock_base_platform : NULL;
}

// Keeps the optimizer from discarding parsed results.
static volatile uintptr_t g_mock_sink;
static void mock_sink_bytes(const void* p, size_t n) {
  const unsigned char* b = (const unsigned char*)p;
  uintptr_t acc = 0;
  for (size_t i = 0; i < n; ++i) acc = acc * 31 + b[i];
  g_mock_sink += acc;
}

static const char* const kCpuinfoOnly[] = {"/proc/cpuinfo", NULL};

#endif  // MAYHEM_FUZZ_MOCK_H_
