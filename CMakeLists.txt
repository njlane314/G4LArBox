#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.3 FATAL_ERROR)
project(G4LArBox)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

find_package(Geant4 REQUIRED)
find_package(ROOT REQUIRED COMPONENTS RIO Core)
find_package(CLHEP REQUIRED)

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
include(${Geant4_USE_FILE})
include(${ROOT_USE_FILE})
#include(${CLHEP_USE_FILE})

message(STATUS "Geant4 GDML SUPPORT: ${Geant4_gdml_FOUND}")

#----------------------------------------------------------------------------
# Setup Projects PATHS
set(MAIN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(MAIN_SOURCE_DIR "${PROJECT_SOURCE_DIR}/source")
set(MAIN_BUILD_DIR "${PROJECT_SOURCE_DIR}/build")

if (NOT MAIN_EXTERNAL_DIR)
  set(MAIN_EXTERNAL_DIR "${PROJECT_SOURCE_DIR}/extern" )
endif()

#----------------------------------------------------------------------------
# Check if Marley is already installed

set(MARLEY_DIR "${MAIN_EXTERNAL_DIR}/marley")
set(MARLEY_INSTALLED FALSE)

file(GLOB MARLEY_FILES "${MARLEY_DIR}/*")

if (MARLEY_FILES)
  message(STATUS "Marley is already installed at ${MARLEY_DIR}")
  set(MARLEY_INSTALLED TRUE)
endif()

#----------------------------------------------------------------------------
# Clone and build Marley if not already installed

if (NOT MARLEY_INSTALLED)
  message(STATUS "Cloning and building Marley...")

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E env git clone https://github.com/njlane314/marley.git ${MARLEY_DIR}
    WORKING_DIRECTORY ${MAIN_EXTERNAL_DIR}
  )

  execute_process(
    COMMAND make
    WORKING_DIRECTORY ${MAIN_EXTERNAL_DIR}/marley/build
  )
endif()

#----------------------------------------------------------------------------
#----------------------------------------------------------------------------
#----------------------------------------------------------------------------
# Find libraries and include directories

find_library(MARLEY
  NAMES MARLEY
  PATHS "${MARLEY_DIR}/build"
  REQUIRED
  NO_DEFAULT_PATH
)

find_library(MARLEY_ROOT
  NAMES MARLEY_ROOT
  PATHS "${MARLEY_DIR}/build"
  REQUIRED
  NO_DEFAULT_PATH
)

file(GLOB SOURCES   ${MAIN_SOURCE_DIR}/*.cc
)

file(GLOB HEADERS   ${MAIN_INCLUDE_DIR}/*.hh
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
                            ${MAIN_INCLUDE_DIR}
                            ${MARLEY_DIR}/include
)

target_link_libraries(${PROJECT_NAME} ${Geant4_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${ROOT_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${CLHEP_LIBRARIES})

target_link_libraries(${PROJECT_NAME} ${MARLEY} ${MARLEY_ROOT})