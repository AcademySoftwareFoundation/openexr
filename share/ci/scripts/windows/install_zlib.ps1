$homeDir = (pwd)

$zlibVersion = $Args[0]
$zlibWorkingDir = $Args[1]

$zlibMajorMinor = [io.path]::GetFileNameWithoutExtension("$zlibVersion")
$zlibVersionConcise = $zlibVersion -replace '[.]',''
$zlibArchive = "https://www.zlib.net/zlib${zlibVersionConcise}.zip"

$zlibRoot = "${zlibWorkingDir}\_zlib"
$zlibBuildPath = "${zlibWorkingDir}\zlib-${zlibVersion}"
$zlibDllPath = "${zlibRoot}\bin"
$msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\msbuild.exe"

Write-Host "Retrieving ${zlibArchive}"
Invoke-WebRequest "${zlibArchive}" -OutFile "${zlibBuildPath}.zip"
Write-Host "Expanding archive ${zlibBuildPath}.zip"
Expand-Archive "${zlibBuildPath}.zip" -DestinationPath "${zlibWorkingDir}"

if (-NOT (Test-Path $zlibRoot))
{
	New-Item -ItemType Directory $zlibRoot
}

cd $zlibBuildPath
mkdir _build
cd _build
cmake .. -G"Visual Studio 16 2019" -DCMAKE_INSTALL_PREFIX="${zlibRoot}"

Write-Host "Building ${zlibBuildPath}\_build\INSTALL.vcxproj" -foregroundcolor green 
& "${msbuild}" "${zlibBuildPath}\_build\INSTALL.vcxproj" /P:Configuration=Release

cd $homeDir

echo "::set-env name=ZLIB_ROOT::$zlibRoot"
echo "::add-path::$zlibDllPath"
