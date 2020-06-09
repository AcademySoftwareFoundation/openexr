#
# TODO: fix this script to work with sourceforge archive!
#
$homeDir = (pwd)

$boostVersion = $Args[0]
$boostWorkingDir = $Args[1]
$pythonVersion = $Args[2]

$boostRoot = "${boostWorkingDir}\_boost"

$boostMajorMinor = [io.path]::GetFileNameWithoutExtension("$boostVersion")
$boostVersionConcise = $boostVersion -replace '[.]',''
$boostArray = $boostVersion -split "\."
$boostMajor = $boostArray[0]
$boostMinor = $boostArray[1]
$boostPatch = $boostArray[2];
$boostVersionU = "boost_${boostMajor}_${boostMinor}_${boostPatch}"
$boostArchive = "https://sourceforge.net/projects/boost/files/boost-binaries/1.70.0/boost_1_70_0-msvc-14.1-64.exe"
$boostBuildPath = "${boostRoot}\boost-${boostVersion}\${boostVersionU}"

$pythonMajor = ($pythonVersion -split '\.')[0]
$pythonMinor = ($pythonVersion -split '\.')[1]
$pythonMajorMinor = "${pythonMajor}.${pythonMinor}"

Write-Host "boostRoot ${boostRoot}"
Write-Host "boostBuildPath ${boostBuildPath}"
Write-Host "pythonMajorMinor ${pythonMajorMinor}"

if (-NOT (Test-Path $boostRoot))
{
	New-Item -ItemType Directory $boostRoot
}

cd $boostRoot

# Retrieve/expand archive 
Write-Host "Retrieving ${boostArchive}"
Invoke-WebRequest $boostArchive -OutFile "boost-${boostVersion}.zip"
Write-Host "Expanding archive boost-${boostVersion}.zip"
Expand-Archive "boost-${boostVersion}.zip" 

cd "${boostBuildPath}\tools\build"

# Configure and install boost 
echo "Configuring boost..."
& .\bootstrap.bat --with-python-version=$pythonMajorMinor
& .\b2 install -j4 variant=release toolset=msvc `
      --prefix=$boostBuildPath `
      --address-model=64

$env:Path = "${boostBuildPath}\bin;${boostBuildPath};$env:Path"

cd $boostBuildPath

# Build boost-python2 libs 
echo "Building boost python..."
& b2 --with-python `
	 --address-model=64 `
	 --prefix=$boostBuildPath `
	 toolset=msvc

cd $homeDir

$env:BOOST_ROOT = $boostBuildPath
echo "BOOST_ROOT = ${env:BOOST_ROOT}"

echo "::set-env name=BOOST_ROOT::$boostBuildPath"
echo "::add-path::$boostBuildPath"

