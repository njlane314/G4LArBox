cmake_minimum_required(VERSION 3.20)
project(G4LArBox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}")

find_package(Geant4 REQUIRED)
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree)
find_package(CLHEP QUIET)
find_package(GSL REQUIRED)
include("${Geant4_USE_FILE}")

set(G4LARBOX_EXTERNAL_DIR "${PROJECT_SOURCE_DIR}/extern" CACHE PATH
    "Directory containing external generator libraries")
set(MARLEY_DIR "${G4LARBOX_EXTERNAL_DIR}/marley" CACHE PATH
    "MARLEY source directory")
set(BXDECAY0_DIR "${G4LARBOX_EXTERNAL_DIR}/bxdecay0" CACHE PATH
    "BxDecay0 source directory")
file(MAKE_DIRECTORY "${G4LARBOX_EXTERNAL_DIR}")

if(NOT EXISTS "${MARLEY_DIR}/build/libMARLEY${CMAKE_SHARED_LIBRARY_SUFFIX}" OR
   NOT EXISTS "${MARLEY_DIR}/build/libMARLEY_ROOT${CMAKE_SHARED_LIBRARY_SUFFIX}")
  if(NOT EXISTS "${MARLEY_DIR}/build/Makefile")
    message(STATUS "Cloning MARLEY")
    execute_process(
      COMMAND git clone --depth 1 https://github.com/njlane314/marley.git "${MARLEY_DIR}"
      RESULT_VARIABLE MARLEY_CLONE_STATUS
    )
    if(NOT MARLEY_CLONE_STATUS EQUAL 0)
      message(FATAL_ERROR "Failed to clone MARLEY into ${MARLEY_DIR}")
    endif()
  endif()

  message(STATUS "Building MARLEY")
  execute_process(
    COMMAND make -j2 "CXX_STD=c++${CMAKE_CXX_STANDARD}"
            "libMARLEY${CMAKE_SHARED_LIBRARY_SUFFIX}"
            "libMARLEY_ROOT${CMAKE_SHARED_LIBRARY_SUFFIX}"
    WORKING_DIRECTORY "${MARLEY_DIR}/build"
    RESULT_VARIABLE MARLEY_BUILD_STATUS
  )
  if(NOT MARLEY_BUILD_STATUS EQUAL 0)
    message(FATAL_ERROR "Failed to build MARLEY in ${MARLEY_DIR}")
  endif()
endif()

set(BXDECAY0_INCLUDE_DIR "${BXDECAY0_DIR}/_install.d/include")
set(BXDECAY0_LIBRARY_DIR "${BXDECAY0_DIR}/_install.d/lib")
set(BXDECAY0_RESOURCE_DIR "${BXDECAY0_DIR}/resources")
set(BXDECAY0_BUILD_DIR "${BXDECAY0_DIR}/_build.d")

if(NOT EXISTS "${BXDECAY0_INCLUDE_DIR}/bxdecay0/decay0_generator.h" OR
   NOT EXISTS "${BXDECAY0_LIBRARY_DIR}/libBxDecay0${CMAKE_SHARED_LIBRARY_SUFFIX}")
  if(NOT EXISTS "${BXDECAY0_DIR}/CMakeLists.txt")
    message(STATUS "Cloning BxDecay0")
    execute_process(
      COMMAND git clone --depth 1 https://github.com/BxCppDev/bxdecay0.git "${BXDECAY0_DIR}"
      RESULT_VARIABLE BXDECAY0_CLONE_STATUS
    )
    if(NOT BXDECAY0_CLONE_STATUS EQUAL 0)
      message(FATAL_ERROR "Failed to clone BxDecay0 into ${BXDECAY0_DIR}")
    endif()
  endif()

  message(STATUS "Building BxDecay0")
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
            -S "${BXDECAY0_DIR}"
            -B "${BXDECAY0_BUILD_DIR}"
            "-DCMAKE_INSTALL_PREFIX=${BXDECAY0_DIR}/_install.d"
            -DBXDECAY0_INSTALL_DBD_GA_DATA=OFF
            -DBXDECAY0_WITH_GEANT4_EXTENSION=OFF
    RESULT_VARIABLE BXDECAY0_CONFIGURE_STATUS
  )
  if(NOT BXDECAY0_CONFIGURE_STATUS EQUAL 0)
    message(FATAL_ERROR "Failed to configure BxDecay0 in ${BXDECAY0_DIR}")
  endif()

  execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${BXDECAY0_BUILD_DIR}" --parallel 2
    RESULT_VARIABLE BXDECAY0_BUILD_STATUS
  )
  if(NOT BXDECAY0_BUILD_STATUS EQUAL 0)
    message(FATAL_ERROR "Failed to build BxDecay0 in ${BXDECAY0_DIR}")
  endif()

  execute_process(
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${BXDECAY0_BUILD_DIR}"
            --output-on-failure
    RESULT_VARIABLE BXDECAY0_TEST_STATUS
  )
  if(NOT BXDECAY0_TEST_STATUS EQUAL 0)
    message(FATAL_ERROR "BxDecay0 tests failed in ${BXDECAY0_DIR}")
  endif()

  execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${BXDECAY0_BUILD_DIR}"
    RESULT_VARIABLE BXDECAY0_INSTALL_STATUS
  )
  if(NOT BXDECAY0_INSTALL_STATUS EQUAL 0)
    message(FATAL_ERROR "Failed to install BxDecay0 from ${BXDECAY0_DIR}")
  endif()
endif()

option(G4LARBOX_DEPENDENCIES_ONLY
       "Fetch and build generator dependencies without building G4LArBox" OFF)
if(G4LARBOX_DEPENDENCIES_ONLY)
  return()
endif()

find_library(MARLEY_LIBRARY MARLEY
  PATHS "${MARLEY_DIR}/build" REQUIRED NO_DEFAULT_PATH)
find_library(MARLEY_ROOT_LIBRARY MARLEY_ROOT
  PATHS "${MARLEY_DIR}/build" REQUIRED NO_DEFAULT_PATH)
find_library(BXDECAY0_LIBRARY BxDecay0
  PATHS "${BXDECAY0_LIBRARY_DIR}" "${BXDECAY0_DIR}/_build.d"
  REQUIRED NO_DEFAULT_PATH)

add_executable(G4LArBox
  source/ActionInitialisation.cc
  source/BulkVertexGenerator.cc
  source/BxDecay0Generator.cc
  source/CorsikaReader.cc
  source/DataHandler.cc
  source/DetectorConstruction.cc
  source/EventAction.cc
  source/GeneratorMessenger.cc
  source/GenieGSTReader.cc
  source/Main.cc
  source/MarleyGenerator.cc
  source/MediumResponse.cc
  source/Messenger.cc
  source/PhysicsList.cc
  source/PrimaryGeneratorAction.cc
  source/RunAction.cc
  source/SteppingAction.cc
  source/TrackingAction.cc
  source/VertexGeneratorInterface.cc
)

target_include_directories(G4LArBox PRIVATE
  "${PROJECT_SOURCE_DIR}/include"
  "${MARLEY_DIR}/include"
  "${BXDECAY0_INCLUDE_DIR}"
)

target_compile_definitions(G4LArBox PRIVATE
  G4LARBOX_MARLEY_DIR="${MARLEY_DIR}"
  G4LARBOX_BXDECAY0_RESOURCE_DIR="${BXDECAY0_RESOURCE_DIR}"
)

target_link_libraries(G4LArBox PRIVATE
  ${Geant4_LIBRARIES}
  ROOT::Core
  ROOT::RIO
  ROOT::Tree
  GSL::gsl
  GSL::gslcblas
  "${MARLEY_LIBRARY}"
  "${MARLEY_ROOT_LIBRARY}"
  "${BXDECAY0_LIBRARY}"
)

if(CLHEP_FOUND)
  target_link_libraries(G4LArBox PRIVATE ${CLHEP_LIBRARIES})
endif()
