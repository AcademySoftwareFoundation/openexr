$homeDir = (pwd)

$pythonVersion = $Args[0]
$pythonWorkingDir = $Args[1]

$pythonMajor = ($pythonVersion -split '\.')[0]
$pythonMinor = ($pythonVersion -split '\.')[1]
$pythonRoot = "${pythonWorkingDir}\_python${pythonMajor}${pythonMinor}"

Write-Host "Installing python version ${pythonVersion} ${pythonRoot}"

if (-NOT (Test-Path $pythonRoot))
{
	New-Item -ItemType Directory $pythonRoot
}

cd $pythonRoot

if ($pythonMajor -eq "3")
{
    Invoke-WebRequest "https://www.python.org/ftp/python/${pythonVersion}/python-${pythonVersion}.exe" -OutFile "python-${pythonVersion}-amd64.exe"
    Invoke-Expression "./python-${pythonVersion}-amd64.exe /quiet /l* _python.log TargetDir=${pythonRoot} PrependPath=1"
}
else
{
    Invoke-WebRequest "https://www.python.org/ftp/python/${pythonVersion}/python-${pythonVersion}.amd64.msi" -OutFile "python-${pythonVersion}-amd64.msi"
    msiexec /i "python-${pythonVersion}-amd64.msi" /quiet /l* _python.log TARGETDIR="${pythonRoot}" PrependPath=1
}

cd $homeDir

echo "::set-env name=PYTHON_ROOT::$pythonRoot"
echo "::add-path::$pythonRoot"
echo "::add-path::$pythonRoot/Scripts"