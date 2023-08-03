void
C_IStream::seekg (uint64_t pos)
{
    clearerr (_file);
    fseek (_file, pos, SEEK_SET);
}

