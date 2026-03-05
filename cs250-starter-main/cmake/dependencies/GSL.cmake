# author Rudy Castan
# date 2025 Spring
# CS250 Computer Graphics II
# copyright DigiPen Institute of Technology

CachedFetchContent_Declare(
    gsl
    URL https://github.com/microsoft/GSL/archive/refs/tags/v4.2.1.tar.gz
    URL_HASH SHA256=d959f1cb8bbb9c94f033ae5db60eaf5f416be1baa744493c32585adca066fe1f
)
FetchContent_MakeAvailable(gsl)

add_library(the_gsl INTERFACE)
target_include_directories(the_gsl SYSTEM INTERFACE ${gsl_SOURCE_DIR}/include)
