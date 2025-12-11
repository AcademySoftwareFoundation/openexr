//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

char *
MemoryMappedIStream::readMemoryMapped (int n)
{
    if (_readPosition >= _fileLength)
        throw InputExc ("Unexpected end of file.");

    if (_readPosition + n > _fileLength)
        throw InputExc ("Reading past end of file.");

    char *data = _buffer + _readPosition;

    _readPosition += n;

    return data;

}

