cmake_minimum_required(VERSION 3.20)
project(nozzle-sokol C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

option(NOOZZLE_SOKOL_BUILD_EXAMPLES "Build example applications" OFF)

add_library(nozzle-sokol INTERFACE)
target_include_directories(nozzle-sokol INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sokol
)

if(NOOZZLE_SOKOL_BUILD_EXAMPLES)
    set(NOZZLE_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    add_subdirectory(nozzle)

    if(APPLE)
        set(SOKOL_BACKEND "METAL")
    elseif(WIN32)
        set(SOKOL_BACKEND "D3D11")
    else()
        set(SOKOL_BACKEND "GLCORE")
    endif()

    add_executable(sender examples/sender.c)
    target_link_libraries(sender PRIVATE nozzle-sokol nozzle)
    target_include_directories(sender PRIVATE ${nozzle_SOURCE_DIR}/include)

    add_executable(receiver examples/receiver.c)
    target_link_libraries(receiver PRIVATE nozzle-sokol nozzle)
    target_include_directories(receiver PRIVATE ${nozzle_SOURCE_DIR}/include)

    if(APPLE)
        find_library(FOUNDATION Foundation)
        find_library(METAL Metal)
        find_library(APPKIT AppKit)
        find_library(IOSURFACE IOSurface)
        target_link_libraries(sender PRIVATE ${FOUNDATION} ${METAL} ${APPKIT} ${IOSURFACE})
        target_link_libraries(receiver PRIVATE ${FOUNDATION} ${METAL} ${APPKIT} ${IOSURFACE})
        set_target_properties(sender receiver PROPERTIES
            OBJCXX_STANDARD 17
            MACOSX_DEPLOYMENT_TARGET 12.0
        )
    endif()

    if(WIN32)
        target_link_libraries(sender PRIVATE d3d11 dxgi dxguid)
        target_link_libraries(receiver PRIVATE d3d11 dxgi dxguid)
    endif()
endif()
