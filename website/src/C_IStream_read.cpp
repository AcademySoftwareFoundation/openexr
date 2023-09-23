bool
C_IStream::read (char c[], int n)
{
    if (n != static_cast <int> (fread (c, 1, n, _file)))
    {
        // fread() failed, but the return value does not distinguish
        // between I/O errors and end of file, so we call ferror() to
        // determine what happened.
    
        if (ferror (_file))
            throwErrnoExc();
        else
            throw InputExc ("Unexpected end of file.");
    }
    
    return !feof (_file);
}

