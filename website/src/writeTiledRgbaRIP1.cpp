void
writeTiledRgbaRIP1 (
    const char fileName[], int width, int height, int tileWidth, int tileHeight)
{
    TiledRgbaOutputFile out (
        fileName,
        width,
        height,
        tileWidth,
        tileHeight,
        RIPMAP_LEVELS,
        ROUND_DOWN,
        WRITE_RGBA);

    Array2D<Rgba> pixels (height, width);

    out.setFrameBuffer (&pixels[0][0], 1, width);

    for (int yLevel = 0; yLevel < out.numYLevels (); ++yLevel)
    {
        for (int xLevel = 0; xLevel < out.numXLevels (); ++xLevel)
        {
            generatePixels (pixels, width, height, xLevel, yLevel);

            out.writeTiles (
                0,
                out.numXTiles (xLevel) - 1,
                0,
                out.numYTiles (yLevel) - 1,
                xLevel,
                yLevel);
        }
    }
}
