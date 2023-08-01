MemoryMappedIStream::~MemoryMappedIStream()
{
    munmap (_buffer, _fileLength);
}
