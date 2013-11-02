xcodebuild -project IlmImf/IlmImf.xcodeproj install DSTROOT=/tmp
sudo mv /tmp/lib/libIlmImf.a /usr/local/lib/libIlmImf-2.0.1.a
sudo ln -sf /usr/local/lib/libIlmImf-2.0.1.a /usr/local/lib/libIlmImf.a
sudo mkdir -p /usr/local/include/OpenEXR
sudo cp ../IlmImf/*.h /usr/local/include/OpenEXR
