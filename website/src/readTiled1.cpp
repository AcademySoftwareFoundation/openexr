// [begin readTiled1]
void
readTiled1 (const char fileName[], Array2D<GZ>& pixels, int& width, int& height)
{
    TiledInputFile in (fileName);

    Box2i dw = in.header ().dataWindow ();
    width    = dw.max.x - dw.min.x + 1;
    height   = dw.max.y - dw.min.y + 1;

    int dx = dw.min.x;
    int dy = dw.min.y;

    pixels.resizeErase (height, width);

    FrameBuffer frameBuffer;

    frameBuffer.insert (
        "G",
        Slice (
            HALF,
            (char*) &pixels[-dy][-dx].g,
            sizeof (pixels[0][0]) * 1,
            sizeof (pixels[0][0]) * width));

    frameBuffer.insert (
        "Z",
        Slice (
            FLOAT,
            (char*) &pixels[-dy][-dx].z,
            sizeof (pixels[0][0]) * 1,
            sizeof (pixels[0][0]) * width));

    in.setFrameBuffer (frameBuffer);
    in.readTiles (0, in.numXTiles () - 1, 0, in.numYTiles () - 1);
}
// [end readTiled1]
void
readTiledOtherVersions (const char fileName[])
{
    // read tile function versions
    TiledInputFile in(fileName);
    int tileX = 0, tileY = 0, levelX = 0, levelY = 0;
    int tileXMin = 0, tileXMax = 0, tileYMin = 0, tileYMax = 0;
    // [begin v1]
    in.readTile (tileX, tileY, levelX, levelY);
    // [end v1]
    in.readTiles (tileXMin, tileXMax, tileYMin, tileYMax, levelX, levelY);
    // [end v2]
}
