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



# ---- GLM (math library) ----
macro(import_glm)
  if (NOT TARGET glm)
    FetchContent_Declare(
      glm
      GIT_REPOSITORY https://github.com/g-truc/glm.git
      GIT_TAG 1.0.1
    )
    FetchContent_MakeAvailable(glm)
  endif()
endmacro()

# ---- stb_image (image loader) ----
macro(import_stb_image)
  if (NOT TARGET stb_image)
    FetchContent_Declare(
      stb
      GIT_REPOSITORY https://github.com/nothings/stb.git
      GIT_TAG master
    )
    FetchContent_MakeAvailable(stb)

    # Generate a tiny .cpp that defines STB_IMAGE_IMPLEMENTATION
    file(WRITE ${stb_BINARY_DIR}/stb_image_impl.cpp
"// Auto-generated at configure time
#define STB_IMAGE_IMPLEMENTATION
#include \"${stb_SOURCE_DIR}/stb_image.h\"
")

    add_library(stb_image STATIC ${stb_BINARY_DIR}/stb_image_impl.cpp)
    target_include_directories(stb_image PUBLIC ${stb_SOURCE_DIR})
    if (MSVC)
      target_compile_options(stb_image PRIVATE /W0)
    else()
      target_compile_options(stb_image PRIVATE -w)
    endif()
  endif()
endmacro()

# ---- Bundle entrypoint ----
macro(importDependencies)
   import_glfw()
  import_glm()
  import_stb_image()
endmacro()
