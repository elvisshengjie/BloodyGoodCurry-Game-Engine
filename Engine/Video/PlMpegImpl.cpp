#include <cstdio>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244) // conversion from 'double/int' to smaller type
#pragma warning(disable : 4267) // size_t to int/long
#pragma warning(disable : 4305) // double to float truncation
#endif

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
