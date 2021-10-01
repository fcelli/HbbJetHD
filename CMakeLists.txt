cmake_minimum_required(VERSION 3.17)

project(HbbJetHD
    DESCRIPTION "Hbb+Jet Histogram Dumper"
)

add_library(${PROJECT_NAME} SHARED
    src/HistDumper.cpp
    src/ArgParser.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER include/HistDumper/HistDumper.h
    PUBLIC_HEADER include/HistDumper/ArgParser.h
)
target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(ROOT REQUIRED COMPONENTS RooFit RooFitCore)
include(${ROOT_USE_FILE})

add_subdirectory(apps)