# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

from subprocess import PIPE, run
import sys

def do_run(cmd, expect_error = False, data=None):
    cmd_string = " ".join(cmd)
    print(f"running {cmd_string}")
    if data:
        result = run (cmd, input=data, stdout=PIPE, stderr=PIPE)
    else:
        result = run (cmd, stdout=PIPE, stderr=PIPE, universal_newlines=True)
    if expect_error:
        if result.returncode != 0:
            return result
        print(f"error: {cmd_string} did not fail as expected")
        print(f"stdout:\n{result.stdout}")
        print(f"stderr:\n{result.stderr}")
        sys.exit(1)
            
    if result.returncode != 0:
        print(f"error: {cmd_string} failed: returncode={result.returncode}")
        print(f"stdout:\n{result.stdout}")
        print(f"stderr:\n{result.stderr}")
        sys.exit(1)
    return result
