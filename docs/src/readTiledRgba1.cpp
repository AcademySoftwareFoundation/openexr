void
readTiledRgba1 (
    const char fileName[], Array2D<Rgba>& pixels, int& width, int& height)
{
    TiledRgbaInputFile in (fileName);

    Box2i dw = in.dataWindow ();
    width    = dw.max.x - dw.min.x + 1;
    height   = dw.max.y - dw.min.y + 1;

    int dx = dw.min.x;
    int dy = dw.min.y;

    pixels.resizeErase (height, width);

    in.setFrameBuffer (&pixels[-dy][-dx], 1, width);
    in.readTiles (0, in.numXTiles () - 1, 0, in.numYTiles () - 1);
}
