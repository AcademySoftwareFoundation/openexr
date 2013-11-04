xcodebuild -project PyIex/PyIex.xcodeproj install DSTROOT=/tmp
sudo cp /tmp/lib/libPyIex.dylib /usr/local/lib/libPyIex.dylib
sudo ln -sf /usr/local/lib/libPyIex.dylib /Library/Python/2.7/site-packages/iex.so
sudo cp ../PyIex/*.h /usr/local/include/OpenEXR

xcodebuild -project PyImath/PyImath.xcodeproj install DSTROOT=/tmp
sudo cp /tmp/lib/libPyImath.dylib /usr/local/lib/libPyImath.dylib
sudo ln -sf /usr/local/lib/libPyImath.dylib /Library/Python/2.7/site-packages/imath.so
