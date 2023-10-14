
char fileName[100];
// [begin hasEnvmap]
RgbaInputFile file (fileName);

if (hasEnvmap (file.header()))
{
    Envmap type = envmap (file.header());
    // ...
}
// [end hasEnvmap]
