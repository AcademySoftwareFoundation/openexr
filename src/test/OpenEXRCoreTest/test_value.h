/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_TEST_VALUE_H
#define OPENEXR_CORE_TEST_VALUE_H

#include <cstdlib>
#include <iostream>

static void
core_test_fail (
    const char* expr, const char* file, unsigned line, const char* func)
{
    std::cerr << "Core Test failed: " << expr << "\n           file:" << file
              << "\n           line:" << line << "\n       function:" << func
              << std::endl;
    abort ();
}

#define EXRCORE_TEST(expr)                                                     \
    (static_cast<bool> (expr))                                                 \
        ? void (0)                                                             \
        : core_test_fail (#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__)

#define EXRCORE_TEST_RVAL(expr)                                                \
    {                                                                          \
        exr_result_t _test_rv = expr;                                          \
        if (_test_rv != EXR_ERR_SUCCESS)                                       \
        {                                                                      \
            std::cerr << "Return Error: (" << (int) _test_rv << ") "           \
                      << exr_get_default_error_message (_test_rv)              \
                      << std::endl;                                            \
            core_test_fail (#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__);   \
        }                                                                      \
    }

#define EXRCORE_TEST_RVAL_FAIL(code, expr)                                     \
    {                                                                          \
        exr_result_t _test_rv = expr;                                          \
        if (_test_rv != code)                                                  \
        {                                                                      \
            std::cerr << "Return Error: (" << (int) _test_rv << ") "           \
                      << exr_get_default_error_message (_test_rv)              \
                      << "\n    expected: (" << (int) (code) << ") "            \
                      << exr_get_default_error_message (code) << std::endl;    \
            core_test_fail (#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__);   \
        }                                                                      \
    }
#endif // OPENEXR_CORE_TEST_VALUE_H
