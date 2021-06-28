void
writeTiled1 (const char fileName[],
             Array2D<GZ> &pixels,
             int width, int height,
             int tileWidth, int tileHeight)
{

    Header header (width, height); // 1
    header.channels().insert ("G", Channel (HALF)); // 2
    header.channels().insert ("Z", Channel (FLOAT)); // 3

    header.setTileDescription (TileDescription (tileWidth, tileHeight, ONE_LEVEL)); // 4

    TiledOutputFile out (fileName, header); // 5

    FrameBuffer frameBuffer; // 6

    frameBuffer.insert ("G", // name // 7
                        Slice (HALF, // type // 8
                               (char *) &pixels[0][0].g, // base // 9
                               sizeof (pixels[0][0]) * 1, // xStride // 10
                               sizeof (pixels[0][0]) * width)); // yStride // 11

    frameBuffer.insert ("Z", // name // 12
                        Slice (FLOAT, // type // 13
                               (char *) &pixels[0][0].z, // base // 14
                               sizeof (pixels[0][0]) * 1, // xStride // 15
                               sizeof (pixels[0][0]) * width)); // yStride // 16

    out.setFrameBuffer (frameBuffer); // 17
    out.writeTiles (0, out.numXTiles() - 1, 0, out.numYTiles() - 1); // 18
}
