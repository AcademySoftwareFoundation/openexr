void
accessPreviewImage (const char fileName[])
{
    // [begin accessPreviewImage]
    RgbaInputFile file (fileName);
        
    if (file.header().hasPreviewImage())
    {
        const PreviewImage &preview = file.header().previewImage();
        
        for (int y = 0; y < preview.height(); ++y)
        {
            for (int x = 0; x < preview.width(); ++x)
            {
        
                const PreviewRgba &pixel = preview.pixel (x, y);
        
                // ...
        
            }
        }
    }
    // [end accessPreviewImage]
}

// [begin gamma]
unsigned char
gamma (float x)
{
    x = pow (5.5555f * max (0.f, x), 0.4545f) * 84.66f;
    return (unsigned char) IMATH_NAMESPACE::clamp (x, 0.f, 255.f);
}
// [end gamma]

// [begin makePreviewImage]
void
makePreviewImage (
    const Array2D<Rgba>&  pixels,
    int                   width,
    int                   height,
    Array2D<PreviewRgba>& previewPixels,
    int&                  previewWidth,
    int&                  previewHeight)
{
    const int N = 8;

    previewWidth  = width / N;
    previewHeight = height / N;

    previewPixels.resizeErase (previewHeight, previewWidth);

    for (int y = 0; y < previewHeight; ++y)
    {
        for (int x = 0; x < previewWidth; ++x)
        {

            const Rgba&  inPixel  = pixels[y * N][x * N];
            PreviewRgba& outPixel = previewPixels[y][x];

            outPixel.r = gamma (inPixel.r);
            outPixel.g = gamma (inPixel.g);
            outPixel.b = gamma (inPixel.b);
            outPixel.a = static_cast<int> (IMATH_NAMESPACE::clamp (inPixel.a * 255.f, 0.f, 255.f) + 0.5f);
        }
    }
}
// [end makePreviewImage]
