uint64_t
C_IStream::tellg ()
{
    return ftell (_file);
}

