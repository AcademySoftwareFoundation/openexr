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

#define EXR_PRINTF_FUNC_ATTRIBUTE __attribute__ ((format (printf, 3, 4)))

#endif /* OPENEXR_CONF_H */
