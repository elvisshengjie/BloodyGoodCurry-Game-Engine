# author Rudy Castan
# date 2026 Spring
# CS250 Computer Graphics II
# copyright DigiPen Institute of Technology

# Provides caching functionality for FetchContent URL downloads
# Downloads are cached in OS-appropriate locations and reused across builds

# Initialize cache directory
if(NOT DEFINED CACHED_FETCHCONTENT_BASE_DIR)
    # Set cache location based on OS
    if(WIN32)
        # Windows: Use %LOCALAPPDATA%\cmake-downloads
        set(CACHED_FETCHCONTENT_BASE_DIR "$ENV{LOCALAPPDATA}/cmake-downloads" CACHE PATH "Base directory for cached downloads")
    else()
        # Linux/Mac: Use ~/.cache/cmake-downloads
        set(CACHED_FETCHCONTENT_BASE_DIR "$ENV{HOME}/.cache/cmake-downloads" CACHE PATH "Base directory for cached downloads")
    endif()
    
    file(MAKE_DIRECTORY "${CACHED_FETCHCONTENT_BASE_DIR}")
    message(STATUS "CachedFetchContent cache directory: ${CACHED_FETCHCONTENT_BASE_DIR}")
endif()

# Cached URL-based FetchContent_Declare
# Usage:
#   CachedFetchContent_Declare(
#       <name>
#       URL <url>
#       URL_HASH <hash_algo>=<hash_value>
#       [FILENAME <custom_filename>]    # Optional: custom cache filename
#       [<other_FetchContent_options>]  # Any other FetchContent_Declare options
#   )
function(CachedFetchContent_Declare name)
    
    # Parse arguments
    set(options "")
    set(oneValueArgs URL URL_HASH FILENAME)
    set(multiValueArgs "")
    cmake_parse_arguments(CACHE_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Validate required arguments
    if(NOT CACHE_ARG_URL)
        message(FATAL_ERROR "CachedFetchContent_Declare: URL is required")
    endif()
    if(NOT CACHE_ARG_URL_HASH)
        message(FATAL_ERROR "CachedFetchContent_Declare: URL_HASH is required")
    endif()
    
    # Extract hash algorithm and value
    string(REGEX MATCH "^([^=]+)=(.+)$" _match "${CACHE_ARG_URL_HASH}")
    if(NOT _match)
        message(FATAL_ERROR "CachedFetchContent_Declare: Invalid URL_HASH format. Expected ALGO=value")
    endif()
    set(HASH_ALGO "${CMAKE_MATCH_1}")
    set(HASH_VALUE "${CMAKE_MATCH_2}")
    
    # Determine cache filename
    if(CACHE_ARG_FILENAME)
        set(CACHE_FILENAME "${CACHE_ARG_FILENAME}")
    else()
        # Extract filename from URL
        get_filename_component(CACHE_FILENAME "${CACHE_ARG_URL}" NAME)
    endif()
    
    set(CACHED_FILE "${CACHED_FETCHCONTENT_BASE_DIR}/${CACHE_FILENAME}")
    
    # Check if cached file exists and hash matches
    set(USE_CACHED_FILE FALSE)
    if(EXISTS "${CACHED_FILE}")
        file(${HASH_ALGO} "${CACHED_FILE}" CACHED_FILE_HASH)
        if(CACHED_FILE_HASH STREQUAL HASH_VALUE)
            message(STATUS "Using cached ${name} from: ${CACHED_FILE}")
            set(USE_CACHED_FILE TRUE)
            set(FINAL_URL "file://${CACHED_FILE}")
        else()
            message(STATUS "Cached ${name} hash mismatch, will re-download")
            file(REMOVE "${CACHED_FILE}")
        endif()
    endif()
    
    # Download from internet if not cached
    if(NOT USE_CACHED_FILE)
        message(STATUS "Downloading ${name} to cache: ${CACHED_FILE}")
        file(DOWNLOAD "${CACHE_ARG_URL}" "${CACHED_FILE}"
            EXPECTED_HASH ${HASH_ALGO}=${HASH_VALUE}
            STATUS DOWNLOAD_STATUS
            SHOW_PROGRESS
        )
        list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
        if(NOT STATUS_CODE EQUAL 0)
            list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
            message(FATAL_ERROR "Failed to download ${name}: ${ERROR_MESSAGE}")
        endif()
        set(FINAL_URL "file://${CACHED_FILE}")
    endif()
    
    # Build the FetchContent_Declare call with all unparsed arguments
    # Remove URL and URL_HASH from unparsed args since we're replacing them
    set(FETCH_ARGS "")
    set(skip_next FALSE)
    foreach(arg ${ARGN})
        if(skip_next)
            set(skip_next FALSE)
            continue()
        endif()
        
        if(arg STREQUAL "URL" OR arg STREQUAL "URL_HASH" OR arg STREQUAL "FILENAME")
            set(skip_next TRUE)
            continue()
        endif()
        
        list(APPEND FETCH_ARGS ${arg})
    endforeach()
    
    # Call FetchContent_Declare with cached URL
    FetchContent_Declare(
        ${name}
        URL ${FINAL_URL}
        URL_HASH ${HASH_ALGO}=${HASH_VALUE}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        ${FETCH_ARGS}
    )
endfunction()
