# Create the Sandbox executable (your game)
add_executable(MyGame
    MyGame/main.cpp
    MyGame/Game.cpp
)

# Make sure MyGame can find headers in Engine/
target_include_directories(MyGame PRIVATE ${CMAKE_SOURCE_DIR}/Engine)

# Link MyGame with the Engine library
target_link_libraries(MyGame PRIVATE MyEngine)

# FMOD setup for Sandbox
set(FMOD_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmod/inc)
set(FMOD_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmod/lib/x64)

# Include FMOD headers
target_include_directories(MyGame PRIVATE ${FMOD_INCLUDE_DIR})

# Link FMOD libraries
target_link_directories(MyGame PRIVATE ${FMOD_LIBRARY_DIR})
target_link_libraries(MyGame PRIVATE fmod_vc)


# Use C++20
set_property(TARGET MyGame PROPERTY CXX_STANDARD 20)


# Platform-specific compiler flags (optional)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(MyGame PRIVATE -Wall -Wextra)
elseif (MSVC)
    target_compile_options(MyGame PRIVATE /W3 /WX-)
endif()

# Copy FMOD DLL files to build directory
set(FMOD_DLL_DIR "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmod/lib/x64")
set(FMOD_DLL_FILE "${FMOD_DLL_DIR}/fmod.dll")

# Copy FMOD DLL after build
add_custom_command(TARGET MyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${FMOD_DLL_FILE}"
    "$<TARGET_FILE_DIR:MyGame>/"
    COMMENT "Copying FMOD DLL to build directory"
    VERBATIM
)

# Copy game assets (audio files) to build directory
add_custom_command(TARGET MyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:MyGame>/game-assests/audio/sfx"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/game-assests/audio/sfx"
    "$<TARGET_FILE_DIR:MyGame>/game-assests/audio/sfx"
    COMMENT "Copying game assets (audio files) to build directory"
    VERBATIM
)
