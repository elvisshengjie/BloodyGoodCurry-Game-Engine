# author Rudy Castan
# date 2025 Spring
# CS250 Computer Graphics II
# copyright DigiPen Institute of Technology

# most platforms should have OpenGL already available
# debian linux : libgl1-mesa-dev (not that installing GLEW installs this as a dependency)
find_package(OpenGL REQUIRED)

add_library(the_opengl INTERFACE)
target_link_libraries(the_opengl INTERFACE ${OPENGL_LIBRARIES})

if(EMSCRIPTEN)
    # We want WebGL 2
    # WebGL 2 is almost the same as OpenGL ES 3 which is almost the same as OpenGL 3.3
    # MIN_WEBGL_VERSION=2           - Specifies the lowest WebGL version to target. Pass MIN_WEBGL_VERSION=2 to drop support for WebGL 1.0
    # MAX_WEBGL_VERSION=2           - Specifies the highest WebGL version to target. Pass -sMAX_WEBGL_VERSION=2 to enable targeting WebGL 2.
    # https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html
    
    target_link_options(the_opengl INTERFACE -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2)
    target_compile_definitions(the_opengl INTERFACE IS_WEBGL2)

endif()

