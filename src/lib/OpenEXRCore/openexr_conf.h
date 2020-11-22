/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CONF_H
#define OPENEXR_CONF_H
#pragma once

#include <OpenEXRConfig.h>

/*  TODO: Move this stuff to OpenEXRConfig.h.in? */

/** 
 * @defgroup configurable name wrappers for C
 * @brief These are a group of macros which enable people to define a custom name
 *
 * This might be used to put this version of the C layer into a custom
 * "namespace" so as not to conflict with a system library used by an
 * existing DCC or similar.
 *
 * @{
 */

/** Used to declare / use types */
#define EXR_TYPE(n) exr_ ## n
/** Used to declare / use enums */
#define EXR_DEF(n) EXR_ ## n
/** Used to declare / call functions */
#define EXR_FUN(n) exr_ ## n

/** @} */

#if defined(OPENEXR_DLL)
# if defined(OPENEXR_EXPORTS)
#  define EXR_EXPORT __declspec(dllexport)
# else
#  define EXR_EXPORT __declspec(dllimport)
# endif
#else
# define EXR_EXPORT __attribute__ ((visibility ("default")))
#endif

#define EXR_PRINTF_FUNC_ATTRIBUTE __attribute__ ((format (printf, 3, 4)))

#endif /* OPENEXR_CONF_H */
