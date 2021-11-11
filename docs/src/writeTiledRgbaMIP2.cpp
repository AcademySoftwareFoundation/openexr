void
writeTiledRgbaMIP2 (const char fileName[],
                    int width, int height,
                    int tileWidth, int tileHeight)
{
    TiledRgbaOutputFile out (fileName,
                             width, height,
                             tileWidth, tileHeight,
                             MIPMAP_LEVELS,
                             ROUND_DOWN,
                             WRITE_RGBA);

    Array2D<Rgba> pixels (tileHeight, tileWidth);

    for (int level = 0; level < out.numLevels (); ++level)
    {
        for (int tileY = 0; tileY < out.numYTiles (level); ++tileY)
        {
            for (int tileX = 0; tileX < out.numXTiles (level); ++tileX)
            {

                Box2i range = out.dataWindowForTile (tileX, tileY, level);

                generatePixels (pixels, width, height, range, level);

                out.setFrameBuffer (&pixels[-range.min.y][-range.min.x],
                                    1,          // xStride
                                    tileWidth); // yStride

                out.writeTile (tileX, tileY, level);
            }
        }
    }
}


