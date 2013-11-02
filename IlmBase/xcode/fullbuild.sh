xcodebuild -project IlmBase/IlmBase.xcodeproj install DSTROOT=/tmp
sudo mv /tmp/lib/libIlmBase.a /usr/local/lib/libIlmBase-2.0.1.a
sudo ln -sf /usr/local/lib/libIlmBase-2.0.1.a /usr/local/lib/libIlmBase.a
sudo mkdir -p /usr/local/include/OpenEXR
sudo cp IlmBase/*.h /usr/local/include/OpenEXR
sudo cp ../Iex/*.h /usr/local/include/OpenEXR
sudo cp ../Half/*.h /usr/local/include/OpenEXR
sudo cp ../Imath/*.h /usr/local/include/OpenEXR
sudo cp ../IlmThread/*.h /usr/local/include/OpenEXR
sudo cp ../IexMath/*.h /usr/local/include/OpenEXR
