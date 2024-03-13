void
writeRgbaWithPreview2 (const char fileName[], int width, int height)
{
    Array<Rgba> pixels (width);

    const int N             = 8;
    int       previewWidth  = width / N;
    int       previewHeight = height / N;

    Array2D<PreviewRgba> previewPixels (previewHeight, previewWidth);

    Header header (width, height);
    header.setPreviewImage (PreviewImage (previewWidth, previewHeight));

    RgbaOutputFile file (fileName, header, WRITE_RGBA);
    file.setFrameBuffer (pixels, 1, 0);

    for (int y = 0; y < height; ++y)
    {
        generatePixels (pixels, width, height, y);

        file.writePixels (1);

        if (y % N == 0)
        {
            for (int x = 0; x < width; x += N)
            {
                const Rgba&  inPixel  = pixels[x];
                PreviewRgba& outPixel = previewPixels[y / N][x / N];

                outPixel.r = gamma (inPixel.r);
                outPixel.g = gamma (inPixel.g);
                outPixel.b = gamma (inPixel.b);
                outPixel.a = int (Imath::clamp (inPixel.a * 255.f, 0.f, 255.f) + 0.5f);
            }
        }
    }

    file.updatePreviewImage (&previewPixels[0][0]);
}
