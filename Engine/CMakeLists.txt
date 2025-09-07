# Name of the engine library
set(ENGINE_NAME MyEngine)

# Sources
file(GLOB_RECURSE GRAPHICS_SRC_FILES
     CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_LIST_DIR}/Graphics/*.[ch]pp)

add_library(${ENGINE_NAME} ${GRAPHICS_SRC_FILES})

# C++ standard
set_property(TARGET ${ENGINE_NAME} PROPERTY CXX_STANDARD 20)

# If Engine uses GLFW internally, keep PRIVATE.
# If Engine's PUBLIC headers include GLFW headers/types, change to PUBLIC.
target_link_libraries(${ENGINE_NAME} PUBLIC glfw)

# Warnings per toolchain
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(${ENGINE_NAME} PRIVATE -Wall -Wextra)
elseif (MSVC)
  target_compile_options(${ENGINE_NAME} PRIVATE /W3 /WX-)
endif()

# Expose Engine headers to dependents (so Sandbox can #include "Graphics/...")
target_include_directories(${ENGINE_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
