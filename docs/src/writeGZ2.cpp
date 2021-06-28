void
writeGZ2 (const char fileName[],
          const half *gPixels,
          const float *zPixels,
          int width,
          int height,
          const Box2i &dataWindow)
{
    Header header (width, height);
    header.dataWindow() = dataWindow;
    header.channels().insert ("G", Channel (HALF));
    header.channels().insert ("Z", Channel (FLOAT));
        
    OutputFile file (fileName, header);
        
    FrameBuffer frameBuffer;
        
    frameBuffer.insert ("G", // name
                        Slice (HALF, // type
                               (char *) gPixels, // base
                               sizeof (*gPixels) * 1, // xStride
                               sizeof (*gPixels) * width)); // yStride
        
    frameBuffer.insert ("Z", // name
                        Slice (FLOAT, // type
                               (char *) zPixels, // base
                               sizeof (*zPixels) * 1, // xStride
                               sizeof (*zPixels) * width)); // yStride
        
    file.setFrameBuffer (frameBuffer);
    file.writePixels (dataWindow.max.y - dataWindow.min.y + 1);
}

