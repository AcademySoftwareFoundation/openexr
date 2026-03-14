#! /bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

buildtype=Debug
cflags="-ggdb3 -Og"

lib_names=(Iex IlmThread OpenEXRCore OpenEXR OpenEXRUtil)

libver_old=v3.1.7
sonames_old=(libIex-3_1_d.so.30.7.1 libIlmThread-3_1_d.so.30.7.1 libOpenEXRCore-3_1_d.so.30.7.1 libOpenEXRUtil-3_1_d.so.30.7.1 libOpenEXR-3_1_d.so.30.7.1)
libver_new=RB-3.1
sonames_new=(libIex-3_1_d.so.30.8.1 libIlmThread-3_1_d.so.30.8.1 libOpenEXRCore-3_1_d.so.30.8.1 libOpenEXRUtil-3_1_d.so.30.8.1 libOpenEXR-3_1_d.so.30.8.1)

srcdir=`pwd`

if [[ "$1" == distclean ]]; then
    rm -rf abi_check
fi

mkdir -p abi_check
export PATH=${srcdir}/abi_check/bin:${PATH}

cd abi_check

if [[ ! -e bin/vtable-dumper ]]; then
    mkdir -p src
    cd src
    git clone https://github.com/lvc/vtable-dumper
    cd vtable-dumper
    make
    make install prefix=${srcdir}/abi_check
    cd ${srcdir}/abi_check
fi

if [[ ! -e bin/abi-dumper ]]; then
    mkdir -p src
    cd src
    git clone https://github.com/lvc/abi-dumper
    cd abi-dumper
    perl Makefile.pl -install --prefix=${srcdir}/abi_check
    cd ${srcdir}/abi_check
fi

if [[ ! -e bin/abi-compliance-checker ]]; then
    mkdir -p src
    cd src
    git clone https://github.com/lvc/abi-compliance-checker
    cd abi-compliance-checker
    perl Makefile.pl -install --prefix=${srcdir}/abi_check
    cd ${srcdir}/abi_check
fi

builddir_v1=build.${libver_old}
builddir_v2=build.${libver_new}
instdir_v1=inst.${libver_old}
instdir_v2=inst.${libver_new}

if [[ "$1" == clean ]]; then
    rm -rf ${builddir_v1}
    rm -rf ${builddir_v2}
    rm -rf ${instdir_v1}
    rm -rf ${instdir_v2}
    rm -rf abidumps
fi

curhash=`git branch --show-current`
if [[ "${curhash}" == "" ]]; then
    curhash=`git rev-parse HEAD`
fi

if [[ ! -e ${instdir_v1} ]]; then
    rm -rf ${builddir_v1}
    git checkout ${libver_old} || exit 1
    cmake -B ${builddir_v1} -S ${srcdir} -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF -DCMAKE_C_FLAGS="${cflags}" -DCMAKE_CXX_FLAGS="${cflags}" -DCMAKE_INSTALL_PREFIX=${srcdir}/abi_check/${instdir_v1} || exit 1
    ninja -C ${builddir_v1} install || exit 1
    git checkout ${curhash}
fi

if [[ ! -e ${instdir_v2} ]]; then
    rm -rf ${builddir_v2}
    git checkout ${libver_new} || exit 1
    cmake -B ${builddir_v2} -S ${srcdir} -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF -DCMAKE_C_FLAGS="${cflags}" -DCMAKE_CXX_FLAGS="${cflags}" -DCMAKE_INSTALL_PREFIX=${srcdir}/abi_check/${instdir_v2} || exit 1
    ninja -C ${builddir_v2} install || exit 1
    git checkout ${curhash}
fi

len=${#lib_names[@]}

mkdir -p abidumps/${libver_old}
mkdir -p abidumps/${libver_new}
for (( i=0; i<$len; i++ ));
do
    name=${lib_names[$i]}
    echo "Processing ${name}"
    
    abi-dumper ${instdir_v1}/lib/${sonames_old[$i]} -o abidumps/${libver_old}/ABI-${name}.dump -vnum ${libver_old} -skip-cxx -all-symbols
    abi-dumper ${instdir_v2}/lib/${sonames_new[$i]} -o abidumps/${libver_new}/ABI-${name}.dump -vnum ${libver_new} -skip-cxx -all-symbols

    abi-compliance-checker -l ${name} -d1 abidumps/${libver_old}/ABI-${name}.dump -v1 ${libver_old} -d2 abidumps/${libver_new}/ABI-${name}.dump -v2 ${libver_new}
done

echo "Reports should be at:"
for (( i=0; i<$len; i++ ));
do
    name=${lib_names[$i]}
    echo "${name}: file://${srcdir}/abi_check/compat_reports/${name}/${libver_old}_to_${libver_new}/compat_report.html"
done

