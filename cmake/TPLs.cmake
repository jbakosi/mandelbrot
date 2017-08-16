################################################################################
#
# \file      cmake/TPLs.cmake
# \copyright 2016-2017, Los Alamos National Security, LLC.
# \brief     Find the third-party libraries required to build Mandelbrot
#
################################################################################

# Add TPL_DIR to modules directory for TPLs that provide cmake FIND_PACKAGE
# code, such as Trilinos
SET(CMAKE_PREFIX_PATH ${TPL_DIR} ${CMAKE_PREFIX_PATH})

# Include support for multiarch path names
include(GNUInstallDirs)

#### TPLs we attempt to find on the system #####################################

message(STATUS "------------------------------------------")

#### Charm++
set(CHARM_ROOT ${TPL_DIR}/charm)
find_package(Charm REQUIRED)

#### Boost
set(BOOST_INCLUDEDIR ${TPL_DIR}/include) # prefer ours
find_package(Boost)
if(Boost_FOUND)
  message(STATUS "Boost at ${Boost_INCLUDE_DIR} (include)")
  include_directories(${Boost_INCLUDE_DIR})
endif()

#### LibJPEG
find_package(JPEG REQUIRED)

message(STATUS "------------------------------------------")
