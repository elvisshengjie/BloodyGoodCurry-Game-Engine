# Create the Sandbox executable (your game)
add_executable(MyGame
    MyGame/main.cpp
    MyGame/Game.cpp
)


# Make sure MyGame can find headers in Engine/
target_include_directories(MyGame PRIVATE ${CMAKE_SOURCE_DIR}/Engine)

# Link MyGame with the Engine library
target_link_libraries(MyGame PRIVATE MyEngine)

# Use C++20
set_property(TARGET MyGame PROPERTY CXX_STANDARD 20)


# Platform-specific compiler flags (optional)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(MyGame PRIVATE -Wall -Wextra)
elseif (MSVC)
    target_compile_options(MyGame PRIVATE /W3 /WX-)
endif()
