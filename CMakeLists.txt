cmake_minimum_required (VERSION 3.11)
project(logic2_spi_multics)

add_definitions( -DLOGIC2 )

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum supported MacOS version" FORCE)

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/SpiAnalyzer.cpp
src/SpiAnalyzer.h
src/SpiAnalyzerResults.cpp
src/SpiAnalyzerResults.h
src/SpiAnalyzerSettings.cpp
src/SpiAnalyzerSettings.h
src/SpiSimulationDataGenerator.cpp
src/SpiSimulationDataGenerator.h
)

add_analyzer_plugin(logic2_spi_multics SOURCES ${SOURCES})

# Separate plugin: its legacy frames never overlap the SPI analyzer frames.
set(MUX_SELECTION_SOURCES
   src/MuxSelectionAnalyzer.cpp
)
add_analyzer_plugin(logic2_mux_selection SOURCES ${MUX_SELECTION_SOURCES})
