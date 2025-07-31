cmake_minimum_required(VERSION 3.21)

# Set the project name and version. While the project name should not be modified,
# the version should be updated with each release, later automatically using CI/CD
project(tinylogger VERSION 1.0.0 LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add subdirectories
add_subdirectory(tests)