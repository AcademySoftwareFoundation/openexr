from conans import ConanFile, CMake, RunEnvironment, tools
import os


class PyIlmBaseConan(ConanFile):
    name = "PyIlmBase"
    description = "PyIlmBase is a high dynamic-range (HDR) image file format developed by Industrial Light & " \
                  "Magic for use in computer imaging applications."
    version = "2.3.0"
    license = "BSD"
    url = "https://github.com/openexr/openexr"
    settings = "os", "compiler", "build_type", "arch", "cppstd"
    generators = "cmake_paths"
    exports_sources = "PyIlmBase/*", "CMakeLists.txt", "LICENSE"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    def requirements(self):
        self.requires('IlmBase/2.3.0@aswf/vfx2018')
        self.requires('boost/1.61.0@aswf/vfx2018')

    def configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["CMAKE_TOOLCHAIN_FILE"] = "conan_paths.cmake"
        cmake.definitions["OPENEXR_BUILD_ILMBASE"] = "OFF"
        cmake.definitions["OPENEXR_BUILD_OPENEXR"] = "OFF"
        cmake.definitions["OPENEXR_BUILD_PYTHON_LIBS"] = "ON"
        cmake.definitions["OPENEXR_BUILD_UTILS"] = "OFF"
        cmake.definitions["OPENEXR_BUILD_VIEWERS"] = "OFF"
        cmake.configure()
        return cmake

    def build(self):
        cmake = self.configure_cmake()
        cmake.build()
        with tools.environment_append(RunEnvironment(self).vars):
            cmake.test(output_on_failure=True)

    def package(self):
        cmake = self.configure_cmake()
        cmake.install()

    def package_info(self):
        self.env_info.PYTHONPATH.append(os.path.join(self.package_folder, 'lib/python2.7/site-packages'))
        self.cpp_info.includedirs = ['include', os.path.join('include', 'OpenEXR')]
        self.cpp_info.libs = ['PyImath', 'Pylex']
