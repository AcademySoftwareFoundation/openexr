class C_IStream: public IStream
{
  public:
    C_IStream (FILE *file, const char fileName[]):
        IStream (fileName), _file (file) {}

    virtual bool read (char c[], int n);
    virtual uint64_t tellg ();
    virtual void seekg (uint64_t pos);
    virtual void clear ();

  private:

    FILE * _file;
};
