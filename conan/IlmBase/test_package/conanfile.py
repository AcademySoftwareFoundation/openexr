from conans import CMake, ConanFile


class DefaultNameConan(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    generators = "cmake_paths"

    def build(self):
        cmake = CMake(self)
        cmake.definitions["CMAKE_TOOLCHAIN_FILE"] = "conan_paths.cmake"
        cmake.configure()
        cmake.build()

    def test(self):
        self.run("testPackage", run_environment=True)
