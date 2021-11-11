void
readGZ2 (const char fileName[],
         Array2D<GZ> &pixels,
         int &width, int &height)
{
    InputFile file (fileName);

    Box2i dw = file.header().dataWindow();
    width = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;

    int dx = dw.min.x;
    int dy = dw.min.y;

    pixels.resizeErase (height, width);

    FrameBuffer frameBuffer;

    frameBuffer.insert ("G",
                        Slice (HALF,
                               (char *) &pixels[-dy][-dx].g,
                               sizeof (pixels[0][0]) * 1,
                               sizeof (pixels[0][0]) * width));

    frameBuffer.insert ("Z",
                        Slice (FLOAT,
                               (char *) &pixels[-dy][-dx].z,
                               sizeof (pixels[0][0]) * 1,
                               sizeof (pixels[0][0]) * width));

    file.setFrameBuffer (frameBuffer);
    file.readPixels (dw.min.y, dw.max.y);
}
