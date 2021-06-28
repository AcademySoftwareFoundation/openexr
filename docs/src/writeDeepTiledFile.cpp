void
writeDeepTiledFile(const char filename[],
                   Box2i displayWindow,
                   Box2i dataWindow,
                   int tileSizeX, int tileSizeY)
{
    int height = dataWindow.max.y - dataWindow.min.y + 1;
    int width = dataWindow.max.x - dataWindow.min.x + 1;

    Header header(displayWindow, dataWindow);
    header.channels().insert("Z", Channel(FLOAT));
    header.channels().insert("A", Channel(HALF));
    header.setType(DEEPTILE);

    header.setTileDescription(TileDescription(tileSizeX, tileSizeY, MIPMAP_LEVELS));

    Array2D< unsigned int* > dataZ;
    dataZ.resizeErase(height, width);

    Array2D< unsigned int* > dataA;
    dataA.resizeErase(height, width);

    Array2D<unsigned int> sampleCount;
    sampleCount.resizeErase(height, width);

    DeepTiledOutputFile file(filename, header);

    DeepFrameBuffer frameBuffer;

    frameBuffer.insertSampleCountSlice (Slice (UINT,
                                               (char *) (&sampleCount[0][0]
                                                          - dataWindow.min.x
                                                          - dataWindow.min.y * width),
                                               sizeof (unsigned int) * 1, // xStride
                                               sizeof (unsigned int) * width)); // yStride

    frameBuffer.insert ("Z",
                        DeepSlice (FLOAT,
                                   (char *) (&dataZ[0][0]
                                              - dataWindow.min.x
                                              - dataWindow.min.y * width),
                                   sizeof (float *) * 1, // xStride for pointer array
                                   sizeof (float *) * width, // yStride for pointer array
                                   sizeof (float) * 1)); // stride for samples

    frameBuffer.insert ("A",
                        DeepSlice (HALF,
                                   (char *) (&dataA[0][0]
                                              - dataWindow.min.x
                                              - dataWindow.min.y * width),
                                   sizeof (half *) * 1, // xStride for pointer array
                                   sizeof (half *) * width, // yStride for pointer array
                                   sizeof (half) * 1)); // stride for samples

    file.setFrameBuffer(frameBuffer);

    for (int l = 0; l < file.numLevels(); l++)
    {
        for (int j = 0; j < file.numYTiles(l); j++)
        {
            for (int i = 0; i < file.numXTiles(l); i++)
            {
                getSampleCountForTile(i, j, sampleCount);
                getSampleDataForTile(i, j, dataZ, dataA);
                file.writeTile(i, j, l);
            }
        }
    }
}

