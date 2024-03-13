// [begin writeRgba1]
void
writeRgba1 (const char fileName[], const Rgba* pixels, int width, int height)
{
    RgbaOutputFile file (fileName, width, height, WRITE_RGBA); // 1
    file.setFrameBuffer (pixels, 1, width);                    // 2
    file.writePixels (height);                                 // 3
}
// [end writeRgba1]

void
tryCatchExample (const char fileName[], const Rgba* pixels, int width, int height) {
    // [begin tryCatchExample]
    try
    {
        writeRgba1 (fileName, pixels, width, height);
    }
    catch (const std::exception &exc)
    {
        std::cerr << exc.what() << std::endl;
    }
    // [end tryCatchExample]
}