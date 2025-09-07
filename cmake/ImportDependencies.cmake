include(FetchContent)

# ---- GLFW ----
macro(import_glfw)
  if (NOT TARGET glfw)
    # Configure GLFW options before fetching
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
      glfw
      GIT_REPOSITORY https://github.com/glfw/glfw.git
      GIT_TAG 3.3.8
    )

    # New, CMake-4.0 friendly API (does populate + add_subdirectory)
    FetchContent_MakeAvailable(glfw)
  endif()
endmacro()

# ---- Bundle entrypoint ----
macro(importDependencies)
  import_glfw()
endmacro()
