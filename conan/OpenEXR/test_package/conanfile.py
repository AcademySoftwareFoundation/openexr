from conans import ConanFile, CMake, tools
import os


class DefaultNameConan(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    generators = "cmake_paths"

    def build(self):
        cmake = CMake(self)
        cmake.definitions["CMAKE_TOOLCHAIN_FILE"] = "conan_paths.cmake"
        cmake.configure()
        cmake.build()

    def test(self):
        imgfile = os.path.join(self.source_folder, 'bonita.exr')
        self.run("testPackage " + imgfile, run_environment=True)
