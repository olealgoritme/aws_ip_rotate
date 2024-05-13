cmake_minimum_required(VERSION 3.10)
project(AWSIPRotateLibrary)

# C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# set external include and link directories based on install location
include_directories(/opt/homebrew/include)
link_directories(/opt/homebrew/lib)

# build target executable and library
add_executable(wrapped src/wrapped.c src/ApiGateway.cpp src/ApiGatewayWrapper.cpp)
add_library(aws-ip-rotate SHARED src/ApiGateway.cpp src/ApiGatewayWrapper.cpp)

target_link_libraries(wrapped aws-cpp-sdk-apigateway aws-cpp-sdk-core curl pthread)
target_link_libraries(aws-ip-rotate PUBLIC aws-cpp-sdk-apigateway aws-cpp-sdk-core curl pthread)

# install
install(TARGETS aws-ip-rotate
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES include/ApiGateway.hpp include/ApiGatewayWrapper.h DESTINATION include)
