void
readGZ1 (const char fileName[],
         Array2D<half> &rPixels,
         Array2D<half> &gPixels,
         Array2D<float> &zPixels,
         int &width, int &height)
    
{
    InputFile file (fileName);

    Box2i dw = file.header().dataWindow();
    width = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;

    rPixels.resizeErase (height, width);
    gPixels.resizeErase (height, width);
    zPixels.resizeErase (height, width);

    FrameBuffer frameBuffer;

    frameBuffer.insert ("R",                                    // name
                        Slice (HALF,                            // type
                               (char *) (&rPixels[0][0] -      // base
                                          dw.min.x -
                                          dw.min.y * width),
                               sizeof (rPixels[0][0]) * 1,     // xStride
                               sizeof (rPixels[0][0]) * width, // yStride
                               1, 1,                            // x/y sampling
                               0.0));                           // fillValue

    frameBuffer.insert ("G",                                    // name
                        Slice (HALF,                            // type
                               (char *) (&gPixels[0][0] -      // base
                                          dw.min.x -
                                          dw.min.y * width),
                               sizeof (gPixels[0][0]) * 1,     // xStride
                               sizeof (gPixels[0][0]) * width, // yStride
                               1, 1,                            // x/y sampling
                               0.0));                           // fillValue

    frameBuffer.insert ("Z",                                    // name
                        Slice (FLOAT,                           // type
                               (char *) (&zPixels[0][0] -      // base
                                          dw.min.x -
                                          dw.min.y * width),
                               sizeof (zPixels[0][0]) * 1,     // xStride
                               sizeof (zPixels[0][0]) * width, // yStride
                               1, 1,                            // x/y sampling
                               FLT_MAX));                       // fillValue

    file.setFrameBuffer (frameBuffer);
    file.readPixels (dw.min.y, dw.max.y);
}

