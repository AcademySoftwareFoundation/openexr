void
writeGZ1 (const char fileName[],
          const half *gPixels,
          const float *zPixels,
          int width,
          int height)
{
    Header header (width, height);                               // 1
    header.channels().insert ("G", Channel (HALF));              // 2
    header.channels().insert ("Z", Channel (FLOAT));             // 3
    
    OutputFile file (fileName, header);                          // 4
    
    FrameBuffer frameBuffer;                                     // 5
    
    frameBuffer.insert ("G",                          // name    // 6
                        Slice (HALF,                  // type    // 7
                               (char *) gPixels,            // base    // 8
                               sizeof (*gPixels) * 1,       // xStride // 9
                               sizeof (*gPixels) * width)); // yStride // 10
    
    frameBuffer.insert ("Z",                          // name    // 11
                        Slice (FLOAT,                 // type    // 12
                               (char *) zPixels,            // base    // 13
                               sizeof (*zPixels) * 1,       // xStride // 14
                               sizeof (*zPixels) * width)); // yStride // 15
    
    file.setFrameBuffer (frameBuffer);                           // 16
    file.writePixels (height);                                   // 17
}

