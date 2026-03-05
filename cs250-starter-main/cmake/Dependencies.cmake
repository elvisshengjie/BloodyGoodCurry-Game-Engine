# author Rudy Castan
# date 2025 Spring
# CS250 Computer Graphics II
# copyright DigiPen Institute of Technology
include(cmake/CachedFetchContent.cmake)

include(cmake/dependencies/OpenGL.cmake)    #defines target the_opengl
include(cmake/dependencies/GLEW.cmake)      # defines target the_glew
include(cmake/dependencies/SDL2.cmake)      # defines target the_sdl2
include(cmake/dependencies/DearImGUI.cmake) # defines target the_imgui   ;  note DearImGUI.cmake depends on SDL2.cmake
include(cmake/dependencies/STB.cmake)       # defines target the_stb
include(cmake/dependencies/GSL.cmake)       # defines target the_gsl
include(cmake/dependencies/GLM.cmake)       # defines target the_glm

add_library(dependencies INTERFACE)

target_link_libraries(dependencies INTERFACE 
    the_opengl
    the_glew
    the_sdl2
    the_imgui
    the_stb
    the_gsl
    the_glm
)
