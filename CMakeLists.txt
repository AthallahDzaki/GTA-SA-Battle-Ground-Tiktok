cmake_minimum_required(VERSION 3.30)
project(GTASABattleGroundTikTok VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard (C++23 required for std::expected in safetyhook used by plugin-sdk)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer Ninja generator
if(NOT CMAKE_GENERATOR)
    set(CMAKE_GENERATOR "Ninja")
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Build plugin-sdk as subdirectory (only SA)
set(PSDK_BUILD_VC OFF CACHE BOOL "" FORCE)
set(PSDK_BUILD_III OFF CACHE BOOL "" FORCE)
set(PSDK_BUILD_SA ON CACHE BOOL "" FORCE)
add_subdirectory(libs/plugin-sdk)

# Find Boost (required by websocketpp)
find_package(Boost REQUIRED COMPONENTS system)

# Collect source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.h
)

# Create shared library (.asi is a renamed .dll for GTA SA)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Set output name to .asi
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
    OUTPUT_NAME "GTASABattleGroundTikTok"
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/websocketpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    plugin_sa
    Boost::system
    ws2_32  # Windows Sockets
)

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    GTASA
    _CRT_SECURE_NO_WARNINGS
    _WINSOCK_DEPRECATED_NO_WARNINGS
    ASIO_STANDALONE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

# Copy config.ini to output directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.ini
    COPYONLY
)

# Install rules
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION bin
    RUNTIME DESTINATION bin
)
install(FILES config.ini DESTINATION bin)
