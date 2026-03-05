# Project Settings
cmake_minimum_required(VERSION 3.20...3.30 FATAL_ERROR)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MyGame)

project(giraphics VERSION 0.1 LANGUAGES C CXX)
# Enable /MP for all targets globally
if(MSVC)
    add_compile_options(/MP)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Feature toggles used across engine/game targets.
option(SOFASPUDS_ENABLE_EDITOR "Enable in-engine editor and ImGui tooling" ON)
option(SOFASPUDS_DISABLE_AUDIO "Disable FMOD audio backend and use no-audio stubs" OFF)

if(EMSCRIPTEN)
  # Browser target should ship as gameplay-only and avoid native FMOD linkage.
  set(SOFASPUDS_ENABLE_EDITOR OFF CACHE BOOL "Enable in-engine editor and ImGui tooling" FORCE)
  set(SOFASPUDS_DISABLE_AUDIO ON CACHE BOOL "Disable FMOD audio backend and use no-audio stubs" FORCE)
endif()

# Ensure a consistent MSVC runtime across all targets and prebuilt libs.
# This avoids LIBCMT vs MSVCRT defaultlib conflicts.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Where extra cmake modules live
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Import third-party deps (e.g., glfw) if the helper exists
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/ImportDependencies.cmake")
  include(ImportDependencies)
  importDependencies()
endif()

# Subprojects
add_subdirectory(Engine)
add_subdirectory(Sandbox)

# Set startup project to BloodyGoodCurry
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BloodyGoodCurry)

# Optional test/setup module (only include if present)
include(BuildTest OPTIONAL)   # or include(BuildTests OPTIONAL) if that's the filename


