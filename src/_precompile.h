// note: initially taken from V8 repo

// This file is used as a precompiled header for both C and C++ files. So
// any C++ headers must go in the __cplusplus block below.

#if defined(BUILD_PRECOMPILE_H_)
#error You shouldn't include the precompiled header file more than once.
#endif

#define BUILD_PRECOMPILE_H_

#if defined(__cplusplus)

#include "Base.h"

#endif  // __cplusplus
