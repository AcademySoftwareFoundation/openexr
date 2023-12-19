// [begin writeRgba2]
void
writeRgba2 (
    const char   fileName[],
    const Rgba*  pixels,
    int          width,
    int          height,
    const Box2i& dataWindow)
{
    Box2i          displayWindow (V2i (0, 0), V2i (width - 1, height - 1));
    RgbaOutputFile file (fileName, displayWindow, dataWindow, WRITE_RGBA);
    file.setFrameBuffer (pixels, 1, width);
    file.writePixels (dataWindow.max.y - dataWindow.min.y + 1);
}
// [end writeRgba2]

void
writeRgba2ResizeFrameBuffer (
    const char   fileName[],
    const Rgba*  pixels,
    int          width,
    int          height,
    const Box2i& dataWindow)
{
    Box2i          displayWindow (V2i (0, 0), V2i (width - 1, height - 1));
    RgbaOutputFile file (fileName, displayWindow, dataWindow, WRITE_RGBA);
    // [begin writeRgba2ResizeFrameBuffer]
    int dwWidth = dataWindow.max.x - dataWindow.min.x + 1;
    file.setFrameBuffer (pixels - dataWindow.min.x - dataWindow.min.y * dwWidth, 1, dwWidth);
    // [end writeRgba2ResizeFrameBuffer]
}

