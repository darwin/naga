#pragma once

#include "Base.h"

// enable all features by default, to work with full source set during development
#define STPYV8_FEATURE_CLJS 1

// disable individual features via build system
#if defined(STPYV8_DISABLE_FEATURE_CLJS)
#undef STPYV8_FEATURE_CLJS
#endif