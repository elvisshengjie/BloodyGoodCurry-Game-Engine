# author Rudy Castan
# author TODO your name
# date 2025 Spring
# CS250 Computer Graphics II
# copyright DigiPen Institute of Technology

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SOURCE_CODE

    assets/Path.hpp assets/Path.cpp
    assets/Reloader.hpp assets/Reloader.cpp

    demos/D01HelloQuad.hpp demos/D01HelloQuad.cpp
    demos/DemosFactory.hpp demos/DemosFactory.cpp
    demos/IDemo.hpp

    opengl/Environment.hpp
    opengl/GL.hpp opengl/GL.cpp
    opengl/Handle.hpp
    opengl/IndexBuffer.hpp opengl/IndexBuffer.cpp
    opengl/Shader.hpp opengl/Shader.cpp
    opengl/Texture.hpp opengl/Texture.cpp
    opengl/VertexArray.hpp opengl/VertexArray.cpp
    opengl/VertexBuffer.hpp opengl/VertexBuffer.cpp

    util/Directory.hpp util/Directory.cpp
    util/Environment.hpp
    util/FPS.hpp
    util/Logger.hpp util/Logger.cpp
    util/Random.hpp util/Random.cpp
    util/Timer.hpp
    util/WatchFiles.hpp util/WatchFiles.cpp

    window/input/Environment.hpp
    window/input/SDLInput.hpp window/input/SDLInput.cpp
    window/Application.hpp window/Application.cpp
    window/Environment.hpp
    window/ImGuiHelper.hpp window/ImGuiHelper.cpp
    window/Logo.hpp window/Logo.cpp
    window/Settings.hpp window/Settings.cpp
    window/Title.hpp

    main.cpp
)

add_executable(graphics_fun ${SOURCE_CODE})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_CODE})

target_link_libraries(graphics_fun PRIVATE project_options dependencies)
target_include_directories(graphics_fun PRIVATE .)

target_compile_definitions(graphics_fun PRIVATE $<$<BOOL:${IS_DEVELOPER_VERSION}>:DEVELOPER_VERSION>)

if(EMSCRIPTEN)

    set_target_properties(graphics_fun PROPERTIES SUFFIX ".html")

    # https://emscripten.org/docs/tools_reference/settings_reference.html
    # ASSERTIONS=1                  - we want asserts to work
    # WASM=1                        - we want web assembly generated rather than just javascript
    # DISABLE_EXCEPTION_CATCHING=0  - we are using exceptions so let's not disable them
    # ALLOW_MEMORY_GROWTH=1         - we don't know exactly how much heap space we'll need so allow it to grow
    # EXIT_RUNTIME=1                - have exiting actually stop the program
    # SINGLE_FILE=1                 - generate everything into one html file
    # --embed-file                  - https://emscripten.org/docs/tools_reference/emcc.html#emcc-embed-file
    # --use-preload-cache           - help with faster reloads : https://emscripten.org/docs/compiling/Deploying-Pages.html#providing-a-quick-second-time-load
    # -lembind                      - to call c++ from javascript https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html
    # --shell-file                  - to customize the webpage https://emscripten.org/docs/compiling/Deploying-Pages.html#build-files-and-custom-shell
    target_link_options(graphics_fun PRIVATE 
    -sASSERTIONS=$<$<BOOL:${IS_DEVELOPER_VERSION}>:1>$<$<NOT:$<BOOL:${IS_DEVELOPER_VERSION}>>:0>
    -sWASM=1 
    -sDISABLE_EXCEPTION_CATCHING=0 
    -sALLOW_MEMORY_GROWTH=1 
    -sEXIT_RUNTIME=1 
    -sSINGLE_FILE=1 
    --embed-file ${CMAKE_SOURCE_DIR}/assets@/assets
    --use-preload-cache
    -lembind
    --shell-file ${CMAKE_SOURCE_DIR}/app_resources/web/index_shell.html)
elseif(WIN32)
    # developer versions can have a debug console but the Release version shouldn't have one
    # Let's tell the linker to always use the regulary main() function, otherewise it will look for a WinMain function
    target_link_options(graphics_fun PRIVATE 
    $<IF:$<BOOL:${IS_DEVELOPER_VERSION}>,/SUBSYSTEM:CONSOLE,/SUBSYSTEM:WINDOWS>
    /ENTRY:mainCRTStartup)

    set(ICON_RC ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
    if(NOT EXISTS ${ICON_RC})
        set(ICON_FILE ${CMAKE_SOURCE_DIR}/app_resources/windows/digipen.ico)
        file(WRITE ${ICON_RC} "IDR_MAINFRAME ICON \"${ICON_FILE}\"\nIDI_ICON1 ICON DISCARDABLE \"${ICON_FILE}\"")
    endif()

    set_source_files_properties(${ICON_RC} PROPERTIES GENERATED TRUE)
    target_sources(graphics_fun PRIVATE ${ICON_RC})
endif()
