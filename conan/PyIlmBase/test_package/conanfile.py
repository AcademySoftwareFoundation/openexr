from conans import ConanFile, CMake, tools
import os


class PyIlmBaseTester(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    requires = "PyIlmBase/2.2.0@aswf/vfx2018"

    def test(self):
        testfile = os.path.join(self.source_folder, 'test_imath.py')
        self.run('/usr/bin/python2.7 %s'%testfile, run_environment=True)
