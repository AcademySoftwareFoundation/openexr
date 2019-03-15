from conans import ConanFile, CMake, RunEnvironment, tools
import os


class OpenEXRConan(ConanFile):
    name = "OpenEXR"
    description = "OpenEXR is a high dynamic-range (HDR) image file format developed by Industrial Light & " \
                  "Magic for use in computer imaging applications."
    version = "2.3.0"
    license = "BSD"
    url = "https://github.com/openexr/openexr"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    settings = "os", "compiler", "build_type", "arch", "cppstd"
    generators = "cmake_paths"
    exports_sources = "OpenEXR/*", "cmake/FindOpenEXR.cmake", "CMakeLists.txt", "LICENSE"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    def requirements(self):
        self.requires('IlmBase/2.3.0@aswf/vfx2018')

    def configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["CMAKE_TOOLCHAIN_FILE"] = "conan_paths.cmake"
        cmake.definitions["OPENEXR_BUILD_ILMBASE"] = "OFF"
        cmake.definitions["OPENEXR_BUILD_OPENEXR"] = "ON"
        cmake.definitions["OPENEXR_BUILD_PYTHON_LIBS"] = "OFF"
        cmake.definitions["OPENEXR_BUILD_UTILS"] = "OFF"
        cmake.definitions["OPENEXR_BUILD_VIEWERS"] = "OFF"
        cmake.configure()
        return cmake

    def build(self):
        cmake = self.configure_cmake()
        env_build = RunEnvironment(self)
        with tools.environment_append(env_build.vars): # needed by dwaLookups during build and the tests
            cmake.build()
            cmake.test()

    def package(self):
        self.copy("FindOpenEXR.cmake", src="cmake", keep_path=False)
        cmake = self.configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.includedirs = ['include', os.path.join('include', 'OpenEXR')]
        self.cpp_info.libs = ['IlmImf', 'IlmImfUtil']
