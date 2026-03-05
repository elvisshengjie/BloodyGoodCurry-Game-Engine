# author Rudy Castan
# date 2025 Spring
# CS250 Computer Graphics II
# copyright DigiPen Institute of Technology

CachedFetchContent_Declare(
    glm
    URL https://github.com/g-truc/glm/releases/download/1.0.1/glm-1.0.1-light.7z
    URL_HASH SHA256=5f9b8ddd74ff6bdff6403877d492665c57b70cff019c59fbce86e205667fbd16
    SOURCE_DIR _deps/the_glm-src/glm
)
FetchContent_MakeAvailable(glm)

add_library(the_glm INTERFACE)
target_include_directories(the_glm SYSTEM INTERFACE ${glm_SOURCE_DIR}/..)
