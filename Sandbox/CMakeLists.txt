cmake_minimum_required(VERSION 3.20)

set(GAME_NAME "${SOFASPUDS_GAME_NAME}")
set(GAME_TARGET "${GAME_NAME}")
set(GAME_CODE_ROOT "${CMAKE_CURRENT_LIST_DIR}/${SOFASPUDS_GAME_CODE_DIR}")

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/Games/${GAME_NAME}")
  message(FATAL_ERROR "Game project '${GAME_NAME}' does not exist under ${CMAKE_SOURCE_DIR}/Games.")
endif()
if(NOT EXISTS "${GAME_CODE_ROOT}")
  message(FATAL_ERROR "Game code directory '${GAME_CODE_ROOT}' does not exist.")
endif()

# Create the selected game executable
add_executable(${GAME_TARGET})

set(GAME_PROJECT_ROOT "${CMAKE_SOURCE_DIR}/Games/${GAME_NAME}")
set(GAME_PROJECT_METADATA_FILE "${CMAKE_CURRENT_BINARY_DIR}/${GAME_NAME}.project_root.txt")
file(GENERATE OUTPUT "${GAME_PROJECT_METADATA_FILE}" CONTENT "${GAME_PROJECT_ROOT}\n")

# Git does not preserve empty folders, so ensure a newly cloned game project
# always has the expected runtime content directories before build steps run.
file(MAKE_DIRECTORY
  "${GAME_PROJECT_ROOT}/Assets"
  "${GAME_PROJECT_ROOT}/Data"
  "${GAME_PROJECT_ROOT}/Saves"
)

set(GAME_ASSETS_SOURCE "${GAME_PROJECT_ROOT}/Assets")
set(GAME_DATA_SOURCE "${GAME_PROJECT_ROOT}/Data")

# Automatically collect source files from MyGame folder
file(GLOB_RECURSE MYGAME_SRC
  CONFIGURE_DEPENDS
  ${GAME_CODE_ROOT}/*.[cC]pp
)

file(GLOB_RECURSE MYGAME_HDR
  CONFIGURE_DEPENDS
  ${GAME_CODE_ROOT}/*.[hH]
  ${GAME_CODE_ROOT}/*.hpp
)

# Attach files to the selected game target
target_sources(${GAME_TARGET} PRIVATE ${MYGAME_SRC} ${MYGAME_HDR})

# Link against the engine
target_link_libraries(${GAME_TARGET} PRIVATE MyEngine)

if(EMSCRIPTEN)
    if(NOT DEFINED SOFASPUDS_WEB_SINGLE_FILE)
        set(SOFASPUDS_WEB_SINGLE_FILE ON)
    endif()

    # Build a browser entry point instead of a native .exe.
    # Emscripten will emit HTML plus the matching JS/wasm runtime files.
    set_target_properties(${GAME_TARGET} PROPERTIES SUFFIX ".html")
    target_link_options(${GAME_TARGET} PRIVATE
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sDISABLE_EXCEPTION_CATCHING=0
        -sEXIT_RUNTIME=1
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sGL_ENABLE_GET_PROC_ADDRESS=1
        # Use our custom HTML page shell instead of Emscripten's default page.
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/app_resources/web/index_shell.html"
    )
    if(SOFASPUDS_WEB_SINGLE_FILE)
        # Single-file mode embeds project assets/data directly into the output,
        # then inlines the generated JS/wasm into one large HTML file.
        target_link_options(${GAME_TARGET} PRIVATE
            "SHELL:--embed-file ${GAME_ASSETS_SOURCE}@/Assets"
            "SHELL:--embed-file ${GAME_DATA_SOURCE}@/Data"
        )
        target_link_options(${GAME_TARGET} PRIVATE -sSINGLE_FILE=1)
        message(STATUS "[SofaSpuds] Web packaging: SINGLE_FILE=ON (slow link/load, single output HTML)")
    else()
        # Split-file mode keeps the main wasm/html smaller and packages the game's
        # runtime files into a separate Emscripten data bundle.
        #
        # The @/Assets and @/Data mount points matter: when the game later calls
        # ResolveAssetPath(...) or ResolveDataPath(...), it reads from these virtual
        # filesystem locations created by Emscripten at startup.
        target_link_options(${GAME_TARGET} PRIVATE
            "SHELL:--preload-file ${GAME_ASSETS_SOURCE}@/Assets"
            "SHELL:--preload-file ${GAME_DATA_SOURCE}@/Data"
        )
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            # Extra runtime checks are useful in browser debug builds, where native
            # debugging tools are more limited than desktop.
            target_link_options(${GAME_TARGET} PRIVATE
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
    target_link_options(${GAME_TARGET} PRIVATE
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
    target_include_directories(${GAME_TARGET} PRIVATE ${FMOD_INCLUDE_DIR})

    # Link FMOD libraries
    target_link_directories(${GAME_TARGET} PRIVATE ${FMOD_LIBRARY_DIR})
    target_link_libraries(${GAME_TARGET} PRIVATE fmod_vc)
endif()

# Includes for headers
target_include_directories(${GAME_TARGET} PRIVATE ${GAME_CODE_ROOT})

# C++ standard & warnings
set_property(TARGET ${GAME_TARGET} PROPERTY CXX_STANDARD 20)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(${GAME_TARGET} PRIVATE -Wall -Wextra)
elseif (MSVC)
  target_compile_options(${GAME_TARGET} PRIVATE /W4 /WX-)
endif()

if(NOT SOFASPUDS_DISABLE_AUDIO)
    # Copy FMOD DLL files to build directory
    set(FMOD_DLL_DIR "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/fmod/lib/x64")
    set(FMOD_DLL_FILE "${FMOD_DLL_DIR}/fmod.dll")

    # Copy FMOD DLL after build
    add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FMOD_DLL_FILE}"
        "$<TARGET_FILE_DIR:${GAME_TARGET}>/"
        COMMENT "Copying FMOD DLL to build directory"
        VERBATIM
    )
endif()

add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${GAME_PROJECT_METADATA_FILE}"
    "$<TARGET_FILE_DIR:${GAME_TARGET}>/sofaspuds_project_root.txt"
    COMMENT "Copying project root metadata"
    VERBATIM
)

if(NOT EMSCRIPTEN)
    # Copy runtime folders next to exe
    add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${GAME_TARGET}>/assets"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${GAME_TARGET}>/Assets"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${GAME_TARGET}>/Data_Files"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${GAME_TARGET}>/Data"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${GAME_TARGET}>/logs"

        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${GAME_TARGET}>/Assets"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${GAME_TARGET}>/Data"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${GAME_TARGET}>/logs"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${GAME_ASSETS_SOURCE}"
                "$<TARGET_FILE_DIR:${GAME_TARGET}>/Assets"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${GAME_DATA_SOURCE}"
                "$<TARGET_FILE_DIR:${GAME_TARGET}>/Data"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/logs"
                "$<TARGET_FILE_DIR:${GAME_TARGET}>/logs"

        COMMENT "Reset & Copy Assets, Data, logs"
        VERBATIM
    )
endif()

# Nice grouping in IDEs
source_group(TREE ${GAME_CODE_ROOT} FILES ${MYGAME_SRC} ${MYGAME_HDR})
