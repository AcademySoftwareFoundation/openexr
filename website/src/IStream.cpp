class IStream
{
  public:
    virtual ~IStream ();

    virtual bool read (char c[], int n) = 0;
    virtual uint64_t tellg () = 0;
    virtual void seekg (uint64_t pos) = 0;
    virtual void clear ();
    const char * fileName () const;
    virtual bool isMemoryMapped () const;
    virtual char * readMemoryMapped (int n);

  protected:
    IStream (const char fileName[]);
    private:

    // ...
};
    
