// *****************************************************************************
/*!
  \file      src/NoWarning/mandel.def.h
  \copyright 2012-2015, J. Bakosi, 2016-2017, Los Alamos National Security, LLC.
  \brief     Include mandel.def.h with turning off specific compiler
             warnings
*/
// *****************************************************************************
#ifndef nowarning_mandel_def_h
#define nowarning_mandel_def_h

#include "Macro.h"

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wold-style-cast"
  #pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

#include "mandel.def.h"

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

#endif // nowarning_mandel_def_h
