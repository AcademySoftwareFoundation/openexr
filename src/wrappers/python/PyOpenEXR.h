//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

typedef Array2D<void*> Array2DVoidPtr;
typedef std::map<std::string,std::unique_ptr<Array2DVoidPtr>> SliceDataMap;

//
// PyFile is the object that corresponds to an exr file, either for reading
// or writing, consisting of a simple list of parts.
//

class PyPart;
class PyChannel;

class PyFile 
{
public:
    PyFile() {}
    PyFile(const std::string& filename, bool separate_channels = false, bool header_only = false);
    PyFile(const py::dict& header, const py::dict& channels);
    PyFile(const py::list& parts);

    py::object   __enter__();
    void         __exit__(py::args args);
    
    py::dict&    header(int part_index = 0);
    py::dict&    channels(int part_index = 0);

    void         write(const char* filename);
    
    std::string  filename;
    py::list     parts;

protected:
    
    bool         header_only;
    
    py::object   getAttributeObject(const std::string& name, const Attribute* a);
    
    void         insertAttribute(Header& header,
                                 const std::string& name,
                                 const py::object& object);

};

//
// PyPart holds the information for a part of an exr file: name, type,
// dimension, compression, the list of attributes (e.g. "header") and the
// list of channels.
//

class PyPart
{
  public:
    PyPart() {}
    PyPart(const py::dict& header, const py::dict& channels, const std::string& name);
    
    std::string    name() const;
    V2i            shape() const;
    size_t         width() const;
    size_t         height() const;
    Compression    compression() const;
    exr_storage_t  type() const;
    std::string    typeString() const;
    
    py::dict       header;
    py::dict       channels;

    size_t         part_index;

    void           writePixels(MultiPartOutputFile& outfile, const Box2i& dw) const;
    void           writeDeepPixels(MultiPartOutputFile& outfile, const Box2i& dw) const;
    
    void           setDeepSliceData(const ChannelList& channel_list, size_t height, size_t width,
                                    SliceDataMap& sliceDataMap,
                                    std::map<std::string,PyChannel*>& rgbaChannelMap,
                                    const Array2D<unsigned int>& sampleCount);

    void           readPixels(MultiPartInputFile& infile, const ChannelList& channel_list,
                              const std::vector<size_t>& shape, const std::set<std::string>& rgbaChannels,
                              const Box2i& dw, bool separate_channels);
    void           readDeepPixels(MultiPartInputFile& infile, const std::string& type, const ChannelList& channel_list,
                                  const std::vector<size_t>& shape, const std::set<std::string>& rgbaChannels,
                                  const Box2i& dw, bool separate_channels);
    int            channelNameToRGBA(const ChannelList& channel_list, const std::string& name,
                                     std::string& py_channel_name, char& channel_name);
    
};

//
// PyChannel holds information for a channel of a PyPart: name, type, x/y
// sampling, and the array of pixel data.
//
  
class PyChannel 
{
public:

    PyChannel()
        : xSampling(1), ySampling(1), pLinear(false), channel_index(0),
          _type(NUM_PIXELTYPES), _nrgba(0) {}

    PyChannel(int xSampling, int ySampling, bool pLinear = false)
        : xSampling(xSampling), ySampling(ySampling), pLinear(pLinear), channel_index(0),
          _type(NUM_PIXELTYPES), _nrgba(0) {}
    PyChannel(const py::array& p)
        : xSampling(1), ySampling(1), pLinear(false), pixels(p), channel_index(0),
          _type(NUM_PIXELTYPES), _nrgba(0) { validatePixelArray(); }
    PyChannel(const py::array& p, int xSampling, int ySampling, bool pLinear = false)
        : xSampling(xSampling), ySampling(ySampling), pLinear(pLinear), pixels(p), channel_index(0),
          _type(NUM_PIXELTYPES), _nrgba(0) { validatePixelArray(); }
        
    PyChannel(const char* n)
        : name(n), xSampling(1), ySampling(1), pLinear(false), channel_index(0),
          _type(NUM_PIXELTYPES), _nrgba(0) {}
    PyChannel(const char* n, int xSampling, int ySampling, bool pLinear = false)
        : name(n), xSampling(xSampling), ySampling(ySampling), pLinear(pLinear), channel_index(0),
          _type(NUM_PIXELTYPES), _nrgba(0) {}
    PyChannel(const char* n, const py::array& p)
        : name(n), xSampling(1), ySampling(1), pLinear(false), pixels(p), channel_index(0),
          _type(NUM_PIXELTYPES), _nrgba(0) { validatePixelArray(); }
    PyChannel(const char* n, const py::array& p, int xSampling, int ySampling, bool pLinear = false)
        : name(n), xSampling(xSampling), ySampling(ySampling), pLinear(pLinear), pixels(p), channel_index(0),
          _type(NUM_PIXELTYPES), _nrgba(0) { validatePixelArray(); }

    PixelType             pixelType() const;

    std::string           name;
    int                   xSampling;
    int                   ySampling;
    int                   pLinear;
    py::array             pixels;
    size_t                channel_index;

    mutable PixelType      _type;
    mutable int            _nrgba;

    void                   validatePixelArray();
    template<class T> void setSliceDataPtr(Array2DVoidPtr& slice_data,const py::array& a,
                                           size_t y, size_t x,
                                           int channel_offset, PixelType type) const;
    void                   createDeepPixelArrays(size_t height, size_t width,
                                                 const Array2D<unsigned int>& sampleCount);
    void                   insertDeepSlice(DeepFrameBuffer& frameBuffer, const std::string& slice_name,
                                           size_t height, size_t width, int nrgba,
                                           int dw_offset, int channel_offset,
                                           Array2D<unsigned int>& sampleCount,
                                           std::vector<std::shared_ptr<Array2DVoidPtr>>& slice_datas) const;
};
    
class PyPreviewImage
{
public:
    static constexpr uint32_t style = py::array::c_style | py::array::forcecast;
    static constexpr size_t stride = sizeof(PreviewRgba);

    PyPreviewImage() {}
    
    PyPreviewImage(unsigned int width, unsigned int height,
                   const PreviewRgba* data = nullptr)
        : pixels(py::array_t<PreviewRgba,style>(std::vector<size_t>({height, width}),
                                                std::vector<size_t>({stride*width, stride}),
                                                data)) {}
    
    PyPreviewImage(const py::array_t<PreviewRgba>& p) : pixels(p) {}

    inline bool operator==(const PyPreviewImage& other) const;

    py::array_t<PreviewRgba> pixels;
};
    
inline std::ostream&
operator<< (std::ostream& s, const PreviewRgba& p)
{
    s << " (" << int(p.r)
      << "," << int(p.g)
      << "," << int(p.b)
      << "," << int(p.a)
      << ")";
    return s;
}

inline std::ostream&
operator<< (std::ostream& s, const PyPreviewImage& P)
{
    auto width = P.pixels.shape(1);
    auto height = P.pixels.shape(0);
    
    s << "PreviewImage(" << width
      << ", " << height;
#if PRINT_PIXELS
    s << "," << std::endl;
    py::buffer_info buf = P.pixels.request();
    const PreviewRgba* rgba = static_cast<PreviewRgba*>(buf.ptr);
    for (decltype(height) y = 0; y<height; y++)
    {
        for (decltype(width) x = 0; x<width; x++)
            s << rgba[y*width+x];
        s << std::endl;
    }
#endif
    s << ")";
    return s;
}
    
inline bool
PyPreviewImage::operator==(const PyPreviewImage& other) const
{
    py::buffer_info buf = pixels.request();
    py::buffer_info obuf = other.pixels.request();
    
    const PreviewRgba* apixels = static_cast<PreviewRgba*>(buf.ptr);
    const PreviewRgba* bpixels = static_cast<PreviewRgba*>(obuf.ptr);
    for (decltype(buf.size) i = 0; i < buf.size; i++)
        if (!(apixels[i] == bpixels[i]))
            return false;
    return true;
}


inline std::ostream&
operator<< (std::ostream& s, const Chromaticities& c)
{
    s << "(" << c.red
      << ", " << c.green
      << ", " << c.blue
      << ", " << c.white
      << ")";
    return s;
}
    
inline std::ostream&
operator<< (std::ostream& s, const Rational& v)
{
    s << v.n << "/" << v.d;
    return s;
}

inline std::ostream&
operator<< (std::ostream& s, const KeyCode& v)
{
    s << "(" << v.filmMfcCode()
      << ", " << v.filmType()
      << ", " << v.prefix()
      << ", " << v.count()
      << ", " << v.perfOffset()
      << ", " << v.perfsPerFrame()
      << ", " << v.perfsPerCount()
      << ")";
    return s;
}

inline std::ostream&
operator<< (std::ostream& s, const TimeCode& v)
{
    s << "(" << v.hours()
      << ", " << v.minutes()
      << ", " << v.seconds()
      << ", " << v.frame()
      << ", " << v.dropFrame()
      << ", " << v.colorFrame()
      << ", " << v.fieldPhase()
      << ", " << v.bgf0()
      << ", " << v.bgf1()
      << ", " << v.bgf2()
      << ")";
    return s;
}


inline std::ostream&
operator<< (std::ostream& s, const TileDescription& v)
{
    s << "TileDescription(" << v.xSize
      << ", " << v.ySize
      << ", " << py::cast(v.mode)
      << ", " << py::cast(v.roundingMode)
      << ")";

    return s;
}

inline std::ostream&
operator<< (std::ostream& s, const Box2i& v)
{
    s << "(" << v.min << "  " << v.max << ")";
    return s;
}

inline std::ostream&
operator<< (std::ostream& s, const Box2f& v)
{
    s << "(" << v.min << "  " << v.max << ")";
    return s;
}

inline std::ostream&
operator<< (std::ostream& s, const PyChannel& C)
{
    s << "Channel(\"" << C.name 
      << "\", xSampling=" << C.xSampling 
      << ", ySampling=" << C.ySampling;
    if (C.pLinear)
        s << ", pLinear=True";
    s << ")";
    return s;
}

inline std::ostream&
operator<< (std::ostream& s, const PyPart& P)
{
    auto name = P.name();
    s << "Part(";
    if (name != "")
        s << "\"" << name << "\"";
    s << ", " << py::cast(P.compression())
      << ", width=" << P.width()
      << ", height=" << P.height()
      << ")";
    return s;
}

