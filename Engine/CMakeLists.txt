# Set the Engine
set(ENGINE_NAME MyEngine)

#Get the Source Files (cpp)
file(GLOB_RECURSE ENGINE_SRC_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/Collision_Tools/*.[cC]pp
    ${CMAKE_CURRENT_LIST_DIR}/Graphics/*.[cC]pp
    ${CMAKE_CURRENT_LIST_DIR}/Math/*.[cC]pp
)

#Get the Source Files (h) and explictly the hpp file (have to be specific)
file(GLOB_RECURSE ENGINE_HEADER_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/Collision_Tools/*.[hH]
    ${CMAKE_CURRENT_LIST_DIR}/Graphics/*.[hH]
    ${CMAKE_CURRENT_LIST_DIR}/Graphics/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/Math/*.[hH]
)

# --- Manual GLAD (vendored) ---
add_library(glad STATIC
    ${CMAKE_CURRENT_LIST_DIR}/ThirdParty/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/ThirdParty/glad/include
)
#Get the Source Files (h)
add_library(${ENGINE_NAME}
    ${ENGINE_SRC_FILES}
    ${ENGINE_HEADER_FILES} 
)


set(ALL_ENGINE_FILES ${ENGINE_SRC_FILES} ${ENGINE_HEADER_FILES})
if(COMMAND GROUP_FILES_BY_FOLDER)
    GROUP_FILES_BY_FOLDER(${ALL_ENGINE_FILES})
endif()


set_property(TARGET ${ENGINE_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${ENGINE_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)


# Expose top-level engine folder so headers are visible
target_include_directories(${ENGINE_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# Make sure glfw is added with add_subdirectory() or find_package()
target_link_libraries(${ENGINE_NAME} PUBLIC glfw glad stb_image glm::glm)

#Complier Warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${ENGINE_NAME} PRIVATE -Wall -Wextra)
elseif (MSVC)
    target_compile_options(${ENGINE_NAME} PRIVATE /W3 /WX-)
endif()

# Platform OpenGL (required)
if (WIN32)
  target_link_libraries(${ENGINE_NAME} PRIVATE opengl32)
elseif (APPLE)
  find_library(COCOA_LIBRARY Cocoa REQUIRED)
  find_library(OpenGL_LIBRARY OpenGL REQUIRED)
  target_link_libraries(${ENGINE_NAME} PRIVATE ${OpenGL_LIBRARY} ${COCOA_LIBRARY})
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(${ENGINE_NAME} PRIVATE OpenGL::GL)
endif()