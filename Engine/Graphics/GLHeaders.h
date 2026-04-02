/*********************************************************************************************
 \file      GLHeaders.h
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%
 \brief     Provides the platform-appropriate OpenGL header includes for the engine.
 \details   Uses GLES headers for Emscripten/WebGL builds and glad for native desktop builds.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#else
#include <glad/glad.h>
#endif

