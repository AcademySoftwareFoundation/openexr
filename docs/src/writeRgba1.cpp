void
writeRgba1 (const char fileName[], const Rgba *pixels, int width, int height)
{
    RgbaOutputFile file (fileName, width, height, WRITE_RGBA); // 1
    file.setFrameBuffer (pixels, 1, width);                    // 2
    file.writePixels (height);                                 // 3
}
