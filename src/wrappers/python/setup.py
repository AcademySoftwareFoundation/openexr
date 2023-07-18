# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

from setuptools import setup, Extension
import os
import platform
import re


DESC = """Python bindings for ILM's OpenEXR image file format.

This is a script to autobuild the wheels using github actions. Please, do not
use it manually

If you detect any problem, please feel free to report the issue on the GitHub
page:

https://github.com/sanguinariojoe/pip-openexr/issues
"""


version = []
with open('src/lib/OpenEXRCore/openexr_version.h', 'r') as f:
    txt = f.read()
    for name in ('MAJOR', 'MINOR', 'PATCH'):
        version.append(re.search(
            f'VERSION_{name} ([0-9]*)', txt).group(0).split(' ')[-1])
version_major, version_minor, version_patch = version
version = f"{version_major}.{version_minor}.{version_patch}"

libs=[]
libs_static=['z',
             f'OpenEXR-{version_major}_{version_minor}',
             f'IlmThread-{version_major}_{version_minor}',
             f'Iex-{version_major}_{version_minor}',
             f'Imath-3_1',
             f'OpenEXRCore-{version_major}_{version_minor}',
             f'deflate']
definitions = [('PYOPENEXR_VERSION_MAJOR', f'{version_major}'),
               ('PYOPENEXR_VERSION_MINOR', f'{version_minor}'),
               ('PYOPENEXR_VERSION_PATCH', f'{version_patch}'),]
if platform.system() == "Windows":
    libs_static[0] = 'zlibstatic'
    libs_static[-1] = 'deflatestatic'
    definitions = [('PYOPENEXR_VERSION', f'\\"{version}\\"')]
extra_compile_args = []
if platform.system() == 'Darwin':
    extra_compile_args += ['-std=c++11',
                           '-Wc++11-extensions',
                           '-Wc++11-long-long']

libs_dir = "./openexr.install/lib/"
if not os.path.isdir(libs_dir):
    libs_dir = "./openexr.install/lib64/"
if platform.system() == "Windows":
    extra_link_args = [libs_dir + lib + ".lib"
                       for lib in libs_static]
    extra_link_args = extra_link_args + [
        "ws2_32.lib", "dbghelp.lib", "psapi.lib", "kernel32.lib", "user32.lib",
        "gdi32.lib", "winspool.lib", "shell32.lib", "ole32.lib",
        "oleaut32.lib", "uuid.lib", "comdlg32.lib", "advapi32.lib"]
else:
    extra_link_args = [libs_dir + "lib" + lib + ".a"
                       for lib in libs_static]


setup(name='OpenEXR',
    author = 'James Bowman',
    author_email = 'jamesb@excamera.com',
    url = 'https://github.com/sanguinariojoe/pip-openexr',
    description = "Python bindings for ILM's OpenEXR image file format",
    long_description = DESC,
    version=version,
    ext_modules=[ 
        Extension('OpenEXR',
                  ['OpenEXR.cpp'],
                  language='c++',
                  define_macros=definitions,
                  include_dirs=['./openexr.install/include/OpenEXR',
                                './openexr.install/include/Imath',],
                  libraries=libs,
                  extra_compile_args=extra_compile_args,
                  extra_link_args=extra_link_args,
                  )
    ],
    py_modules=['Imath'],
)
