# ================================
# Engine target setup
# ================================
set(ENGINE_NAME MyEngine)

# ---------------------------------------------------------------------------
# Editor toggle (used by code to include/exclude editor & ImGui tooling)
# ---------------------------------------------------------------------------
if(NOT DEFINED SOFASPUDS_ENABLE_EDITOR)
    option(SOFASPUDS_ENABLE_EDITOR "Enable in-engine editor and ImGui tooling" ON)
endif()
if(NOT DEFINED SOFASPUDS_DISABLE_AUDIO)
    option(SOFASPUDS_DISABLE_AUDIO "Disable FMOD audio backend and use no-audio stubs" OFF)
endif()

if(EMSCRIPTEN)
    # Web builds currently run without the editor and native FMOD.
    set(SOFASPUDS_ENABLE_EDITOR OFF CACHE BOOL "Enable in-engine editor and ImGui tooling" FORCE)
    set(SOFASPUDS_DISABLE_AUDIO ON CACHE BOOL "Disable FMOD audio backend and use no-audio stubs" FORCE)
endif()

# ---------------------------------------------------------------------------
# Source / header discovery (excluding ThirdParty)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE ENGINE_SRC_FILES
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/**/*.[cC]pp
)

file(GLOB_RECURSE ENGINE_HEADER_FILES
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/**/*.[hH]
  ${CMAKE_CURRENT_LIST_DIR}/**/*.hpp
)

# Exclude vendored code handled separately (glad, etc.)
list(FILTER ENGINE_SRC_FILES     EXCLUDE REGEX ".*/ThirdParty/.*")
list(FILTER ENGINE_HEADER_FILES  EXCLUDE REGEX ".*/ThirdParty/.*")
# Audio stub should only be compiled in no-audio builds.
list(FILTER ENGINE_SRC_FILES EXCLUDE REGEX ".*/Audio/AudioManagerStub\\.cpp$")

set(ALL_ENGINE_FILES ${ENGINE_SRC_FILES} ${ENGINE_HEADER_FILES})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${ALL_ENGINE_FILES})

if(SOFASPUDS_DISABLE_AUDIO)
    list(FILTER ENGINE_SRC_FILES EXCLUDE REGEX ".*/Audio/AudioManager\\.cpp$")
    list(APPEND ENGINE_SRC_FILES ${CMAKE_CURRENT_LIST_DIR}/Audio/AudioManagerStub.cpp)
endif()

# ---------------------------------------------------------------------------
# Manual GLAD (vendored)
# ---------------------------------------------------------------------------
add_library(glad STATIC
    ${CMAKE_CURRENT_LIST_DIR}/ThirdParty/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/ThirdParty/glad/include
)

# ---------------------------------------------------------------------------
# Engine library
# ---------------------------------------------------------------------------
add_library(${ENGINE_NAME}
    ${ENGINE_SRC_FILES}
    ${ENGINE_HEADER_FILES}
)

# Libraries that are always linked (non-editor specific)
set(ALL_LIBS
    glfw
)

# ---------------------------------------------------------------------------
# FMOD setup
# ---------------------------------------------------------------------------
if(NOT SOFASPUDS_DISABLE_AUDIO)
    set(FMOD_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/ThirdParty/fmod/inc)
    set(FMOD_LIBRARY_DIR ${CMAKE_CURRENT_LIST_DIR}/ThirdParty/fmod/lib/x64)

    # Include FMOD headers
    target_include_directories(${ENGINE_NAME} PUBLIC ${FMOD_INCLUDE_DIR})

    # Link FMOD libraries
    target_link_directories(${ENGINE_NAME} PRIVATE ${FMOD_LIBRARY_DIR})
    list(APPEND ALL_LIBS fmod_vc)
endif()

# ---------------------------------------------------------------------------
# JSON dependency include
# ---------------------------------------------------------------------------
target_include_directories(${ENGINE_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/ThirdParty/json_dep
)

# ---------------------------------------------------------------------------
# Expose top-level engine folder so headers are visible to game target(s)
# ---------------------------------------------------------------------------
target_include_directories(${ENGINE_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(${ENGINE_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Sandbox/${SOFASPUDS_GAME_CODE_DIR})

# ---------------------------------------------------------------------------
# Core engine links (OpenGL stack + math + fonts)
# ---------------------------------------------------------------------------
target_link_libraries(${ENGINE_NAME} PUBLIC
    glfw
    glad
    stb_image
    glm::glm
    freetype
    pl_mpeg
)

target_link_libraries(${ENGINE_NAME} PRIVATE
    ${ALL_LIBS}
)

# ---------------------------------------------------------------------------
# Editor macro + ImGui linkage
#   - When ENABLED: define macro + PUBLIC-link imgui
#   - When DISABLED: no imgui linkage at all (shipping build)
# ---------------------------------------------------------------------------
if(SOFASPUDS_ENABLE_EDITOR)
    message(STATUS "[SofaSpuds] Editor: ENABLED (ImGui + tools compiled in)")
    target_compile_definitions(${ENGINE_NAME} PUBLIC SOFASPUDS_ENABLE_EDITOR=1)
    # Make ImGui visible to anything that links the engine (e.g., BloodyGoodCurry)
    target_link_libraries(${ENGINE_NAME} PUBLIC imgui)
else()
    message(STATUS "[SofaSpuds] Editor: DISABLED (no ImGui; shipping build)")
    target_compile_definitions(${ENGINE_NAME} PUBLIC SOFASPUDS_ENABLE_EDITOR=0)
endif()

if(SOFASPUDS_DISABLE_AUDIO)
    message(STATUS "[SofaSpuds] Audio: DISABLED (FMOD stubs)")
    target_compile_definitions(${ENGINE_NAME} PUBLIC SOFASPUDS_DISABLE_AUDIO=1)
else()
    message(STATUS "[SofaSpuds] Audio: ENABLED (FMOD)")
    target_compile_definitions(${ENGINE_NAME} PUBLIC SOFASPUDS_DISABLE_AUDIO=0)
endif()

target_compile_definitions(${ENGINE_NAME} PUBLIC GLFW_INCLUDE_NONE)

# ---------------------------------------------------------------------------
# Group files by folder (if helper macro exists)
# ---------------------------------------------------------------------------
if(COMMAND GROUP_FILES_BY_FOLDER)
    GROUP_FILES_BY_FOLDER(${ALL_ENGINE_FILES})
endif()

# ---------------------------------------------------------------------------
# C++ standard
# ---------------------------------------------------------------------------
set_property(TARGET ${ENGINE_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${ENGINE_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${ENGINE_NAME} PRIVATE -Wall -Wextra)
elseif (MSVC)
    target_compile_options(${ENGINE_NAME} PRIVATE /W4 /WX-)
endif()

# ---------------------------------------------------------------------------
# Platform OpenGL (required)
# ---------------------------------------------------------------------------
if (EMSCRIPTEN)
    # Emscripten provides GL/WebGL via its own toolchain.
elseif (WIN32)
    target_link_libraries(${ENGINE_NAME} PRIVATE opengl32)
elseif (APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(OpenGL_LIBRARY OpenGL REQUIRED)
    target_link_libraries(${ENGINE_NAME} PRIVATE ${OpenGL_LIBRARY} ${COCOA_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${ENGINE_NAME} PRIVATE OpenGL::GL)
endif()
