void
writeTiledRgbaONE2 (const char fileName[],
                    int width, int height,
                    int tileWidth, int tileHeight)
{
    TiledRgbaOutputFile out (fileName,
                             width, height,            // image size
                             tileWidth, tileHeight,    // tile size
                             ONE_LEVEL,                // level mode
                             ROUND_DOWN,               // rounding mode
                             WRITE_RGBA);              // channels in file // 1

    Array2D<Rgba> pixels (tileHeight, tileWidth);                          // 2

    for (int tileY = 0; tileY < out.numYTiles (); ++tileY)                 // 3
    {
        for (int tileX = 0; tileX < out.numXTiles (); ++tileX)             // 4
        {
            Box2i range = out.dataWindowForTile (tileX, tileY);            // 5

            generatePixels (pixels, width, height, range);                 // 6

            out.setFrameBuffer (&pixels[-range.min.y][-range.min.x],
                                1,                              // xStride
                                tileWidth);                     // yStride // 7

            out.writeTile (tileX, tileY);                                  // 8
        }
    }
}

