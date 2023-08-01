MemoryMappedIStream::MemoryMappedIStream (const char fileName[])
   : IStream (fileName),
     _buffer (0),
     _fileLength (0),
     _readPosition (0)
{
    int file = open (fileName, O_RDONLY);

    if (file < 0)
        THROW_ERRNO ("Cannot open file \"" << fileName << "\".");

    struct stat stat;
    fstat (file, &stat);

    _fileLength = stat.st_size;

    _buffer = (char *) mmap (0, _fileLength, PROT_READ, MAP_PRIVATE, file, 0);

    close (file);

    if (_buffer == MAP_FAILED)
        THROW_ERRNO ("Cannot memory-map file \"" << fileName << "\".");
}
