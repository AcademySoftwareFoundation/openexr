char fileName[100]; 
const Rgba* pixels;
int width; 
int height;
// [begin]
try
{
    writeRgba1 (fileName, pixels, width, height);
}
catch (const std::exception &exc)
{
    std::cerr << exc.what() << std::endl;
}
// [end]
