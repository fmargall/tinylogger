enable_testing()

include(FetchContent)

# Set CMake policy CMP0135 to NEW, to ensure that files extracted from downloaded archives
# have their timestamps set to the time of extraction, not the original archive timestamps
# This avoids potential rebuild issues and silences related CMake warnings.
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# Download GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

# Disable unnecessary parts
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Collect all test source files in this directory
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

# Create a test executable
add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME}_tests PRIVATE
                           ${CMAKE_SOURCE_DIR}/include)

# Link libraries
target_link_libraries(${PROJECT_NAME}_tests PRIVATE
    gtest_main
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)