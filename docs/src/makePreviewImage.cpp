void
makePreviewImage (const Array2D<Rgba> &pixels,
                  int width,
                  int height,
                  Array2D<PreviewRgba> &previewPixels,
                  int &previewWidth,
                  int &previewHeight)
{
    const int N = 8;

    previewWidth = width / N;
    previewHeight = height / N;

    previewPixels.resizeErase (previewHeight, previewWidth);

    for (int y = 0; y < previewHeight; ++y)
    {
        for (int x = 0; x < previewWidth; ++x)
        {

            const Rgba &inPixel = pixels[y * N][x * N];
            PreviewRgba &outPixel = previewPixels[y][x];

            outPixel.r = gamma (inPixel.r);
            outPixel.g = gamma (inPixel.g);
            outPixel.b = gamma (inPixel.b);
            outPixel.a = int (clamp (inPixel.a * 255.f, 0.f, 255.f) + 0.5f);
        }
    }
}
