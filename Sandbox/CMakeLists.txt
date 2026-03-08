cmake_minimum_required(VERSION 3.20)

# Create the game executable
add_executable(BloodyGoodCurry)

set(GAME_NAME BloodyGoodCurry)
set(GAME_PROJECT_ROOT "${CMAKE_SOURCE_DIR}/Games/${GAME_NAME}")
set(GAME_PROJECT_METADATA_FILE "${CMAKE_CURRENT_BINARY_DIR}/${GAME_NAME}.project_root.txt")
file(GENERATE OUTPUT "${GAME_PROJECT_METADATA_FILE}" CONTENT "${GAME_PROJECT_ROOT}\n")

if (EXISTS "${GAME_PROJECT_ROOT}/Assets" AND EXISTS "${GAME_PROJECT_ROOT}/Data")
  set(GAME_ASSETS_SOURCE "${GAME_PROJECT_ROOT}/Assets")
  set(GAME_DATA_SOURCE "${GAME_PROJECT_ROOT}/Data")
else()
  set(GAME_ASSETS_SOURCE "${CMAKE_SOURCE_DIR}/assets")
  set(GAME_DATA_SOURCE "${CMAKE_SOURCE_DIR}/Data_Files")
endif()

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

# Attach files to BloodyGoodCurry
target_sources(BloodyGoodCurry PRIVATE ${MYGAME_SRC} ${MYGAME_HDR})

# Link against the engine
target_link_libraries(BloodyGoodCurry PRIVATE MyEngine)

if(EMSCRIPTEN)
    if(NOT DEFINED SOFASPUDS_WEB_SINGLE_FILE)
        set(SOFASPUDS_WEB_SINGLE_FILE ON)
    endif()

    set_target_properties(BloodyGoodCurry PROPERTIES SUFFIX ".html")
    target_link_options(BloodyGoodCurry PRIVATE
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sDISABLE_EXCEPTION_CATCHING=0
        -sEXIT_RUNTIME=1
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sGL_ENABLE_GET_PROC_ADDRESS=1
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/app_resources/web/index_shell.html"
    )
    if(SOFASPUDS_WEB_SINGLE_FILE)
        target_link_options(BloodyGoodCurry PRIVATE
            "SHELL:--embed-file ${GAME_ASSETS_SOURCE}@/Assets"
            "SHELL:--embed-file ${GAME_DATA_SOURCE}@/Data"
        )
        target_link_options(BloodyGoodCurry PRIVATE -sSINGLE_FILE=1)
        message(STATUS "[SofaSpuds] Web packaging: SINGLE_FILE=ON (slow link/load, single output HTML)")
    else()
        # Faster dev mode: keep wasm small and place assets/data in a separate .data package.
        target_link_options(BloodyGoodCurry PRIVATE
            "SHELL:--preload-file ${GAME_ASSETS_SOURCE}@/Assets"
            "SHELL:--preload-file ${GAME_DATA_SOURCE}@/Data"
        )
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_link_options(BloodyGoodCurry PRIVATE
                -sASSERTIONS=2
                -sSAFE_HEAP=1
                -sSTACK_OVERFLOW_CHECK=2
            )
        endif()
        message(STATUS "[SofaSpuds] Web packaging: SINGLE_FILE=OFF (faster wasm compile, separate .data)")
    endif()
endif()

#  Console Window Management (MSVC specific)
# Debug -> Show Console. Release -> Hide Console.
if (MSVC)
    target_link_options(BloodyGoodCurry PRIVATE
        # Debug: Keep Console
        $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>
        
        # Release: Hide Console (Windows Subsystem)
        $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
        $<$<CONFIG:RelWithDebInfo>:/SUBSYSTEM:WINDOWS>
        $<$<CONFIG:MinSizeRel>:/SUBSYSTEM:WINDOWS>

        # Fix entry point for Release so 'int main()' still works without WinMain
        $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>
        $<$<CONFIG:RelWithDebInfo>:/ENTRY:mainCRTStartup>
        $<$<CONFIG:MinSizeRel>:/ENTRY:mainCRTStartup>
    )
endif()

if(NOT SOFASPUDS_DISABLE_AUDIO)
    # ------------------------------
    # FMOD setup
    # ------------------------------
    set(FMOD_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmod/inc)
    set(FMOD_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmod/lib/x64)

    # Include FMOD headers
    target_include_directories(BloodyGoodCurry PRIVATE ${FMOD_INCLUDE_DIR})

    # Link FMOD libraries
    target_link_directories(BloodyGoodCurry PRIVATE ${FMOD_LIBRARY_DIR})
    target_link_libraries(BloodyGoodCurry PRIVATE fmod_vc)
endif()

# Includes for headers
target_include_directories(BloodyGoodCurry PRIVATE ${CMAKE_CURRENT_LIST_DIR}/MyGame)

# C++ standard & warnings
set_property(TARGET BloodyGoodCurry PROPERTY CXX_STANDARD 20)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(BloodyGoodCurry PRIVATE -Wall -Wextra)
elseif (MSVC)
  target_compile_options(BloodyGoodCurry PRIVATE /W4 /WX-)
endif()

if(NOT SOFASPUDS_DISABLE_AUDIO)
    # Copy FMOD DLL files to build directory
    set(FMOD_DLL_DIR "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmod/lib/x64")
    set(FMOD_DLL_FILE "${FMOD_DLL_DIR}/fmod.dll")

    # Copy FMOD DLL after build
    add_custom_command(TARGET BloodyGoodCurry POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FMOD_DLL_FILE}"
        "$<TARGET_FILE_DIR:BloodyGoodCurry>/"
        COMMENT "Copying FMOD DLL to build directory"
        VERBATIM
    )
endif()

add_custom_command(TARGET BloodyGoodCurry POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${GAME_PROJECT_METADATA_FILE}"
    "$<TARGET_FILE_DIR:BloodyGoodCurry>/sofaspuds_project_root.txt"
    COMMENT "Copying project root metadata"
    VERBATIM
)

if(NOT EMSCRIPTEN)
    # Copy runtime folders next to exe
    add_custom_command(TARGET BloodyGoodCurry POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:BloodyGoodCurry>/assets"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:BloodyGoodCurry>/Assets"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:BloodyGoodCurry>/Data_Files"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:BloodyGoodCurry>/Data"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:BloodyGoodCurry>/logs"

        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BloodyGoodCurry>/Assets"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BloodyGoodCurry>/Data"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BloodyGoodCurry>/logs"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${GAME_ASSETS_SOURCE}"
                "$<TARGET_FILE_DIR:BloodyGoodCurry>/Assets"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${GAME_DATA_SOURCE}"
                "$<TARGET_FILE_DIR:BloodyGoodCurry>/Data"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/logs"
                "$<TARGET_FILE_DIR:BloodyGoodCurry>/logs"

        COMMENT "Reset & Copy Assets, Data, logs"
        VERBATIM
    )
endif()

# Nice grouping in IDEs
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/MyGame FILES ${MYGAME_SRC} ${MYGAME_HDR})
