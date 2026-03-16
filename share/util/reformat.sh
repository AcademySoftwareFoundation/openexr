#! /bin/sh

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

cformat=`which clang-format`
if [[ "${cformat}" == "" ]]; then
	echo "ERROR: clang-format not found"
	exit 1
fi

scriptpath=`realpath $0`
scriptdir=`dirname $scriptpath`
basedir=`dirname $scriptdir`
echo "Processing files in: ${basedir}"
cd ${basedir}
find . \( -name '*.cpp' -o -name '*.h' \) -exec ${cformat} -style=file -verbose -i {} \;

