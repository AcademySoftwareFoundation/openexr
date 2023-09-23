bool
MemoryMappedIStream::read (char c[], int n)
{
    if (_readPosition >= _fileLength)
        throw InputExc ("Unexpected end of file.");
    
    if (_readPosition + n > _fileLength)
        throw InputExc ("Reading past end of file.");

    memcpy (c, _buffer + _readPosition, n);

    _readPosition += n;

    return _readPosition < _fileLength;

}

uint64_t
MemoryMappedIStream::tellg ()
{
    return _readPosition;
}

void
MemoryMappedIStream::seekg (uint64_t pos)
{
    _readPosition = pos;
}

