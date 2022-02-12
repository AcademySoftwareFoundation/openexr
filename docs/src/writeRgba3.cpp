void
writeRgba3 (
    const char  fileName[],
    const Rgba* pixels,
    int         width,
    int         height,
    const char  comments[],
    const M44f& cameraTransform)
{
    Header header (width, height);

    header.insert ("comments", StringAttribute (comments));
    header.insert ("cameraTransform", M44fAttribute (cameraTransform));

    RgbaOutputFile file (fileName, header, WRITE_RGBA);
    file.setFrameBuffer (pixels, 1, width);
    file.writePixels (height);
}
