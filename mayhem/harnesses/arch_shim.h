// Force-included (-include) when building a non-x86 architecture's
// implementation on an x86_64 host. System headers are pulled in first with
// the host's macros, then the host architecture macros are dropped so that
// cpu_features_macros.h selects the architecture given with -D on the command
// line (-D__arm__, -D__aarch64__, -D__mips__, ...). Only upstream's test seams
// are used, so no target-specific instructions are compiled.
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef __x86_64__
#undef __i386__
#undef __amd64__
#undef __amd64
#undef __x86_64
