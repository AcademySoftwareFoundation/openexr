# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# Script run by the coverage-clean target: delete the accumulated gcov
# run-time results (*.gcda) under ${DIR}.
file(GLOB_RECURSE _gcda "${DIR}/*.gcda")
if(_gcda)
  file(REMOVE ${_gcda})
endif()
