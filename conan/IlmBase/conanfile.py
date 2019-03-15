
import os
from conans import ConanFile, CMake


class IlmBaseConan(ConanFile):
    name = "IlmBase"
    description = "IlmBase is a component of OpenEXR. OpenEXR is a high dynamic-range (HDR) image file format developed by Industrial Light & Magic for use in computer imaging applications."
    version = "2.3.0"
    license = "BSD"
    url = "https://github.com/openexr/openexr"
    settings = "os", "compiler", "build_type", "arch", "cppstd"
    generators = "cmake"
    exports_sources = "IlmBase/*", "cmake/FindIlmBase.cmake", "CMakeLists.txt", "LICENSE"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    def configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["OPENEXR_BUILD_ILMBASE"] = "ON"
        cmake.definitions["OPENEXR_BUILD_OPENEXR"] = "OFF"
        cmake.definitions["OPENEXR_BUILD_PYTHON_LIBS"] = "OFF"
        cmake.definitions["OPENEXR_BUILD_UTILS"] = "OFF"
        cmake.definitions["OPENEXR_BUILD_VIEWERS"] = "OFF"
        cmake.configure()
        return cmake

    def build(self):
        cmake = self.configure_cmake()
        cmake.build()
        cmake.test()

    def package(self):
        self.copy("FindIlmBase.cmake", src="cmake", keep_path=False)
        self.copy("LICENSE")
        cmake = self.configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.includedirs = ['include', os.path.join('include', 'OpenEXR')]
        self.cpp_info.libs = ['Half', 'Iex', 'IexMath', 'IlmThread', 'Imath']
