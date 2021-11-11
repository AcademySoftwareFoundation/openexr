void
readRgba2 (const char fileName[])
{
    RgbaInputFile file (fileName);
    Box2i dw = file.dataWindow();

    int width = dw.max.x - dw.min.x + 1;
    int height = dw.max.y - dw.min.y + 1;
    Array2D<Rgba> pixels (10, width);

    while (dw.min.y <= dw.max.y)
    {
        file.setFrameBuffer (&pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);
    

        file.readPixels (dw.min.y, min (dw.min.y + 9, dw.max.y));

        // processPixels (pixels)

        dw.min.y += 10;
    }
}
