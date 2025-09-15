cmake_minimum_required(VERSION 3.20)

# Create the game executable
add_executable(MyGame)

# Automatically collect source files from MyGame folder
file(GLOB_RECURSE MYGAME_SRC
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/MyGame/*.[cC]pp
)

file(GLOB_RECURSE MYGAME_HDR
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/MyGame/*.[hH]
  ${CMAKE_CURRENT_LIST_DIR}/MyGame/*.hpp
)

# Attach files to MyGame
target_sources(MyGame PRIVATE ${MYGAME_SRC} ${MYGAME_HDR})

# Link against the engine
target_link_libraries(MyGame PRIVATE MyEngine)

# Includes for MyGame headers
target_include_directories(MyGame PRIVATE ${CMAKE_CURRENT_LIST_DIR}/MyGame)

# C++ standard & warnings
set_property(TARGET MyGame PROPERTY CXX_STANDARD 20)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(MyGame PRIVATE -Wall -Wextra)
elseif (MSVC)
  target_compile_options(MyGame PRIVATE /W3 /WX-)
endif()

# Optional: Nice grouping in IDEs
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/MyGame FILES ${MYGAME_SRC} ${MYGAME_HDR})
