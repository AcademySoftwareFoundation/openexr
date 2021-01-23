/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CONF_H
#define OPENEXR_CONF_H
#pragma once

#include <OpenEXRConfig.h>

/*  TODO: Move this stuff to OpenEXRConfig.h.in? */

#if defined(OPENEXR_DLL)
# if defined(OPENEXR_EXPORTS)
#  define EXR_EXPORT __declspec(dllexport)
# else
#  define EXR_EXPORT __declspec(dllimport)
# endif
#else
# define EXR_EXPORT __attribute__ ((visibility ("default")))
#endif

/*
 * MSVC does have printf format checks, but it is not in the form of a
 * function attribute, so just skip for non-GCC / clang builds
 */
#if defined(__GNUC__) || defined(__clang__)
#  define EXR_PRINTF_FUNC_ATTRIBUTE __attribute__ ((format (printf, 3, 4)))
#else
#  define EXR_PRINTF_FUNC_ATTRIBUTE
#endif

#endif /* OPENEXR_CONF_H */
