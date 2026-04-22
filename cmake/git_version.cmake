# Get git commit hash for version embedding
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# If git failed (no repo), use unknown
if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()

# Get git commit count for build number
execute_process(
    COMMAND git rev-list --count HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_COUNT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT GIT_COMMIT_COUNT)
    set(GIT_COMMIT_COUNT "0")
endif()

# Get current branch name
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT GIT_BRANCH)
    set(GIT_BRANCH "detached")
endif()

# Build full version string: 0.5.5+abc123 (commit hash)
set(PZEP_VERSION_FULL "${PROJECT_VERSION}+${GIT_COMMIT_HASH}")

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/include/zep/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
    @ONLY
)
