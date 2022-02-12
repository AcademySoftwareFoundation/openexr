void
readRgbaFILE (
    FILE*          cfile,
    const char     fileName[],
    Array2D<Rgba>& pixels,
    int&           width,
    int&           height)
{
    C_IStream istr (cfile, fileName);

    RgbaInputFile file (istr);

    Box2i dw = file.dataWindow ();

    width  = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;
    pixels.resizeErase (height, width);

    file.setFrameBuffer (&pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);

    file.readPixels (dw.min.y, dw.max.y);
}
