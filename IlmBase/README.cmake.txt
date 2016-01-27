WINDOWS
-------

Build IlmBase and OpenEXR on Windows using cmake
------------------

What follows are instructions for generating Visual Studio solution 
files and building those two packages

1. Launch a command window, navigate to the IlmBase folder with 
CMakeLists.txt,and type command:
	mkdir build
	cd build
	cmake
      -DCMAKE_INSTALL_PREFIX=<where you want to install the ilmbase builds>
      -G "Visual Studio 10 Win64" 
      ..

2. Navigate to IlmBase folder in Windows Explorer, open ILMBase.sln
and build the solution. When it build successfully, right click 
INSTALL project and build. It will install the output to the path
you set up at the previous step.  

3. Go to http://www.zlib.net and download zlib 
	  
4. Launch a command window, navigate to the OpenEXR folder with 
CMakeLists.txt, and type command:	  
	mkdir build
	cd build
	cmake 
      -DZLIB_ROOT=<zlib location>
      -DCMAKE_INSTALL_PREFIX=<where you want to instal the openexr builds>
      -G "Visual Studio 10 Win64"
      ..
If installing to a different location to IlmBase then you will also need to supply the flag:
  -DCMAKE_PREFIX_PATH=<where you installed the ilmbase builds>

5. Make sure that the path to the IlmBase DLLs are in your PATH,
as they are required by the executables that generate headers during building.
Navigate to OpenEXR folder in Windows Explorer, open OpenEXR.sln
and build the solution. When it build successfully, right click 
INSTALL project and build. It will install the output to the path
you set up at the previous step. 



LINUX
-----
mkdir  /tmp/openexrbuild
cd  /tmp/openexrbuild

-------------
-- IlmBase --
-------------
initial bootstrapping:
    cmake -DCMAKE_INSTALL_PREFIX=<install location> <source location of IlmBase>

build the actual code base:
    make -j 4

for testing do:
    make test

then to install to your chosen location:
    make install



