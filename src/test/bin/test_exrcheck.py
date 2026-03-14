#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os
from do_run import do_run

print(f"testing exrcheck: {' '.join(sys.argv)}")

exrcheck = sys.argv[1]
image_files = sys.argv[2:]

if not os.path.isfile(exrcheck) or not os.access(exrcheck, os.X_OK):
    print(f"error: no such file: {exrcheck}")
    sys.exit(1)

for exr_file in image_files:

    do_run([exrcheck, exr_file])
    do_run([exrcheck, "-m", exr_file])
    do_run([exrcheck, "-t", exr_file])
    do_run([exrcheck, "-s", exr_file])
    do_run([exrcheck, "-c", exr_file])

print("success.")
sys.exit(0)

