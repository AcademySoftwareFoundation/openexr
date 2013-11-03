xcodebuild -project PyIex/PyIex.xcodeproj install DSTROOT=/tmp
sudo mv /tmp/lib/libPyIex.dylib /Library/Python/2.7/site-packages/iex.so
