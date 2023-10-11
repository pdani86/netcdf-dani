cmake_minimum_required(VERSION 3.20)
project(netcdf_dani)
set(CMAKE_CXX_STANDARD 17)

set(NETCDF_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/deps/include)
set(NETCDF_LIB_DIR ${CMAKE_SOURCE_DIR}/deps/lib)

include_directories(
        ${NETCDF_INCLUDE_DIR}
)

add_executable(netcdf_dani
        src/main.cpp
)

target_link_directories(netcdf_dani
        PUBLIC ${NETCDF_LIB_DIR}
)

target_link_libraries(netcdf_dani
        libhdf5
        netcdf
)