################################################################################
#
# \file      tpl/CMakeLists.txt
# \copyright 2016-2017, Los Alamos National Security, LLC.
# \brief     Build third-party libraries for Mandelbrot
#
#################################################################################

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(TPL C CXX)
include(ExternalProject)

# Quinoa cmake modules directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

# Disallow in-source builds
include(DisallowInSourceBuilds)
disallow_in_source_builds()

# Set CMAKE_BUILD_TYPE
include(BuildType)

# Set BUILD_SHARED_LIBS
include(BuildShared)

# Conditionally enforce static linking for exeuctables added after this point.
# This must precede all add_executable() calls adding executables to link
# statically.
if (NOT BUILD_SHARED_LIBS)
  set(CMAKE_EXE_LINKER_FLAGS "-static")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

# Find MPI and get its underlying compilers
# The get_mpi_compilers cmake function uses the built-in CMake functionality to
# find the MPI wrappers and based on the wrappers detects the underyling
# compilers.
include(MPICompilers)
get_mpi_compilers()

include(ProcessorCount)
ProcessorCount(PROCESSOR_COUNT)
if(PROCESSOR_COUNT EQUAL 0)     # if ncpus could not be determined, assign 1
  set(PROCESSOR_COUNT "1")
endif()
message(STATUS "Found ${PROCESSOR_COUNT} PEs")

# Include support for multiarch path names
include(GNUInstallDirs)

# Set compiler id
string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" COMP)

# Detect operating system type and version
include(DetectOS)
detect_os()
message(STATUS "Detected OS: ${HOST_OS}")

# Query target architecture
include(TargetArch)
target_architecture(ARCH)
message(STATUS "Detected target architecture: ${ARCH}")

# Set install directory based on compiler ID if was not given on command line
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install/${COMP}-${ARCH}"
      CACHE STRING "Third-party install prefix." FORCE)
  message(STATUS "CMAKE_INSTALL_PREFIX not specified, setting to '${PROJECT_SOURCE_DIR}/install/${COMP}-${ARCH}'")
else()
  message(STATUS "CMAKE_INSTALL_PREFIX: " ${CMAKE_INSTALL_PREFIX})
endif()

# Compilers
# =========
# There are two sets of compilers, given by the following CMake variables:
#
# 1. The underlying compiler suite
#
#    UNDERLYING_C_COMPILER
#    UNDERLYING_CXX_COMPILER
#    UNDERLYING_Fortran_COMPILER
#
# 2. The MPI wrappers (of the underlying compilers)
#
#    MPI_C_COMPILER
#    MPI_CXX_COMPILER
#    MPI_Fortran_COMPILER

# Convert system name to lower case
string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME)

# Detect charm
find_package(Charm)
if(CHARM_FOUND)
  set(charm "")
else()
  set(charm "charm")
endif()

# If we are to build our own Charm++, setup prerequisites for its build command
if (charm)

  # Use mpi wrappers by default
  set(CHARM_COMPILER "")

  # Match Charm++ build architecture for system architecture detected
  if(ARCH MATCHES "x86_64")

    set(CHARM_ARCH "-${ARCH}")

  elseif(ARCH MATCHES "ppc64")

    set(SYSTEM_NAME "")
    set(CHARM_ARCH "bluegeneq")
    set(CHARM_COMPILER "clang")

  elseif(ARCH MATCHES "i386")

    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
      set(CHARM_ARCH "")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(CHARM_ARCH "-x86")
    endif()

  else()

    message(FATAL "Selecting a Charm++ architecture for ${ARCH} is not supported")

  endif()

  # Set Charm++ architecture
  if (NOT DEFINED ENV{CRAY_PRGENVPGI} AND
      NOT DEFINED ENV{CRAY_PRGENVGNU} AND
      NOT DEFINED ENV{CRAY_PRGENVCRAY} AND
      NOT DEFINED ENV{CRAY_PRGENVINTEL})
    set(CHARM_ARCHITECTURE "mpi-${SYSTEM_NAME}${CHARM_ARCH}")
  else()
    set(CHARM_ARCHITECTURE "mpi-crayxc")
  endif()

  message(STATUS "Charm++ architecture/version: '${CHARM_ARCHITECTURE}'")

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Cray")  # cray-specific settings
    set(CHARM_COMPILER "craycc")
    message(STATUS "Charm++ compiler: '${CHARM_COMPILER}'")
  endif()

endif(charm)

# Compiler-specific settings
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")  # clang-specific settings

  # Include function that allows to set standard C++ library
  include(libstdcxx)

  # Offer switch between libc++ and libstdc++, using libc++ as default
  if (LIBCXX_FOUND)
    set_libstdcpp_vs_libcpp(CXXFLAGS "libc++")
  endif()

  # Don't error out on functions not returning anything even when supposed to,
  # required only by TestU01 
  set(CFLAGS "${CFLAGS} -Wno-error=return-type")

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")  # gcc-specific settings

  # Gnu will use libstdc++ by default. We do not offer libc++.

  # Detect compiler version
  include(DetectCXXversion)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")  # intel-specific settings

  # Include function that allows to set standard C++ library
  include(libstdcxx)

  # Offer switch between libc++ and libstdc++, using libc++ as default
  if (LIBCXX_FOUND)
    set_libstdcpp_vs_libcpp(CXXFLAGS "libc++")
  endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "PGI")  # pgi-specific settings

  # Some configure scripts don't like pgi's preprocessor so use gnu's
  set(PGCPP "CPP=cpp")

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Cray")  # cray-specific settings

endif()

# Mac OS X specifics
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOSX_RPATH "-DCMAKE_MACOSX_RPATH:BOOL=ON")
endif()

# Attempt to find some TPLs on the system (some others we always build)

# Detect Boost, exclude if found, copy if not
if(NOT BUILD_SHARED_LIBS)
  set(Boost_USE_STATIC_LIBS true)
endif()
find_package(Boost)
if(Boost_FOUND)
  set(boost "")
else()
  set(boost "boost")
endif()

# Get compiler flags (CFLAGS, CXXFLAGS, FFLAGS) independent of CMAKE_BUILD_TYPE
# and echo flags that will be passed to all TPL builds
include(get_compiler_flags)
get_compiler_flags()

set(tpls2build ${charm} ${boost})

list(LENGTH tpls2build ntpl)

# Output TPLs to be built
if (tpls2build)
  message(STATUS "Will build the following ${ntpl} TPLs: ${tpls2build}")
else()
  message(STATUS "All TPLs found. No need to build.")
endif()

# Define fine-grained targets
set_property(DIRECTORY PROPERTY EP_STEP_TARGETS
             download patch configure build install test)

#### Charm++ ###################################################################
# http://charm.cs.illinois.edu
if(BUILD_SHARED_LIBS)
  set(CHARM_SHARED "--build-shared")
endif()
# Enable "--with-production" for Charm++ build for Release builds
if(CMAKE_BUILD_TYPE MATCHES "REL")      # Release | RelWithDebInfo | MinSizeRel
  set(CHARM_RELEASE_BUILD "--with-production")
endif()
if (HOST_OS MATCHES "alpine")
  # To avoid requiring rpc.h on Alpine Linux using musl-libc, see also
  # https://lists.cs.illinois.edu/lists/arc/charm/2016-04/msg00002.html
  set(CHARM_ALPINE "--disable-charmdebug")
  # Use pthreads and avoid requiring glibc's thread context API, unimplemented
  # on purpose on Alpine Linux and musl-libc, see also the thread at
  # http://www.openwall.com/lists/musl/2016/02/04/3
  set(CHARM_PTHREADS "pthreads")
endif()
if (charm)
  ExternalProject_Add(
    charm
    PREFIX charm
    # Move unpacked tarball to install dir and build there
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E copy_directory
                      ${PROJECT_SOURCE_DIR}/src/charm
                      ${CMAKE_INSTALL_PREFIX}/charm
    # Build in install dir and leave stuff there - no install step
    BINARY_DIR ${CMAKE_INSTALL_PREFIX}/charm
    # Build Charm++
    BUILD_COMMAND ${CMAKE_INSTALL_PREFIX}/charm/build
                  charm++
                  ${CHARM_ARCHITECTURE}
                  ${CHARM_EXTRA_ARGS}
                  ${CHARM_COMPILER}
                  ${CHARM_PTHREADS}
                  ${CHARM_SHARED}
                  ${CHARM_RELEASE_BUILD}
                  ${CHARM_ALPINE}
                  -j${PROCESSOR_COUNT}
                  ${CXXFLAGS}
    DOWNLOAD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_CONFIGURE 1
    LOG_BUILD 1
  )
endif()

#### Boost ####################################################################
# http://www.boost.org
# Header only, only if not found
if (boost)
  ExternalProject_Add(
    boost
    PREFIX boost
    # Header-only, copy include dir over
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E copy_directory
                      ${PROJECT_SOURCE_DIR}/src/boost.gil/include
                      ${CMAKE_INSTALL_PREFIX}/include/
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DOWNLOAD_COMMAND ""
  )
endif()
