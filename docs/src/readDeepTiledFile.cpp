void
readDeepTiledFile(const char filename[],
                  Box2i& displayWindow,
                  Box2i& dataWindow,
                  Array2D< float* >& dataZ,
                  Array2D< half* >& dataA,
                  Array2D< unsigned int >& sampleCount)
{
    DeepTiledInputFile file(filename);

    int width = dataWindow.max.x - dataWindow.min.x + 1;
    int height = dataWindow.max.y - dataWindow.min.y + 1;

    sampleCount.resizeErase(height, width);
    dataZ.resizeErase(height, width);
    dataA.resizeErase(height, width);

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

    int numXTiles = file.numXTiles(0);
    int numYTiles = file.numYTiles(0);

    file.readPixelSampleCounts(0, numXTiles - 1, 0, numYTiles - 1);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            dataZ[i][j] = new float[sampleCount[i][j]];
            dataA[i][j] = new half[sampleCount[i][j]];
        }

    file.readTiles(0, numXTiles - 1, 0, numYTiles – 1);

    // (after read data is processed, data must be freed:)

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            delete[] dataZ[i][j];
            delete[] dataA[i][j];
        }
    }
}    


