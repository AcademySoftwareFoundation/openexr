void
writeRgbaMT (const char fileName[], const Rgba* pixels, int width, int height)
{
    setGlobalThreadCount (4);

    RgbaOutputFile file (fileName, width, height, WRITE_RGBA);
    file.setFrameBuffer (pixels, 1, width);
    file.writePixels (height);
}
