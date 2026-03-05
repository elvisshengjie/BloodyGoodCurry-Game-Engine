# author Rudy Castan
# date 2025 Spring
# CS250 Computer Graphics II
# copyright DigiPen Institute of Technology

# STB commit hash - latest version as of 2/2/2026
set(STB_COMMIT f1c79c0)

# Setup STB build directory
set(STB_DOWNLOAD_DIR ${FETCHCONTENT_BASE_DIR}/stb_headers)
file(MAKE_DIRECTORY ${STB_DOWNLOAD_DIR})

# Initialize implementation code tracker
set(STB_IMPLEMENTATION_SOURCE_CODE "")

# Function to download an STB file with hash verification
# Usage: download_stb_file(FILE filename SHA256 sha256_hash)
# Implementation macros are auto-generated for .h files
function(download_stb_file)
    cmake_parse_arguments(STB "" "FILE;SHA256" "" ${ARGN})
    
    # Construct cache path locally
    set(STB_CACHE_DIR ${CACHED_FETCHCONTENT_BASE_DIR}/stb_headers/${STB_COMMIT})
    file(MAKE_DIRECTORY ${STB_CACHE_DIR})
    
    set(CACHED_FILE "${STB_CACHE_DIR}/${STB_FILE}")
    set(BUILD_FILE "${STB_DOWNLOAD_DIR}/${STB_FILE}")
    
    # Check if file exists in cache and verify hash
    set(USE_CACHED_FILE FALSE)
    if(EXISTS "${CACHED_FILE}")
        file(SHA256 "${CACHED_FILE}" CACHED_FILE_HASH)
        if(CACHED_FILE_HASH STREQUAL STB_SHA256)
            message(STATUS "Using cached STB file: ${STB_FILE}")
            set(USE_CACHED_FILE TRUE)
        else()
            message(STATUS "Cached ${STB_FILE} hash mismatch, will re-download")
            file(REMOVE "${CACHED_FILE}")
        endif()
    endif()
    
    # Download from internet to cache if not cached
    if(NOT USE_CACHED_FILE)
        message(STATUS "Downloading ${STB_FILE} to cache: ${CACHED_FILE}")
        file(DOWNLOAD
            https://raw.githubusercontent.com/nothings/stb/${STB_COMMIT}/${STB_FILE}
            ${CACHED_FILE}
            EXPECTED_HASH SHA256=${STB_SHA256}
            SHOW_PROGRESS
        )
    endif()
    
    # Copy from cache to build directory using file:// URL
    file(DOWNLOAD
        file://${CACHED_FILE}
        ${BUILD_FILE}
        EXPECTED_HASH SHA256=${STB_SHA256}
    )
    
    # Auto-generate implementation macro for .h files
    if(STB_FILE MATCHES "\\.h$")
        # Strip extension and convert to uppercase macro name
        string(REGEX REPLACE "\\.[^.]*$" "" FILENAME_NO_EXT "${STB_FILE}")
        string(TOUPPER "${FILENAME_NO_EXT}" MACRO_NAME)
        string(REPLACE "." "_" MACRO_NAME "${MACRO_NAME}")
        set(STB_IMPLEMENTATION_SOURCE_CODE "${STB_IMPLEMENTATION_SOURCE_CODE}    #define ${MACRO_NAME}_IMPLEMENTATION\n    #include \"${STB_FILE}\"\n" PARENT_SCOPE)
    else()
        # For .c files, just include without implementation define
        set(STB_IMPLEMENTATION_SOURCE_CODE "${STB_IMPLEMENTATION_SOURCE_CODE}    #include \"${STB_FILE}\"\n" PARENT_SCOPE)
    endif()
endfunction()

# Define which STB files to download
# Format: download_stb_file(FILE filename SHA256 hash)
# To add/remove files, simply add/remove lines below
download_stb_file(
    FILE stb_image.h
    SHA256 594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3
)

# download_stb_file(
#     FILE stb_image_write.h
#     SHA256 cbd5f0ad7a9cf4468affb36354a1d2338034f2c12473cf1a8e32053cb6914a05
# )

# download_stb_file(
#     FILE stb_truetype.h
#     SHA256 ecd30b05e0dd4fea3a13c26810dd9e1992dc379049482c393d5a19e6b5090aab
# )

# download_stb_file(
#     FILE stb_vorbis.c
#     SHA256 4c7cb2ff1f7011e9d67950446b7eb9ca044f2e464d76bfbb0b84dd2e23e65636
# )

# Auto-generate implementation file from downloaded files
set(STB_IMPLEMENTATION_SOURCE_CODE "// This file is auto-generated from cmake/dependencies/STB.cmake\n${STB_IMPLEMENTATION_SOURCE_CODE}")

# Only write file if content has changed to avoid unnecessary rebuilds
set(STB_IMPL_FILE ${STB_DOWNLOAD_DIR}/stb_implementation.cpp)
if(EXISTS ${STB_IMPL_FILE})
    file(READ ${STB_IMPL_FILE} EXISTING_CONTENT)
    if(NOT "${EXISTING_CONTENT}" STREQUAL "${STB_IMPLEMENTATION_SOURCE_CODE}")
        file(WRITE ${STB_IMPL_FILE} ${STB_IMPLEMENTATION_SOURCE_CODE})
    endif()
else()
    file(WRITE ${STB_IMPL_FILE} ${STB_IMPLEMENTATION_SOURCE_CODE})
endif()
set_source_files_properties(${STB_IMPL_FILE} PROPERTIES GENERATED TRUE)

add_library(the_stb STATIC ${STB_IMPL_FILE})
target_include_directories(the_stb SYSTEM PUBLIC ${STB_DOWNLOAD_DIR})

