char *
MemoryMappedIStream::readMemoryMapped (int n)
{
    if (_readPosition >= _fileLength)
        throw IEX_NAMESPACE::InputExc ("Unexpected end of file.");

    if (_readPosition + n > _fileLength)
        throw IEX_NAMESPACE::InputExc ("Reading past end of file.");

    char *data = _buffer + _readPosition;

    _readPosition += n;

    return data;

}

