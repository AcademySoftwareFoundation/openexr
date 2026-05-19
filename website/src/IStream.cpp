#include <cstdint>

class IStream
{
  public:
    IStream(const char[]) {}

    virtual ~IStream() = default;

    virtual bool read (char c[], int n) = 0;
    virtual uint64_t tellg () = 0;
    virtual void seekg (uint64_t pos) = 0;
    virtual void clear () = 0;

    virtual bool isMemoryMapped() const
    {
        return false;
    }

    virtual char* readMemoryMapped (int)
    {
        return nullptr;
    }
};
    
