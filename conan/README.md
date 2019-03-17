# Conan recipes to build and test IlmBase, OpenEXR and PyIlmBase

Running the circleci job locally:
* install circleci command line tool https://circleci.com/docs/2.0/local-cli/#quick-installation
* run `circleci local execute`

## Debugging notes
To run the jobs locally, use the following docker line which allows running gdb within the container and mounts the current folder as the "circleci" current project.

```
docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -it -v `pwd`:/home/circleci/project -v /tmp/PyIlmBase:/tmp/PyIlmBase -v /tmp/conandata:/home/circleci/.conan/data aloysbaillet/aswf-vfx2018-conan bash
```
Then:
```
cd /home/circleci/project

conan create conan/IlmBase aswf/vfx2018 -tbf /tmp/testbuildfolder
conan create conan/OpenEXR aswf/vfx2018 -tbf /tmp/testbuildfolder
export CONAN_CPU_COUNT=2
conan create conan/PyIlmBase aswf/vfx2018 -tbf /tmp/testbuildfolder
```

In order to debug some tests:
```
conan source conan/PyIlmBase --source-folder=/tmp/conanproject/src
conan install conan/PyIlmBase --install-folder=/tmp/conanproject/build
conan build conan/PyIlmBase --source-folder=/tmp/conanproject/src --build-folder=/tmp/conanproject/build
cd /tmp/conanproject/build
ctest
```
