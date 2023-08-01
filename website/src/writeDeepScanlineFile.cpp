void
writeDeepScanLineFile (
    const char       filename[],
    Box2i            displayWindow,
    Box2i            dataWindow,
    Array2D<float*>& dataZ,

    Array2D<half*>& dataA,

    Array2D<unsigned int>& sampleCount)

{
    int height = dataWindow.max.y - dataWindow.min.y + 1;
    int width  = dataWindow.max.x - dataWindow.min.x + 1;

    Header header (displayWindow, dataWindow);

    header.channels ().insert ("Z", Channel (FLOAT));
    header.channels ().insert ("A", Channel (HALF));
    header.setType (DEEPSCANLINE);
    header.compression () = ZIPS_COMPRESSION;

    DeepScanLineOutputFile file (filename, header);

    DeepFrameBuffer frameBuffer;

    frameBuffer.insertSampleCountSlice (Slice (
        UINT,
        (char*) (&sampleCount[0][0] - dataWindow.min.x - dataWindow.min.y * width),
        sizeof (unsigned int) * 1, // xS

        sizeof (unsigned int) * width)); // yStride

    frameBuffer.insert (
        "Z",
        DeepSlice (
            FLOAT,
            (char*) (&dataZ[0][0] - dataWindow.min.x - dataWindow.min.y * width),
            sizeof (float*) * 1, // xStride for pointer

            sizeof (float*) * width, // yStride for pointer array
            sizeof (float) * 1));    // stride for Z data sample

    frameBuffer.insert (
        "A",
        DeepSlice (
            HALF,
            (char*) (&dataA[0][0] - dataWindow.min.x - dataWindow.min.y * width),
            sizeof (half*) * 1,     // xStride for pointer array
            sizeof (half*) * width, // yStride for pointer array
            sizeof (half) * 1));    // stride for A data sample

    file.setFrameBuffer (frameBuffer);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            sampleCount[i][j] = getPixelSampleCount (i, j);
            dataZ[i][j]       = new float[sampleCount[i][j]];
            dataA[i][j]       = new half[sampleCount[i][j]];
            // Generate data for dataZ and dataA.
            getPixelSampleData(i, j, dataZ, dataA);
        }

        file.writePixels (1);
    }

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            delete[] dataZ[i][j];
            delete[] dataA[i][j];
        }
    }
}
