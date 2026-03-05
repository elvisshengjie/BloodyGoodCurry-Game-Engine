# author Rudy Castan
# date 2025 Spring
# CS250 Computer Graphics II
# copyright DigiPen Institute of Technology

# We will be using SDL2 for windowing & gl context creation
# Linux platforms have a very easy way to install these depenencies and expose them to compilers so,
#   we will use the builtin find_package
# Emscripten has their own port of SDL2 builtin. We can just #include, define -sUSE_SDL=2, and start using it.
#   Windows does not have a simple way to get it, so we download official windows binaries and link against those

add_library(the_sdl2 INTERFACE)

if(WIN32)
    # download binaries for SDL2 for windows x64
    CachedFetchContent_Declare(
        sdl2
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-devel-2.32.10-VC.zip
        URL_HASH SHA256=af347939395a58b365846aaea27391e69f9ec9d4dd650d6ac40802159b418a6e
    )
    FetchContent_MakeAvailable(sdl2)

    target_include_directories(the_sdl2 SYSTEM INTERFACE ${sdl2_SOURCE_DIR}/include)

    target_link_directories(the_sdl2 INTERFACE ${sdl2_SOURCE_DIR}/lib/x64)
    target_link_libraries(the_sdl2 INTERFACE SDL2)
    target_compile_definitions(the_sdl2 INTERFACE SDL_MAIN_HANDLED)

    # Define a custom target to copy SDL2.dll to the build directory
    set(TEMP_EXE_FOLDER $<IF:$<BOOL:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY},${CMAKE_BINARY_DIR}>)
    add_custom_target(copy_sdl2_dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${sdl2_SOURCE_DIR}/lib/x64/SDL2.dll
            ${TEMP_EXE_FOLDER}/SDL2.dll
            DEPENDS ${sdl2_SOURCE_DIR}/lib/x64/SDL2.dll
            COMMENT "Copying SDL2.dll to executable directory"
    )
    add_dependencies(the_sdl2 copy_sdl2_dll)
else()
    if(EMSCRIPTEN)
        # --use-port=sdl2                     - we want version 2 rather than SDL1
        # https://emscripten.org/docs/tools_reference/settings_reference.html?highlight=use_sdl#use-sdl
        target_compile_options(the_sdl2 INTERFACE --use-port=sdl2)
        target_link_options(the_sdl2 INTERFACE --use-port=sdl2)
    else()
        # on    Mac : brew install sdl2
        # on Ubuntu : apt install libsdl2-dev
        find_package(SDL2 REQUIRED)
        target_include_directories(the_sdl2 SYSTEM INTERFACE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(the_sdl2 INTERFACE ${SDL2_LIBRARIES})
    endif()
endif()
