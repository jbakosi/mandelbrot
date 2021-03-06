################################################################################
#
# \file      src/CMakeListst.txt
# \copyright 2016-2017, Los Alamos National Security, LLC.
# \brief     Main cmake file for Mandelbrot
#
################################################################################

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(Mandelbrot CXX)

set(CMAKE_VERBOSE_MAKEFILE 1)

# Mandelbrot cmake modules directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

# Set CMAKE_BUILD_TYPE
include(BuildType)

# Set CMAKE_BUILD_TYPE
include(BuildType)

# Set BUILD_SHARED_LIBS
include(BuildShared)

# Conditionally enforce static linking for executables added after this point.
# This must precede all add_executable() calls adding executables to link
# statically.
if (NOT BUILD_SHARED_LIBS)
  set(CMAKE_EXE_LINKER_FLAGS "-static")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

# Convert CMAKE_BUILD_TYPE
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)

# Query target architecture
include(TargetArch)
target_architecture(ARCH)
message(STATUS "Target architecture: ${ARCH}")

# If not specified on the command-line, set default third-party libraries
# directory based on cmake-supplied compiler ID
if (NOT TPL_DIR)
  string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" COMP)
  set(REL_TPL_DIR ${PROJECT_SOURCE_DIR}/../tpl/install/${COMP}-${ARCH})
  get_filename_component(ABS_TPL_DIR ${REL_TPL_DIR} ABSOLUTE)
  set(TPL_DIR ${ABS_TPL_DIR} CACHE STRING "Third-party libraries directory.")
  if (EXISTS ${TPL_DIR}/include)
    set(TPL_INCLUDE_DIR ${TPL_DIR}/include)
    message(STATUS "Third-party library dir: " ${TPL_DIR})
  endif()
endif()

# Find MPI and get its underlying compilers
include(MPICompilers)
get_mpi_compilers()

# Mac OS X specifics
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

  # On Mac OS X, set RPATH
  set(CMAKE_MACOSX_RPATH ON)

  # On Mac OS X, do not to resolve symbols when creating a shared library and
  # only resolve the symbols when linking executables. This behavior is
  # consistent with gcc, intel, and clang on linux.
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")

  # On Mac OS X, due to changes in SnowLeopard (darwin10) linker changes, we add
  # "-no_compact_unwind" to the link line. This tells the linker to not produce
  # compact unwind information from the dwarf unwind info in .o files. Then at
  # runtime the darwin unwinder will fallback and use the slow dwarf unwind
  # info.
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_compact_unwind" )
endif()

# Set the requirement for the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Required language standard: C++${CMAKE_CXX_STANDARD}")

# Get git revision
include(GetGitRevisionDescription)
git_describe(GIT_SHA1 --always)
message(STATUS "git sha1: ${GIT_SHA1}")

# Set names of executables
set(MANDEL_EXECUTABLE mandel)

# Macro to add compiler flags
macro (add_compiler_flag flag)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
endmacro (add_compiler_flag)

# Compiler-specific settings
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")  # clang-specific settings

  # Include function that allows to set standard C++ library
  include(libstdcxx)

  # Offer switch between libc++ and libstdc++, using libc++ as default
  if (LIBCXX_FOUND)
    set_libstdcpp_vs_libcpp(CMAKE_CXX_FLAGS "libc++")
  endif()

  # Compiler flags for clang
  add_compiler_flag("-fcolor-diagnostics")

  add_compiler_flag("-Weverything")
  add_compiler_flag("-Wno-c++98-compat")
  add_compiler_flag("-Wno-c++98-compat-pedantic")
  add_compiler_flag("-Wno-padded")
  add_compiler_flag("-Wno-exit-time-destructors")
  add_compiler_flag("-Wno-global-constructors")
  add_compiler_flag("-Wno-weak-vtables")
  add_compiler_flag("-Wno-unknown-pragmas")

  # Linker flags for clang
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)      # clear link flags
  set(CMAKE_EXE_LINKER_FLAGS "-Wno-missing-prototypes -Wno-unused-parameter")

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")  # gcc-specific settings

  # Gnu will use libstdc++ by default. We do not offer libc++.

  # Compiler flags for gcc
  add_compiler_flag("-fdiagnostics-color")
  add_compiler_flag("-Wall")
  add_compiler_flag("-Wextra")
  add_compiler_flag("-pedantic")
  add_compiler_flag("-pedantic-errors")
  add_compiler_flag("-Wcast-align")
  add_compiler_flag("-Wcast-qual")
  add_compiler_flag("-Wdisabled-optimization")
  add_compiler_flag("-Wfloat-equal")
  add_compiler_flag("-Wformat=2")
  add_compiler_flag("-Wformat-nonliteral")
  add_compiler_flag("-Wformat-security")
  add_compiler_flag("-Wformat-y2k")
  add_compiler_flag("-Wimport")
  add_compiler_flag("-Winit-self")
  add_compiler_flag("-Winvalid-pch")
  add_compiler_flag("-Wmissing-field-initializers")
  add_compiler_flag("-Wmissing-format-attribute")
  add_compiler_flag("-Wmissing-include-dirs")
  add_compiler_flag("-Wmissing-noreturn")
  add_compiler_flag("-Wpacked")
  add_compiler_flag("-Wpointer-arith")
  add_compiler_flag("-Wredundant-decls")
  add_compiler_flag("-Wshadow")
  add_compiler_flag("-Wstack-protector")
  add_compiler_flag("-Wstrict-aliasing=2")
  add_compiler_flag("-Wswitch-default")
  add_compiler_flag("-Wunreachable-code")
  add_compiler_flag("-Wunused")
  add_compiler_flag("-Wunused-parameter")
  add_compiler_flag("-Wvariadic-macros")
  add_compiler_flag("-Wwrite-strings")
  add_compiler_flag("-Wno-sign-compare")
  add_compiler_flag("-Wno-unused-function")
  add_compiler_flag("-Wno-stack-protector")
  add_compiler_flag("-Wno-unknown-pragmas")
  add_compiler_flag("-mavx2")
  add_compiler_flag("-ffast-math")

  # Linker flags for gcc
  set(CMAKE_EXE_LINKER_FLAGS "-Wno-unused-parameter")

  # Detect compiler version
  include(DetectCXXversion)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")  # intel-specific settings

  # Include function that allows to set standard C++ library
  include(libstdcxx)

  # Offer switch between libc++ and libstdc++, using libc++ as default
  if (LIBCXX_FOUND)
    set_libstdcpp_vs_libcpp(CMAKE_CXX_FLAGS "libc++")
  endif()

  # Use the C++11 standard (CMAKE_CXX_STANDARD does not set this for intel)
  add_compiler_flag("-std=c++11")
  

  # Compiler flags for intel
  add_compiler_flag("-w3")       # enable diagnostics: remarks, warnings, errors
  add_compiler_flag("-Wremarks") # display remarks and comments

  # Globally suppress the following Intel compiler remarks:
  #     7: unrecognized token
  #    68: integer conversion resulted in a change of sign
  #    82: storage class is not first
  #   111: statement is unreachable
  #   186: pointless comparison of unsigned integer with zero
  #   177: function was declared but never referenced
  #   193: zero used for undefined preprocessing identifier
  #   280: selector expression is constant
  #   304: access control not specified ("public" by default)
  #   383: value copied to temporary, reference to temporary used
  #   327: NULL reference is not allowed
  #   411: defines no constructor for initializer_list<>
  #   444: destructor for base class is not virtual
  #   593: parameter was set but never used
  #   869: parameter was never referenced
  #   981: operands are evaluated in unspecified order
  #  1418: external function definition with no prior declaration
  #  1419: external declaration in primary source file
  #  1572: floating-point equality and inequality comparisons are unreliable
  #  2259: non-pointer conversion from "int" to "char" may lose significant bits
  #  2407: the initialization of class will be done before that of member
  #  2415: variable of static storage duration was declared but never referenced
  #  2547: non-system include dir will be ignored when both a system and non-system include directory were specified
  #  2557: comparison between signed and unsigned operands
  #  3280: declaration hides member
  #  3346: dynamic exception specifications are deprecated
  #  3656: variable may be used before its value is set
  add_compiler_flag("-diag-disable 7,68,82,111,177,186,193,280,304,327,383,411,444,593,869,981,1418,1419,1572,2259,2407,2415,2547,2557,3280,3346,3656")

  # Compiler flags for optimization report
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qopt-report=0 -qopt-report-phase=loop,vec -qopt-report-file=stderr")

  # No limit on inlining. This can increase the build time.
  if(CMAKE_BUILD_TYPE MATCHES "REL") # Release | RelWithDebInfo | MinSizeRel
    #add_compiler_flag("-no-inline-max-size")
    #add_compiler_flag("-no-inline-max-total-size")
    #add_compiler_flag("-no-inline-max-per-routine")
    add_compiler_flag("-diag-disable 11074,11076")
  endif()

  # Linker flags for intel
  set(CMAKE_EXE_LINKER_FLAGS "-diag-disable 869")

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "PGI")  # pgi-specific settings

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Cray")  # cray-specific settings

  # Use the C++11 standard (CMAKE_CXX_STANDARD does not set this for cray)
  add_compiler_flag("-hstd=c++11")

  # Compiler flags for cray
  # enable errors, warnings, cautions, notes, comments
  add_compiler_flag("-hmsglevel_0")
  add_c_compiler_flag("-hmsglevel_0")

  # Globally suppress the following Cray compiler messages:
  # 1: source file does not end with a new-line character
  # 7: indicated token is not valid in this context
  # 21: type qualifiers are meaningless in this declaration
  # 47: macro has an incompatible redefinition
  # 68: integer conversion resulted in a change of sign
  # 111: indicated statement is not reachable
  # 186: unsigned integer is being compared to zero
  # 193: zero is used for the indicated undefined preprocessing identifier
  # 174: indicated expression has no effect
  # 177: variable was declared but never referenced
  # 181: argument is incompatible with the corresponding format string conversion
  # 261: access control is not specified ("private" by default)
  # 237: indicated selector expression is constant
  # 301: indicated "typedef" name has already been declared (with same type)
  # 324: duplicate friend declaration
  # 368: class template defines no constructor to initialize ...
  # 401: destructor for base class is not virtual
  # 550: parameter "parts" is set but never used
  # 737: the using-declaration is ignored--it refers to the current namespace
  # 997: function is the virtual function override intended?
  # 826: parameter was never referenced
  # 1977: declaration hides variable
  # 1982: function ...::operator new has no corresponding member operator delete (to be called if an exception is thrown during initialization of an allocated object)
  # 3138: variable "this" assumed referenced by ...
  # 3137: variable "this" assumed modified by ...
  # 7212: variable ... is used before it is defined
  # 11709: diretory was specified as both a system and non-system include directory -- the non-system entry will be ignored
  # 12307: dynamic exception specifications are deprecated
  # 12489: constexpr non-static member function will not be implicitly 'const' in C++14
  # 7405: variable may be used before it is defined
  # 1418: a pointer expression is cast to a pointer with a greater alignment requirement
  # 10153: the initialization of class "Chare" will be done before that of member
  # 11749: declaration hides member
  # 11548: comparison between signed and unsigned operands
  add_compiler_flag("-hnomessage=11709:261:12307:12489:193:826:737:997:3138:3137:7405:1418:177:181:401:11749:11548:1982:368:324:47:1:7:111:68:10153:186:550:21:237:174:7212")
  add_c_compiler_flag("-hnomessage=11709:301:3137:3138:7405:193:1977")

endif()

# Echo compiler flags
message(STATUS "C++ compiler flags: '${CMAKE_CXX_FLAGS}'")

# Include third-party libraries configuration
include(TPLs)

### Set RPATH so that shared libs paths stay in the binary after install ###
# Don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH false)

# When building, don't use the install RPATH
# (use later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH false)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH true)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
     "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
   set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${PROJECT_BINARY_DIR}       # For Charm++ modules
                    ${TPL_INCLUDE_DIR}
                    ${CHARM_INCLUDE_DIRS})

project(mandel CXX)

### Quinoa executables build configuration #####################################

# Basic configuration
add_definitions()

# Include function for adding Charm++ modules
include(charm)

# Link executables with the charmc wrapper
STRING(REGEX REPLACE "<CMAKE_CXX_COMPILER>"
       "${CHARM_COMPILER} -module DistributedLB -c++ <CMAKE_CXX_COMPILER>"
       CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE}")

include(ConfigExecutable)

### Mandelbrot executable ######################################################
add_executable(${MANDEL_EXECUTABLE}
               Mandel.C)

config_executable(${MANDEL_EXECUTABLE})

target_link_libraries(${MANDEL_EXECUTABLE}
                      ${JPEG_LIBRARY})

# Add custom dependencies for mandel's main Charm++ module
addCharmModule( "mandel" "${MANDEL_EXECUTABLE}" )
