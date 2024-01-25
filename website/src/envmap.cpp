
char fileName[] = "";
// [begin hasEnvmap]
RgbaInputFile file (fileName);

if (hasEnvmap (file.header()))
{
    Envmap type = envmap (file.header());
    // ...
}
// [end hasEnvmap]
