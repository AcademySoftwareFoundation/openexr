class MemoryMappedIStream: public IStream
{
  public:
    MemoryMappedIStream (const char fileName[]);

    virtual ~MemoryMappedIStream ();

    virtual bool isMemoryMapped () const;
    virtual char * readMemoryMapped (int n);
    virtual bool read (char c[], int n);
    virtual uint64_t tellg ();

    virtual void seekg (uint64_t pos);

  private:

    char * _buffer;
    uint64_t _fileLength;
    uint64_t _readPosition;
};

