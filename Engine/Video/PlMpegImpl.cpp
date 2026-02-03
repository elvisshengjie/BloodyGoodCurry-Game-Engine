/*********************************************************************************************
 \file      pl_mpeg_impl.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Provides the single translation unit implementation for the pl_mpeg library.

 \details   pl_mpeg is a single-header (amalgamated) third-party decoder.
            To use it correctly in C/C++ projects:
            - Include "pl_mpeg.h" normally in headers / multiple .cpp files.
            - Define PL_MPEG_IMPLEMENTATION in exactly ONE .cpp file before including
              "pl_mpeg.h" to compile the implementation once.

            This file also suppresses common MSVC warnings produced by third-party code
            (implicit narrowing conversions and size_t conversions). Warnings are pushed
            and popped so suppression is limited to this translation unit only.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include <cstdio>

/*************************************************************************************
  \note   The following warning disables apply only for MSVC builds.
          They are used because pl_mpeg is third-party code that can trigger warnings
          under /W4 or /Wall, and we do not want noise in our project build logs.
*************************************************************************************/
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244) // conversion from 'double/int' to smaller type
#pragma warning(disable : 4267) // conversion from 'size_t' to smaller integer type
#pragma warning(disable : 4305) // truncation from 'double' to 'float'
#endif

/*************************************************************************************
  \brief Compile pl_mpeg's implementation exactly once.
  \note  This macro MUST appear in exactly one .cpp file in the entire project.
*************************************************************************************/
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
