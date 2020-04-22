#pragma once

#include "Base.h"

// enable all features by default, to work with full source set during development
#define NAGA_FEATURE_CLJS 1

// disable individual features via build system
#if defined(NAGA_DISABLE_FEATURE_CLJS)
#undef NAGA_FEATURE_CLJS
#endif