#include <fstream>
// [begin validFileCheck]
bool
isThisAnOpenExrFile (const char fileName[])
{
    std::ifstream f (fileName, std::ios_base::binary);
    
    char b[4];
    f.read (b, sizeof (b));
    
    return !!f && b[0] == 0x76 && b[1] == 0x2f && b[2] == 0x31 && b[3] == 0x01;
}
// [end validFileCheck]
// [begin completeFileCheck]
bool
isComplete (const char fileName[])
{
    InputFile in (fileName);
    return in.isComplete();
}
// [end completeFileCheck]

// [begin otherValidFileChecks]
bool isOpenExrFile (const char fileName[], bool &isTiled);
    
bool isOpenExrFile (const char fileName[]);
    
bool isTiledOpenExrFile (const char fileName[]);
    
bool isOpenExrFile (IStream &is, bool &isTiled);
    
bool isOpenExrFile (IStream &is);
    
bool isTiledOpenExrFile (IStream &is);
// [end otherValidFileChecks]