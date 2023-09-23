char *
MemoryMappedIStream::readMemoryMapped (int n)
{
    if (_readPosition >= _fileLength)
        throw Iex::InputExc ("Unexpected end of file.");

    if (_readPosition + n > _fileLength)
        throw Iex::InputExc ("Reading past end of file.");

    char *data = _buffer + _readPosition;

    _readPosition += n;

    return data;

}

