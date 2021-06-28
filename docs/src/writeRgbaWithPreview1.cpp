void
writeRgbaWithPreview1 (const char fileName[],
                       const Array2D<Rgba> &pixels,
                       int width,
                       int height)
{
    Array2D <PreviewRgba> previewPixels; // 1

    int previewWidth; // 2
    int previewHeight; // 3

    makePreviewImage (pixels, width, height, // 4
                      previewPixels, previewWidth, previewHeight);

    Header header (width, height); // 5
    header.setPreviewImage (PreviewImage (previewWidth, previewHeight, &previewPixels[0][0]));

    RgbaOutputFile file (fileName, header, WRITE_RGBA); // 7
    file.setFrameBuffer (&pixels[0][0], 1, width); // 8
    file.writePixels (height); // 9
}
