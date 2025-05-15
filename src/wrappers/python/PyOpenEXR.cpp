//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//#define DEBUG_VERBOSE 1

#define PYBIND11_DETAILED_ERROR_MESSAGES 1

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/eval.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl_bind.h>
#include <pybind11/operators.h>

#include "openexr.h"

#include <ImfHeader.h>
#include <ImfMultiPartInputFile.h>
#include <ImfMultiPartOutputFile.h>
#include <ImfInputPart.h>
#include <ImfOutputPart.h>
#include <ImfTiledOutputPart.h>
#include <ImfDeepScanLineOutputPart.h>
#include <ImfDeepScanLineInputPart.h>
#include <ImfDeepTiledOutputPart.h>
#include <ImfDeepTiledInputPart.h>
#include <ImfDeepFrameBuffer.h>
#include <ImfPartType.h>
#include <ImfArray.h>

#include <ImfBoxAttribute.h>
#include <ImfChannelListAttribute.h>
#include <ImfChromaticitiesAttribute.h>
#include <ImfCompressionAttribute.h>
#include <ImfDoubleAttribute.h>
#include <ImfEnvmapAttribute.h>
#include <ImfFloatAttribute.h>
#include <ImfIntAttribute.h>
#include <ImfKeyCodeAttribute.h>
#include <ImfLineOrderAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfPreviewImageAttribute.h>
#include <ImfRationalAttribute.h>
#include <ImfStringAttribute.h>
#include <ImfStringVectorAttribute.h>
#include <ImfFloatVectorAttribute.h>
#include <ImfTileDescriptionAttribute.h>
#include <ImfTimeCodeAttribute.h>
#include <ImfVecAttribute.h>

#include <typeinfo>
#include <sys/types.h>

namespace py = pybind11;
using namespace py::literals;

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;

extern bool init_OpenEXR_old(PyObject* module);

namespace pybind11 {
namespace detail {

    // From https://github.com/AcademySoftwareFoundation/OpenImageIO/blob/master/src/python/py_oiio.h
    //
    // This half casting support for numpy was all derived from discussions
    // here: https://github.com/pybind/pybind11/issues/1776

    // Similar to enums in `pybind11/numpy.h`. Determined by doing:
    // python3 -c 'import numpy as np; print(np.dtype(np.float16).num)'
    constexpr int NPY_FLOAT16 = 23;

    template<> struct npy_format_descriptor<half> {
        static pybind11::dtype dtype()
        {
            handle ptr = npy_api::get().PyArray_DescrFromType_(NPY_FLOAT16);
            return reinterpret_borrow<pybind11::dtype>(ptr);
        }
        static std::string format()
        {
            // following: https://docs.python.org/3/library/struct.html#format-characters
            return "e";
        }
        static constexpr auto name = _("float16");
    };

}  // namespace detail
}  // namespace pybind11

namespace {

#include "PyOpenEXR.h"

//
// Create a PyFile out of a list of parts (i.e. a multi-part file)
//

PyFile::PyFile(const py::list& parts)
    : parts(parts)
{
    int part_index = 0;
    for (auto p : this->parts)
    {
        if (!py::isinstance<PyPart>(p))
            throw std::invalid_argument("must be a list of OpenEXR.Part() objects");

        PyPart& P = p.cast<PyPart&>();
        P.part_index = part_index++;
    }
}

//
// Create a PyFile out of a single part: header, channels,
// type, and compression (i.e. a single-part file)
//

PyFile::PyFile(const py::dict& header, const py::dict& channels)
{
    parts.append(py::cast<PyPart>(PyPart(header, channels, "")));
}

//
// Read a PyFile from the given filename.
//
// Create a 'Part' for each part in the file, even single-part files. The API
// has convenience methods for accessing the first part's header and
// channels, which for single-part files appears as the file's data.
//
// By default, read each channel into a numpy array of the appropriate pixel
// type: uint32, half, or float.
//
// If 'separate_channels' is false, gather 'R', 'G', 'B', and 'A' channels and interleave
// them into  a 3- or 4- (if 'A' is present) element numpy array. In the case
// of raw 'R', 'G', 'B', and 'A' channels, the corresponding key in the
// channels dict is "RGB" or "RGBA".  For channels with a prefix,
// e.g. "left.R", "left.G", etc, the channel key is the prefix.
//

PyFile::PyFile(const std::string& filename, bool separate_channels, bool header_only)
    : filename(filename), header_only(header_only)
{
    MultiPartInputFile infile(filename.c_str());

    for (int part_index = 0; part_index < infile.parts(); part_index++)
    {
        const Header& header = infile.header(part_index);

        PyPart P;

        P.part_index = part_index;
        
        const Box2i& dw = header.dataWindow();
        auto width = static_cast<size_t>(dw.max.x - dw.min.x + 1);
        auto height = static_cast<size_t>(dw.max.y - dw.min.y + 1);

        //
        // Fill the header dict with attributes from the input file header
        //
        
        for (auto a = header.begin(); a != header.end(); a++)
        {
            std::string name = a.name();
            const Attribute& attribute = a.attribute();
            P.header[py::str(name)] = getAttributeObject(name, &attribute);
        }

        //
        // If we're only reading the header, we're done.
        //
        
        if (header_only)
            continue;
        
        //
        // If we're gathering RGB channels, identify which channels to gather
        // by examining common prefixes.
        //
        
        std::set<std::string> rgbaChannels;
        if (!separate_channels)
        {
            for (auto c = header.channels().begin(); c != header.channels().end(); c++)
            {
                std::string py_channel_name;
                char channel_name;
                if (P.channelNameToRGBA(header.channels(), c.name(), py_channel_name, channel_name) > 0)
                    rgbaChannels.insert(c.name());
            }
        }
        
        std::vector<size_t> shape ({height, width});

        //
        // Read the channel data, different for image vs. deep
        //
        
        auto type = header.type();
        if (type == SCANLINEIMAGE || type == TILEDIMAGE)
        {
            P.readPixels(infile, header.channels(), shape, rgbaChannels, dw, separate_channels);
        }
        else if (type == DEEPSCANLINE || type == DEEPTILE)
        {
            P.readDeepPixels(infile, type, header.channels(), shape, rgbaChannels, dw, separate_channels);
        }
        parts.append(py::cast<PyPart>(PyPart(P)));
    } // for parts
}

void
PyPart::readPixels(MultiPartInputFile& infile, const ChannelList& channel_list,
                   const std::vector<size_t>& shape, const std::set<std::string>& rgbaChannels,
                   const Box2i& dw, bool separate_channels)
{
    FrameBuffer frameBuffer;

    for (auto c = channel_list.begin(); c != channel_list.end(); c++)
    {
        std::string py_channel_name = c.name();
        char channel_name; 
        int nrgba = 0;
        if (!separate_channels)
            nrgba = channelNameToRGBA(channel_list, c.name(), py_channel_name, channel_name);
            
        auto py_channel_name_str = py::str(py_channel_name);
            
        if (!channels.contains(py_channel_name_str))
        {
            //
            // We haven't add a PyChannel yet, so add one now.
            //
            
            PyChannel C;

            C.name = py_channel_name;
            C.xSampling = c.channel().xSampling;
            C.ySampling = c.channel().ySampling;
            C.pLinear = c.channel().pLinear;
                
            const auto style = py::array::c_style | py::array::forcecast;

            std::vector<size_t> c_shape = shape;

            //
            // If this channel belongs to one of the rgba's, give
            // the PyChannel the extra dimension and the proper shape.
            // nrgba is 3 for RGB and 4 for RGBA.
            //
            
            if (rgbaChannels.find(c.name()) != rgbaChannels.end())
                c_shape.push_back(nrgba);

            switch (c.channel().type)
            {
              case UINT:
                  C.pixels = py::array_t<uint32_t,style>(c_shape);
                  break;
              case HALF:
                  C.pixels = py::array_t<half,style>(c_shape);
                  break;
              case FLOAT:
                  C.pixels = py::array_t<float,style>(c_shape);
                  break;
              default:
                  throw std::runtime_error("invalid pixel type");
            } // switch c->type

            channels[py_channel_name.c_str()] = C;
        }

        //
        // Add a slice to the framebuffer
        //
        
        auto v = channels[py_channel_name.c_str()];
        auto C = v.cast<PyChannel&>();

        py::buffer_info buf = C.pixels.request();
        auto basePtr = static_cast<uint8_t*>(buf.ptr);

        //
        // Offset the pointer for the channel
        //
        
        py::dtype dt = C.pixels.dtype();
        size_t xStride = dt.itemsize();
        if (nrgba > 0)
        {
            xStride *= nrgba;
            switch (channel_name)
            {
              case 'R':
                  break;
              case 'G':
                  basePtr += dt.itemsize();
                  break;
              case 'B':
                  basePtr += 2 * dt.itemsize();
                  break;
              case 'A':
                  basePtr += 3 * dt.itemsize();
                  break;
              default:
                  break;
            }
        }

        size_t yStride = xStride * shape[1] / C.xSampling;

        frameBuffer.insert (c.name(),
                            Slice::Make (c.channel().type,
                                         (void*) basePtr,
                                         dw, xStride, yStride,
                                         C.xSampling,
                                         C.ySampling));
    } // for header.channels()


    //
    // Read the pixels
    //
    
    InputPart part (infile, part_index);

    part.setFrameBuffer (frameBuffer);
    part.readPixels (dw.min.y, dw.max.y);
}

void
PyChannel::createDeepPixelArrays(size_t height, size_t width, const Array2D<unsigned int>& sampleCount)
{
    //
    // Create the py::array of the appropriate type and shape to hold the
    // samples for each pixel
    //
    
    std::vector<size_t> shape;
    shape.push_back(0);
    if (_nrgba > 0)
        shape.push_back(_nrgba); // shape=(count,3) for RGB; shape=(count,4) for RGBA

    py::object* pixel_objects = static_cast<py::object*>(pixels.mutable_data());

    for (size_t y=0; y<height; y++)
        for (size_t x=0; x<width; x++)
        {
            auto i = y * width + x;

            if (sampleCount[y][x] == 0)
            {
                //
                // No samples, no array.
                //
                
                pixel_objects[i] = py::none();
            }
            else
            {
                shape[0] = sampleCount[y][x];

                switch (_type)
                {
                  case UINT:
                      pixel_objects[i] = py::array_t<uint32_t>(shape);
                      break;
                  case HALF:
                      pixel_objects[i] = py::array_t<half>(shape);
                      break;
                  case FLOAT:
                      pixel_objects[i] = py::array_t<float>(shape);
                      break;
                  default:
                      throw std::runtime_error("invalid pixel type");
                } // switch _type
            }
        }
}

void
PyPart::setDeepSliceData(const ChannelList& channel_list, size_t height, size_t width,
                         SliceDataMap& sliceDataMap,
                         std::map<std::string,PyChannel*>& rgbaChannelMap,
                         const Array2D<unsigned int>& sampleCount)
{
    
    //
    // Now that we know the sample counts, create the sample array for
    // each pixel.  For RGB images, the sample arrays are of shape
    // (count, 3), or (count,4) if there's an A channel. For separate
    // channels, the arrays are 1D. The arrays are None for pixels with
    // no samples.
    //

    for (auto c : channels)
    {
        auto C = py::cast<PyChannel&>(c.second);
        C.createDeepPixelArrays(height, width, sampleCount);
    }
        
    for (auto c = channel_list.begin(); c != channel_list.end(); c++)
    {
        //
        // Set the slice data pointers to point into the pixel sample
        // arrays, with the proper offset/stride when coalescing channels
        // into RGB/RGBA.
        //
            
        auto &sliceData = *sliceDataMap[c.name()];

        PyChannel& C = *rgbaChannelMap[c.name()];
        py::object* pixel_objects = static_cast<py::object*>(C.pixels.mutable_data());

        size_t channel_offset = 0;
        if (C._nrgba > 0)
        {
            if (!strcmp(c.name(), "G"))
                channel_offset = 1;
            else if (!strcmp(c.name(), "B"))
                channel_offset = 2;
            else if (!strcmp(c.name(), "A"))
                channel_offset = 3;
        }

        for (size_t y=0; y<height; y++)
            for (size_t x=0; x<width; x++)
                if (sampleCount[y][x] == 0)
                    sliceData[y][x] = nullptr;
                else
                {
                    auto i = y * width + x;
                    auto a = py::cast<py::array>(pixel_objects[i]);
                    switch (C._type)
                    {
                    case UINT:
                        {
                            auto d = static_cast<uint32_t*>(a.request().ptr);
                            sliceData[y][x] = static_cast<void*>(&d[channel_offset]);
                        }
                        break;
                    case HALF:
                        {
                            auto d = static_cast<half*>(a.request().ptr);
                            sliceData[y][x] = static_cast<void*>(&d[channel_offset]);
                        }
                        break;
                    case FLOAT:
                        {
                            auto d = static_cast<float*>(a.request().ptr);
                            sliceData[y][x] = static_cast<void*>(&d[channel_offset]);
                        }
                        break;
                    case NUM_PIXELTYPES:
                        break;
                    }
                }
    }        
}

void
PyPart::readDeepPixels(MultiPartInputFile& infile, const std::string& type, const ChannelList& channel_list,
                       const std::vector<size_t>& shape, const std::set<std::string>& rgbaChannels,
                       const Box2i& dw, bool separate_channels)
{
    size_t width  = dw.max.x - dw.min.x + 1;
    size_t height = dw.max.y - dw.min.y + 1;
    auto dw_offset = dw.min.y * width + dw.min.x;

    Array2D<unsigned int> sampleCount (height, width);

    DeepFrameBuffer frameBuffer;

    frameBuffer.insertSampleCountSlice (Slice (UINT,
                                               (char*) (&sampleCount[0][0] - dw_offset),
                                               sizeof (unsigned int) * 1,       // xStride
                                               sizeof (unsigned int) * width)); // yStride

    //
    // Map from channel name to 2D array of pointers to sample arrays for
    // each slice.
    
    SliceDataMap sliceDataMap;
    
    //
    // The channel_list argument is the Imf::Header's list of channels.
    //
    // When building a py::dict of channels that coalesces "R", "G", "B", and
    // 'A' channels into "RGBA", the PyPart's channels py::dict has a single
    // entry for all 4 (or 3 if no alpha). rgbaChannelMap maps the
    // channel_list names (e.g. "R") to the PyChannel name (e.g. "RGBA").
    //
    //
    
    std::map<std::string,PyChannel*> rgbaChannelMap;

    for (auto c = channel_list.begin(); c != channel_list.end(); c++)
    {
        std::string py_channel_name = c.name();
        char channel_name; 
        int nrgba = 0;
        if (!separate_channels)
            nrgba = channelNameToRGBA(channel_list, c.name(), py_channel_name, channel_name);
        
        auto py_channel_name_str = py::str(py_channel_name);
            
        if (!channels.contains(py_channel_name_str))
        {
            // We haven't add a PyChannel yet, so add one now.
                
            channels[py_channel_name.c_str()] = PyChannel();
            PyChannel& C = channels[py_channel_name.c_str()].cast<PyChannel&>();
            C.name = py_channel_name;
            C.xSampling = c.channel().xSampling;
            C.ySampling = c.channel().ySampling;
            C.pLinear = c.channel().pLinear;

            C.pixels = py::array(py::dtype("O"), {height,width});

            C._type = c.channel().type;
            C._nrgba = nrgba;
        }

        auto v = channels[py_channel_name.c_str()];
        rgbaChannelMap[c.name()] = v.cast<PyChannel*>();
            
        size_t xStride = sizeof(void*);
        size_t yStride = xStride * shape[1];
        size_t sampleStride;
        switch (c.channel().type)
        {
        case UINT:
            sampleStride = sizeof(uint32_t);
            break;
        case HALF:
            sampleStride = sizeof(half);
            break;
        case FLOAT:
            sampleStride = sizeof(float);
            break;
        default:
            sampleStride = 0;
            break;
        }

        //
        // If coalescing RGBA, the sampleStride strides all of RGBA.
        //
        
        if (nrgba > 0)
            sampleStride *= nrgba;

        sliceDataMap[c.name()] = std::unique_ptr<Array2DVoidPtr>(new Array2DVoidPtr(height, width));
        Array2DVoidPtr* sliceData = sliceDataMap[c.name()].get();
        
        auto base = &(*sliceData)[0][0] - dw_offset;
        frameBuffer.insert (c.name(),
                            DeepSlice (c.channel().type,
                                       (char*) base,
                                       xStride,
                                       yStride,
                                       sampleStride,
                                       c.channel().xSampling,
                                       c.channel().ySampling));
    } // for header.channels()

    if (type == DEEPSCANLINE)
    {
        DeepScanLineInputPart part (infile, part_index);
        part.setFrameBuffer (frameBuffer);
        part.readPixelSampleCounts (dw.min.y, dw.max.y);

        setDeepSliceData(channel_list, height, width, sliceDataMap, rgbaChannelMap, sampleCount);

        part.readPixels (dw.min.y, dw.max.y);
    }
    else if (type == DEEPTILE)
    {
        DeepTiledInputPart part (infile, part_index);
        part.setFrameBuffer (frameBuffer);

        int numXTiles = part.numXTiles (0);
        int numYTiles = part.numYTiles (0);

        part.readPixelSampleCounts (0, numXTiles - 1, 0, numYTiles - 1);

        setDeepSliceData(channel_list, height, width, sliceDataMap, rgbaChannelMap, sampleCount);

        part.readTiles (0, numXTiles - 1, 0, numYTiles - 1);
    }
}

void
PyPart::writePixels(MultiPartOutputFile& outfile, const Box2i& dw) const
{
    FrameBuffer frameBuffer;
        
    for (auto c : channels)
    {
        auto C = c.second.cast<const PyChannel&>();

        auto pixelType = C.pixelType();
            
        if (C.pixels.ndim() == 3)
        {
            //
            // The py::dict has RGB or RGBA channels, but the
            // framebuffer needs a slice per dimension
            //
                    
            std::string name_prefix;
            if (C.name == "RGB" || C.name == "RGBA")
                name_prefix = "";
            else
                name_prefix = C.name + ".";
                
            py::buffer_info buf = C.pixels.request();
            auto basePtr = static_cast<uint8_t*>(buf.ptr);
            py::dtype dt = C.pixels.dtype();
            int nrgba = C.pixels.shape(2);
            size_t xStride = dt.itemsize() * nrgba;
            size_t yStride = xStride * width() / C.xSampling;
                    
            auto rPtr = basePtr;
            frameBuffer.insert (name_prefix + "R",
                                Slice::Make (pixelType,
                                             static_cast<void*>(rPtr),
                                             dw, xStride, yStride,
                                             C.xSampling,
                                             C.ySampling));

            auto gPtr = &basePtr[dt.itemsize()];
            frameBuffer.insert (name_prefix + "G",
                                Slice::Make (pixelType,
                                             static_cast<void*>(gPtr),
                                             dw, xStride, yStride,
                                             C.xSampling,
                                             C.ySampling));

            auto bPtr = &basePtr[2*dt.itemsize()];
            frameBuffer.insert (name_prefix + "B",
                                Slice::Make (pixelType,
                                             static_cast<void*>(bPtr),
                                             dw, xStride, yStride,
                                             C.xSampling,
                                             C.ySampling));

            if (nrgba == 4)
            {
                auto aPtr = &basePtr[3*dt.itemsize()];
                frameBuffer.insert (name_prefix + "A",
                                    Slice::Make (pixelType,
                                                 static_cast<void*>(aPtr),
                                                 dw, xStride, yStride,
                                                 C.xSampling,
                                                 C.ySampling));
            }
        }
        else
        {
            frameBuffer.insert (C.name,
                                Slice::Make (pixelType,
                                             static_cast<void*>(C.pixels.request().ptr),
                                             dw, 0, 0,
                                             C.xSampling,
                                             C.ySampling));
        }
    }
                
    if (type() == EXR_STORAGE_SCANLINE)
    {
        OutputPart part(outfile, part_index);
        part.setFrameBuffer (frameBuffer);
        part.writePixels (height());
    }
    else
    {
        TiledOutputPart part(outfile, part_index);
        part.setFrameBuffer (frameBuffer);
        part.writeTiles (0, part.numXTiles() - 1, 0, part.numYTiles() - 1);
    }
}

template<class T>
void
PyChannel::setSliceDataPtr(Array2DVoidPtr& sliceData,const py::array& a, size_t y, size_t x,
                              int channel_offset, PixelType type) const
{
    auto s = py::cast<const py::array_t<T>>(a);
    auto d = static_cast<const T*>(s.request().ptr);    
    const void* v = static_cast<const void*>(&d[channel_offset]);
    sliceData[y][x] = const_cast<void*>(v);

    if (_type == NUM_PIXELTYPES)
        _type = type;
    else if (_type != type)
    {
        std::stringstream err;
        err << "invalid deep pixel array at " << y << "," << x
            << ": all pixels must have same type of samples";
        throw std::invalid_argument(err.str());
    }
}

int
get_deep_nrgba(const py::array& pixels)
{
    const py::object* pixel_objects = static_cast<const py::object*>(pixels.data());

    size_t height = pixels.shape(0);
    size_t width = pixels.shape(1);
    for (size_t y = 0; y<height; y++)
        for (size_t x = 0; x<width; x++)
        {
            auto i = y * width + x;
            if (py::isinstance<py::array>(pixel_objects[i]))
            {
                auto a = py::cast<const py::array>(pixel_objects[i]);
                if (a.ndim() == 2)
                    return a.shape(1);
                return 0;
            }
        }

    return 0;
}
    
void
PyChannel::insertDeepSlice(DeepFrameBuffer& frameBuffer, const std::string& slice_name,
                           size_t height, size_t width, int nrgba, int dw_offset, int channel_offset,
                           Array2D<unsigned int>& sampleCount,
                           std::vector<std::shared_ptr<Array2DVoidPtr>>& sliceDatas) const
{
    Array2DVoidPtr* sliceDataPtr = new Array2DVoidPtr(height, width);
    Array2DVoidPtr& sliceData(*sliceDataPtr);
    sliceDatas.push_back(std::shared_ptr<Array2DVoidPtr>(sliceDataPtr));

    auto pixel_objects = static_cast<const py::object*>(pixels.data());

    for (size_t y=0; y<height; y++)
        for (size_t x=0; x<width; x++)
        {
            int i = y * width + x;
                
            auto object = pixel_objects[i];
            if (object.is(py::none()))
                continue;
            
            if (py::isinstance<py::array>(object))
            {
                auto a = object.cast<py::array>();
                if (sampleCount[y][x] == 0)
                    sampleCount[y][x] = a.shape(0);
                else if (sampleCount[y][x] != a.shape(0))
                {
                    std::stringstream err;
                    err << "invalid sample count at pixel " << y << "," << x
                        << ": all channels must have the same number of samples";
                    throw std::invalid_argument(err.str());
                }
                
                if (py::isinstance<py::array_t<uint32_t>>(a))
                    setSliceDataPtr<uint32_t>(sliceData, a, y, x, channel_offset, UINT);
                else if (py::isinstance<py::array_t<half>>(a))
                    setSliceDataPtr<half>(sliceData, a, y, x, channel_offset, HALF);
                else if (py::isinstance<py::array_t<float>>(a))
                    setSliceDataPtr<float>(sliceData, a, y, x, channel_offset, FLOAT);
                else
                {
                    std::stringstream err;
                    err << "invalid deep pixel array at " << y << "," << x
                        << ": unrecognized array type";
                    throw std::invalid_argument(err.str());
                }
            }
            else
            {
                std::stringstream err;
                err << "invalid deep pixel array at " << y << "," << x
                    << ": unrecognized object type";
                throw std::invalid_argument(err.str());
            }
        }

    size_t xStride = sizeof(void*);
    size_t yStride = xStride * width;
    size_t sampleStride;
    switch (_type)
    {
    case UINT:
        sampleStride = sizeof(uint32_t);
        break;
    case HALF:
        sampleStride = sizeof(half);
        break;
    case FLOAT:
        sampleStride = sizeof(float);
        break;
    default:
        sampleStride = 0;
        break;
    }
    if (nrgba > 0)
        sampleStride *= nrgba;

    void* base_ptr = &sliceData[0][0] - dw_offset;
    frameBuffer.insert (slice_name,
                        DeepSlice (_type,
                                   static_cast<char*>(base_ptr),
                                   xStride,
                                   yStride,
                                   sampleStride,
                                   xSampling,
                                   ySampling));
}

void
PyPart::writeDeepPixels(MultiPartOutputFile& outfile, const Box2i& dw) const
{
    size_t width  = dw.max.x - dw.min.x + 1;
    size_t height = dw.max.y - dw.min.y + 1;

    auto dw_offset = dw.min.y * width + dw.min.x;

    DeepFrameBuffer frameBuffer;
        
    Array2D<unsigned int> sampleCount (height, width);
    for (size_t y=0; y<height; y++)
        for (size_t x=0; x<width; x++)
            sampleCount[y][x] = 0;

    frameBuffer.insertSampleCountSlice (Slice (UINT,
                                               (char*) (&sampleCount[0][0] - dw_offset),
                                               sizeof (unsigned int) * 1,       // xStride
                                               sizeof (unsigned int) * width)); // yStride

    std::vector<std::shared_ptr<Array2DVoidPtr>> sliceDatas;

    for (auto c : channels)
    {
        const PyChannel& C = c.second.cast<const PyChannel&>();

        if (C.pixels.dtype().kind() != 'O')
            throw std::runtime_error("Expected deep pixel array with dtype 'O'");

        C._type = NUM_PIXELTYPES;
        
        int nrgba = get_deep_nrgba(C.pixels);
        if (nrgba == 0)
            C.insertDeepSlice(frameBuffer, C.name, height, width, nrgba, dw_offset, 0, sampleCount, sliceDatas);
        else
        {
            std::string name_prefix = "";
            if (C.name != "RGB" && C.name != "RGBA")
                name_prefix = C.name + ".";

            C.insertDeepSlice(frameBuffer, name_prefix+"R", height, width, nrgba, dw_offset, 0, sampleCount, sliceDatas);
            C.insertDeepSlice(frameBuffer, name_prefix+"G", height, width, nrgba, dw_offset, 1, sampleCount, sliceDatas);
            C.insertDeepSlice(frameBuffer, name_prefix+"B", height, width, nrgba, dw_offset, 2, sampleCount, sliceDatas);
            if (nrgba == 4)
                C.insertDeepSlice(frameBuffer, name_prefix+"A", height, width, nrgba, dw_offset, 3, sampleCount, sliceDatas);
        }
    }

    if (type() == EXR_STORAGE_DEEP_SCANLINE)
    {
        DeepScanLineOutputPart part(outfile, part_index);
        part.setFrameBuffer (frameBuffer);
        part.writePixels (height);
    }
    else 
    {
        DeepTiledOutputPart part(outfile, part_index);
        part.setFrameBuffer (frameBuffer);

        for (int y = 0; y < part.numYTiles (0); y++)
            for (int x = 0; x < part.numXTiles (0); x++)
                part.writeTile (x, y, 0);
    }
}

//
// Return whether "name" corresponds to one of the 'R', 'G', 'B', or 'A'
// channels in a "RGBA" tuple of channels. Return 4 if there's an 'A'
// channel, 3 if it's just RGB, and 0 otherwise.
//
// py_channel_name is returned as either the prefix, e.g. "left" for
// "left.R", "left.G", "left.B", or "RGBA" if the channel names are just 'R',
// 'G', and 'B'.
//
// This means:
//
//     channels["left"] = np.array((height,width,3))
// or:
//     channels["RGB"] = np.array((height,width,3))
//
// channel_name is returned as the single character name of the channel
//

int
PyPart::channelNameToRGBA(const ChannelList& channel_list, const std::string& name,
                          std::string& py_channel_name, char& channel_name)
{
    py_channel_name = name;
    channel_name = py_channel_name.back();
    if (channel_name == 'R' ||
        channel_name == 'G' ||
        channel_name == 'B' ||
        channel_name == 'A')
    {
        // It has the right final character. The preceding character is either a
        // '.' (in the case of "right.R", or empty (in the case of a channel
        // called "R")
        //
        
        py_channel_name.pop_back();
        if (py_channel_name.empty() || py_channel_name.back() == '.')
        {
            //
            // It matches the pattern, but are the other channels also
            // present? It's ony "RGBA" if it has all three of 'R', 'G', and
            // 'B'.
            //
            
            if (channel_list.findChannel(py_channel_name + "R") &&
                channel_list.findChannel(py_channel_name + "G") &&
                channel_list.findChannel(py_channel_name + "B"))
            {
                auto A = py_channel_name + "A";
                if (!py_channel_name.empty())
                    py_channel_name.pop_back();
                if (py_channel_name.empty())
                {
                    py_channel_name = "RGB";
                    if (channel_list.findChannel(A))
                        py_channel_name += "A";
                }

                if (channel_list.findChannel(A))
                    return 4;
                return 3;
            }
        }
        py_channel_name = name;
    }

    return 0;
}

py::object
PyFile::__enter__()
{
    return py::cast(this);
}

void
PyFile::__exit__(py::args args)
{
    for (auto p : parts)
    {
        PyPart& P = p.cast<PyPart&>();
        P.header.clear();

        for (auto c : P.channels)
        {
            auto C = py::cast<PyChannel&>(c.second);
            C.pixels = py::none();
        }
        P.channels.clear();
    }
    parts = py::list();
}

void
validate_part_index(int part_index, size_t num_parts)
{
    if (part_index < 0)
    {
        std::stringstream s;
        s << "Invalid negative part index '" << part_index << "'";
        throw std::invalid_argument(s.str());
    }
    
    if (static_cast<size_t>(part_index) >= num_parts)
    {
        std::stringstream s;
        s << "Invalid part index '" << part_index
          << "': file has " << num_parts
          << " part";
        if (num_parts != 1)
            s << "s";
        s << ".";
        throw std::invalid_argument(s.str());
    }
}
    
py::dict&
PyFile::header(int part_index)
{
    validate_part_index(part_index, parts.size());
    return parts[part_index].cast<PyPart&>().header;
}

py::dict&
PyFile::channels(int part_index)
{
    validate_part_index(part_index, parts.size());
    return parts[part_index].cast<PyPart&>().channels;
}

//
// Write the PyFile to the given filename
//

void
PyFile::write(const char* outfilename)
{
    std::vector<Header> headers;

    for (size_t part_index = 0; part_index < parts.size(); part_index++)
    {
        const PyPart& P = parts[part_index].cast<const PyPart&>();
        
        Header header;

        if (P.name().empty())
        {
            std::stringstream n;
            n << "Part" << part_index;
            header.setName (n.str());
        }
        else
            header.setName (P.name());

        //
        // Add attributes from the py::dict to the output header
        //
        
        for (auto a : P.header)
        {
            auto name = py::str(a.first);
            py::object second = py::cast<py::object>(a.second);
            insertAttribute(header, name, second);
        }
        
        //
        // Add required attributes to the header
        //
        
        header.setType(P.typeString());

        if (!P.header.contains("dataWindow"))
        {
            auto shape = P.shape();
            header.dataWindow().max = V2i(shape[1]-1,shape[0]-1);
        }

        if (!P.header.contains("displayWindow"))
        {
            auto shape = P.shape();
            header.displayWindow().max = V2i(shape[1]-1,shape[0]-1);
        }

        if (P.type() == EXR_STORAGE_TILED || P.type() == EXR_STORAGE_DEEP_TILED)
        {
            if (P.header.contains("tiles"))
            {       
                auto td = P.header["tiles"].cast<const TileDescription&>();
                header.setTileDescription (td);
            }
        }

        if (P.header.contains("lineOrder"))
        {
            auto lo = P.header["lineOrder"].cast<LineOrder&>();
            header.lineOrder() = static_cast<LineOrder>(lo);
        }

        header.compression() = P.compression();
        
        //
        // Add channels to the output header
        //
        
        for (auto c : P.channels)
        {
            auto C = py::cast<PyChannel&>(c.second);
            auto pixelType = C.pixelType();

            int nrgba;
            if (C.pixels.dtype().kind() == 'O')
                nrgba = get_deep_nrgba(C.pixels);
            else if (C.pixels.ndim() == 2)
                nrgba = 0;
            else
                nrgba = C.pixels.shape(2);

            if (nrgba > 0)
            {
                //
                // The py::dict has a single "RGB" or "RGBA" numpy array, but
                // the output file gets separate channels
                //
                
                std::string name_prefix;
                if (C.name == "RGB" || C.name == "RGBA")
                    name_prefix = "";
                else
                    name_prefix = C.name + ".";

                header.channels ().insert(name_prefix + "R", Channel (pixelType, C.xSampling, C.ySampling, C.pLinear));
                header.channels ().insert(name_prefix + "G", Channel (pixelType, C.xSampling, C.ySampling, C.pLinear));
                header.channels ().insert(name_prefix + "B", Channel (pixelType, C.xSampling, C.ySampling, C.pLinear));
                if (nrgba > 3)
                    header.channels ().insert(name_prefix + "A", Channel (pixelType, C.xSampling, C.ySampling, C.pLinear));
            }
            else
                header.channels ().insert(C.name, Channel (pixelType, C.xSampling, C.ySampling, C.pLinear));
        }


        headers.push_back (header);
    }

    MultiPartOutputFile outfile(outfilename, headers.data(), headers.size());

    //
    // Write the channel data: add slices to the framebuffer and write.
    //
    
    for (size_t part_index = 0; part_index < parts.size(); part_index++)
    {
        const PyPart& P = parts[part_index].cast<const PyPart&>();

        auto header = headers[part_index];
        const Box2i& dw = header.dataWindow();

        if (P.type() == EXR_STORAGE_SCANLINE ||
            P.type() == EXR_STORAGE_TILED)
        {
            P.writePixels(outfile, dw);
        }
        else if (P.type() == EXR_STORAGE_DEEP_SCANLINE ||
                 P.type() == EXR_STORAGE_DEEP_TILED)
        {
            P.writeDeepPixels(outfile, dw);
        }
        else
            throw std::runtime_error("invalid type");
    }

    filename = outfilename;
}

//
// Helper routine to cast an objec to a type only if it's actually that type,
// since py::cast throws an runtime_error on unexpected type.
//

template <class T>
const T*
py_cast(const py::object& object)
{
    if (py::isinstance<T>(object))
        return py::cast<T*>(object);

    return nullptr;
}

//
// Helper routine to cast an objec to a type only if it's actually that type,
// since py::cast throws an runtime_error on unexpected type. This further cast
// the resulting pointer to a second type.
//

template <class T, class S>
const T*
py_cast(const py::object& object)
{
    if (py::isinstance<S>(object))
    {
        auto o = py::cast<S*>(object);
        return reinterpret_cast<const T*>(o);
    }

    return nullptr;
}

template <>
const double*
py_cast(const py::object& object)
{
    //
    // Recognize a 1-element array of double as a DoubleAttribute
    //
    
    if (py::isinstance<py::array_t<double>>(object))
    {
        auto a = object.cast<py::array_t<double>>();
        if (a.size() == 1)
        {
            py::buffer_info buf = a.request();
            return static_cast<const double*>(buf.ptr);
        }
    }
    return nullptr;
}

template <class T>
py::array
make_v2(const Vec2<T>& v)
{
    std::vector<size_t> shape ({2});
    const auto style = py::array::c_style | py::array::forcecast;
    auto npa = py::array_t<T,style>(shape);
    auto d = static_cast<T*>(npa.request().ptr);
    d[0] = v[0];
    d[1] = v[1];
    return npa;
}

template <class T>
py::array
make_v3(const Vec3<T>& v)
{
    std::vector<size_t> shape ({3});
    const auto style = py::array::c_style | py::array::forcecast;
    auto npa = py::array_t<T,style>(shape);
    auto d = static_cast<T*>(npa.request().ptr);
    d[0] = v[0];
    d[1] = v[1];
    d[2] = v[2];
    return npa;
}

py::object
PyFile::getAttributeObject(const std::string& name, const Attribute* a)
{
    if (auto v = dynamic_cast<const Box2iAttribute*> (a))
    {
        auto min = make_v2<int>(v->value().min);
        auto max = make_v2<int>(v->value().max);
        return py::make_tuple(min, max);
    }

    if (auto v = dynamic_cast<const Box2fAttribute*> (a))
    {
        auto min = make_v2<float>(v->value().min);
        auto max = make_v2<float>(v->value().max);
        return py::make_tuple(min, max);
    }
    
    if (auto v = dynamic_cast<const ChannelListAttribute*> (a))
    {
        auto L = v->value();
        auto l = py::list();
        for (auto c = L.begin (); c != L.end (); ++c)
        {
            auto C = c.channel();
            l.append(py::cast(PyChannel(c.name(),
                                        C.xSampling,
                                        C.ySampling,
                                        C.pLinear)));
        }
        return l;
    }
    
    if (auto v = dynamic_cast<const ChromaticitiesAttribute*> (a))
    {
        auto c = v->value();
        return py::make_tuple(c.red.x, c.red.y,
                              c.green.x, c.green.y, 
                              c.blue.x, c.blue.y, 
                              c.white.x, c.white.y);
    }

    if (auto v = dynamic_cast<const CompressionAttribute*> (a))
        return py::cast(v->value());

    //
    // Convert Double attribute to a single-element numpy array, so
    // its type is preserved.
    //
    
    if (auto v = dynamic_cast<const DoubleAttribute*> (a))
        return py::array_t<double>(1, &v->value());

    if (auto v = dynamic_cast<const EnvmapAttribute*> (a))
        return py::cast(v->value());

    if (auto v = dynamic_cast<const FloatAttribute*> (a))
        return py::float_(v->value());

    if (auto v = dynamic_cast<const IntAttribute*> (a))
        return py::int_(v->value());

    if (auto v = dynamic_cast<const KeyCodeAttribute*> (a))
        return py::cast(v->value());

    if (auto v = dynamic_cast<const LineOrderAttribute*> (a))
        return py::cast(v->value());

    if (auto v = dynamic_cast<const M33fAttribute*> (a))
    {
        std::vector<size_t> shape ({3,3});
        const auto style = py::array::c_style | py::array::forcecast;
        auto npa = py::array_t<float,style>(shape);
        auto m = static_cast<float*>(npa.request().ptr);
        m[0] = v->value()[0][0];
        m[1] = v->value()[0][1];
        m[2] = v->value()[0][2];
        m[3] = v->value()[1][0];
        m[4] = v->value()[1][1];
        m[5] = v->value()[1][2];
        m[6] = v->value()[2][0];
        m[7] = v->value()[2][1];
        m[8] = v->value()[2][2];
        return npa;
    }
    
    if (auto v = dynamic_cast<const M33dAttribute*> (a))
    {
        std::vector<size_t> shape ({3,3});
        const auto style = py::array::c_style | py::array::forcecast;
        auto npa = py::array_t<double,style>(shape);
        auto m = static_cast<double*>(npa.request().ptr);
        m[0] = v->value()[0][0];
        m[1] = v->value()[0][1];
        m[2] = v->value()[0][2];
        m[3] = v->value()[1][0];
        m[4] = v->value()[1][1];
        m[5] = v->value()[1][2];
        m[6] = v->value()[2][0];
        m[7] = v->value()[2][1];
        m[8] = v->value()[2][2];
        return npa;
    }

    if (auto v = dynamic_cast<const M44fAttribute*> (a))
    {
        std::vector<size_t> shape ({4,4});
        const auto style = py::array::c_style | py::array::forcecast;
        auto npa = py::array_t<float,style>(shape);
        auto m = static_cast<float*>(npa.request().ptr);
        m[0] = v->value()[0][0];
        m[1] = v->value()[0][1];
        m[2] = v->value()[0][2];
        m[3] = v->value()[0][3];
        m[4] = v->value()[1][0];
        m[5] = v->value()[1][1];
        m[6] = v->value()[1][2];
        m[7] = v->value()[1][3];
        m[8] = v->value()[2][0];
        m[9] = v->value()[2][1];
        m[10] = v->value()[2][2];
        m[11] = v->value()[2][3];
        m[12] = v->value()[3][0];
        m[13] = v->value()[3][1];
        m[14] = v->value()[3][2];
        m[15] = v->value()[3][3];
        return npa;
    }
    
    if (auto v = dynamic_cast<const M44dAttribute*> (a))
    {
        std::vector<size_t> shape ({4,4});
        const auto style = py::array::c_style | py::array::forcecast;
        auto npa = py::array_t<double,style>(shape);
        auto m = static_cast<double*>(npa.request().ptr);
        m[0] = v->value()[0][0];
        m[1] = v->value()[0][1];
        m[2] = v->value()[0][2];
        m[3] = v->value()[0][3];
        m[4] = v->value()[1][0];
        m[5] = v->value()[1][1];
        m[6] = v->value()[1][2];
        m[7] = v->value()[1][3];
        m[8] = v->value()[2][0];
        m[9] = v->value()[2][1];
        m[10] = v->value()[2][2];
        m[11] = v->value()[2][3];
        m[12] = v->value()[3][0];
        m[13] = v->value()[3][1];
        m[14] = v->value()[3][2];
        m[15] = v->value()[3][3];
        return npa;
    }
    
    if (auto v = dynamic_cast<const PreviewImageAttribute*> (a))
    {
        auto I = v->value();
        return py::cast(PyPreviewImage(I.width(), I.height(), I.pixels()));
    }

    if (auto v = dynamic_cast<const StringAttribute*> (a))
    {
        if (name == "type")
        {
            //
            // The "type" attribute comes through as a string,
            // but we want it to be the OpenEXR.Storage enum.
            //
                  
            exr_storage_t t = EXR_STORAGE_LAST_TYPE;
            if (v->value() == SCANLINEIMAGE) // "scanlineimage")
                t = EXR_STORAGE_SCANLINE;
            else if (v->value() == TILEDIMAGE) // "tiledimage")
                t = EXR_STORAGE_TILED;
            else if (v->value() == DEEPSCANLINE) // "deepscanline")
                t = EXR_STORAGE_DEEP_SCANLINE;
            else if (v->value() == DEEPTILE) // "deeptile") 
                t = EXR_STORAGE_DEEP_TILED;
            else
                throw std::invalid_argument("unrecognized image 'type' attribute");
            return py::cast(t);
        }
        return py::str(v->value());
    }

    if (auto v = dynamic_cast<const StringVectorAttribute*> (a))
    {
        auto l = py::list();
        for (auto i = v->value().begin (); i != v->value().end(); i++)
            l.append(py::str(*i));
        return l;
    }

    if (auto v = dynamic_cast<const FloatVectorAttribute*> (a))
    {
        auto l = py::list();
        for (auto i = v->value().begin(); i != v->value().end(); i++)
            l.append(py::float_(*i));
        return l;
    }

    if (auto v = dynamic_cast<const RationalAttribute*> (a))
    {
        py::module fractions = py::module::import("fractions");
        py::object Fraction = fractions.attr("Fraction");
        return Fraction(v->value().n, v->value().d);
    }

    if (auto v = dynamic_cast<const TileDescriptionAttribute*> (a))
        return py::cast(v->value());

    if (auto v = dynamic_cast<const TimeCodeAttribute*> (a))
        return py::cast(v->value());

    if (auto v = dynamic_cast<const V2iAttribute*> (a))
        return make_v2(v->value());

    if (auto v = dynamic_cast<const V2fAttribute*> (a))
        return make_v2(v->value());

    if (auto v = dynamic_cast<const V2dAttribute*> (a))
        return make_v2(v->value());

    if (auto v = dynamic_cast<const V3iAttribute*> (a))
        return make_v3(v->value());
    
    if (auto v = dynamic_cast<const V3fAttribute*> (a))
        return make_v3(v->value());
    
    if (auto v = dynamic_cast<const V3dAttribute*> (a))
        return make_v3(v->value());
    
    std::stringstream err;
    err << "unsupported attribute type: " << a->typeName();
    throw std::runtime_error(err.str());
    
    return py::none();
}
    
template <class P, class T>
bool
objectToV2(const py::object& object, Vec2<T>& v)
{
    if (py::isinstance<py::tuple>(object))
    {
        auto tup = object.cast<py::tuple>();
        if (tup.size() == 2 &&
            py::isinstance<P>(tup[0]) &&
            py::isinstance<P>(tup[1]))
        {       
            v.x = P(tup[0]);
            v.y = P(tup[1]);
            return true;
        }
    }
    else if (py::isinstance<py::array_t<T>>(object))
    {
        auto a = object.cast<py::array_t<T>>();
        if (a.ndim() == 1 && a.size() == 2)
        {
            auto p = static_cast<T*>(a.request().ptr);
            v.x = p[0];
            v.y = p[1];
            return true;
        }
    }

    return false;
}

bool
objectToV2i(const py::object& object, V2i& v)
{
    return objectToV2<py::int_, int>(object, v);
}

bool
objectToV2f(const py::object& object, V2f& v)
{
    return objectToV2<py::float_, float>(object, v);
}

bool
objectToV2d(const py::object& object, V2d& v)
{
    if (py::isinstance<py::array_t<double>>(object))
    {
        auto a = object.cast<py::array_t<double>>();
        if (a.ndim() == 1 && a.size() == 2)
        {
            auto p = static_cast<double*>(a.request().ptr);
            v.x = p[0];
            v.y = p[1];
            return true;
        }
    }
    return false;
}

template <class P, class T>
bool
objectToV3(const py::object& object, Vec3<T>& v)
{
    if (py::isinstance<py::tuple>(object))
    {
        auto tup = object.cast<py::tuple>();
        if (tup.size() == 3 &&
            py::isinstance<P>(tup[0]) &&
            py::isinstance<P>(tup[1]) &&
            py::isinstance<P>(tup[2]))
        {       
            v.x = P(tup[0]);
            v.y = P(tup[1]);
            v.z = P(tup[2]);
            return true;
        }
    }
    else if (py::isinstance<py::array_t<T>>(object))
    {
        auto a = object.cast<py::array_t<T>>();
        if (a.ndim() == 1 && a.size() == 3)
        {
            auto p = static_cast<T*>(a.request().ptr);
            v.x = p[0];
            v.y = p[1];
            v.z = p[2];
            return true;
        }
    }

    return false;
}

bool
objectToV3i(const py::object& object, V3i& v)
{
    return objectToV3<py::int_, int>(object, v);
}

bool
objectToV3f(const py::object& object, V3f& v)
{
    return objectToV3<py::float_, float>(object, v);
}

bool
objectToV3d(const py::object& object, V3d& v)
{
    if (py::isinstance<py::array_t<double>>(object))
    {
        auto a = object.cast<py::array_t<double>>();
        if (a.ndim() == 1 && a.size() == 3)
        {
            auto p = static_cast<double*>(a.request().ptr);
            v.x = p[0];
            v.y = p[1];
            v.z = p[2];
            return true;
        }
    }
    return false;
}

template <class T>
bool
objectToM33(const py::object& object, Matrix33<T>& m)
{
    if (py::isinstance<py::array_t<T>>(object))
    {
        auto a = object.cast<py::array_t<T>>();
        if (a.ndim() == 2 && a.shape(0) == 3 && a.shape(1) == 3)
        {
            py::buffer_info buf = a.request();
            auto v = static_cast<const T*>(buf.ptr);
            m = Matrix33<T>(v[0], v[1], v[2],
                            v[3], v[4], v[5],
                            v[6], v[7], v[8]);
            return true;
        }
    }
    return false;
}

template <class T>
bool
objectToM44(const py::object& object, Matrix44<T>& m)
{
    if (py::isinstance<py::array_t<T>>(object))
    {
        auto a = object.cast<py::array_t<T>>();
        if (a.ndim() == 2 && a.shape(0) == 4 && a.shape(1) == 4)
        {
            py::buffer_info buf = a.request();
            auto v = static_cast<const T*>(buf.ptr);
            m = Matrix44<T>(v[0], v[1], v[2], v[3],
                            v[4], v[5], v[6], v[7],
                            v[8], v[9], v[10], v[11],
                            v[12], v[13], v[14], v[15]);
            return true;
        }
    }
    return false;
}

    
bool
objectToBox2i(const py::object& object, Box2i& b)
{
    if (py::isinstance<py::tuple>(object))
    {
        auto tup = object.cast<py::tuple>();
        if (tup.size() == 2)
            if (objectToV2i(tup[0], b.min) && objectToV2i(tup[1], b.max))
                return true;
    }

    return false;
}
         
bool
objectToBox2f(const py::object& object, Box2f& b)
{
    if (py::isinstance<py::tuple>(object))
    {
        auto tup = object.cast<py::tuple>();
        if (tup.size() == 2)
            if (objectToV2f(tup[0], b.min) && objectToV2f(tup[1], b.max))
                return true;
    }

    return false;
}
         
bool
objectToChromaticities(const py::object& object, Chromaticities& v)
{
    if (py::isinstance<py::tuple>(object))
    {
        auto tup = object.cast<py::tuple>();
        if (tup.size() == 8 && 
            py::isinstance<py::float_>(tup[0]) &&
            py::isinstance<py::float_>(tup[1]) &&
            py::isinstance<py::float_>(tup[2]) &&
            py::isinstance<py::float_>(tup[3]) &&
            py::isinstance<py::float_>(tup[4]) &&
            py::isinstance<py::float_>(tup[5]) &&
            py::isinstance<py::float_>(tup[6]) &&
            py::isinstance<py::float_>(tup[7]))
        {       
            v.red.x = py::float_(tup[0]);
            v.red.y = py::float_(tup[1]);
            v.green.x = py::float_(tup[2]);
            v.green.y = py::float_(tup[3]);
            v.blue.x = py::float_(tup[4]);
            v.blue.y = py::float_(tup[5]);
            v.white.x = py::float_(tup[6]);
            v.white.y = py::float_(tup[7]);
            return true;
        }
    }    
    return false;
}

void
PyFile::insertAttribute(Header& header, const std::string& name, const py::object& object)
{
    std::stringstream err;
    
    //
    // If the attribute is standard/required, its type is fixed, so
    // cast the rhs to the appropriate type if possible, or reject it
    // as an error if not.
    //
        
    if (name == "dataWindow" ||
        name == "displayWindow" ||
        name == "originalDataWindow" ||
        name == "sensorAcquisitionRectangle")
    {
        Box2i b;
        if (objectToBox2i(object, b))
        {
            header.insert(name, Box2iAttribute(b));
            return;
        }
        err << "invalid value for attribute '" << name << "': expected a box2i tuple, got " << py::str(object);
        throw std::invalid_argument(err.str());
    }

    // Required to be V2f?
        
    if (name == "screenWindowCenter" ||
        name == "sensorCenterOffset" ||
        name == "sensorOverallDimensions" ||
        name == "cameraColorBalance" ||
        name == "adoptedNeutral")
    {
        V2f v;
        if (objectToV2f(object, v))
        {
            header.insert(name, V2fAttribute(v));
            return;
        }
        err << "invalid value for attribute '" << name << "': expected a v2f, got " << py::str(object);
        throw std::invalid_argument(err.str());
    }

    // Required to be chromaticities?

    if (name == "chromaticities")
    {
        Chromaticities c;
        if (objectToChromaticities(object, c))
        {
            header.insert(name, ChromaticitiesAttribute(c));
            return;
        }
        err << "invalid value for attribute '" << name << "': expected a 6-tuple, got " << py::str(object);
        throw std::invalid_argument(err.str());
    }
    
    //
    // Recognize tuples and arrays as V2/V3 i/f/d or M33/M44 f/d
    //
    
    V2i v2i;
    if (objectToV2i(object, v2i))
    {       
        header.insert(name, V2iAttribute(v2i));
        return;
    }

    V2f v2f;
    if (objectToV2f(object, v2f))
    {       
        header.insert(name, V2fAttribute(v2f));
        return;
    }

    V2d v2d;
    if (objectToV2d(object, v2d))
    {       
        header.insert(name, V2dAttribute(v2d));
        return;
    }

    V3i v3i;
    if (objectToV3i(object, v3i))
    {       
        header.insert(name, V3iAttribute(v3i));
        return;
    }

    V3f v3f;
    if (objectToV3f(object, v3f))
    {       
        header.insert(name, V3fAttribute(v3f));
        return;
    }

    V3d v3d;
    if (objectToV3d(object, v3d))
    {       
        header.insert(name, V3dAttribute(v3d));
        return;
    }

    M33f m33f;
    if (objectToM33(object, m33f))
    {       
        header.insert(name, M33fAttribute(m33f));
        return;
    }

    M33d m33d;
    if (objectToM33(object, m33d))
    {       
        header.insert(name, M33dAttribute(m33d));
        return;
    }

    M44f m44f;
    if (objectToM44(object, m44f))
    {       
        header.insert(name, M44fAttribute(m44f));
        return;
    }

    M44d m44d;
    if (objectToM44(object, m44d))
    {       
        header.insert(name, M44dAttribute(m44d));
        return;
    }

    //
    // Recognize 2-tuples of 2-vectors as boxes
    //
    
    Box2i box2i;
    if (objectToBox2i(object, box2i))
    {       
        header.insert(name, Box2iAttribute(box2i));
        return;
    }

    Box2f box2f;
    if (objectToBox2f(object, box2f))
    {       
        header.insert(name, Box2fAttribute(box2f));
        return;
    }

    //
    // Recognize an 8-tuple as chromaticities
    //
    
    Chromaticities c;
    if (objectToChromaticities(object, c))
    {
        header.insert(name, ChromaticitiesAttribute(c));
        return;
    }

    //
    // Inspect the rhs type
    //
    
    py::module fractions = py::module::import("fractions");
    py::object Fraction = fractions.attr("Fraction");

    if (py::isinstance<py::list>(object))
    {
        auto list = py::cast<py::list>(object);
        auto size = list.size();
        if (size == 0)
            throw std::runtime_error("invalid empty list is header: can't deduce attribute type");

        if (py::isinstance<py::float_>(list[0]))
        {
            // float vector
            std::vector<float> v = list.cast<std::vector<float>>();
            header.insert(name, FloatVectorAttribute(v));
        }
        else if (py::isinstance<py::str>(list[0]))
        {
            // string vector
            std::vector<std::string> v = list.cast<std::vector<std::string>>();
            header.insert(name, StringVectorAttribute(v));
        }
        else if (py::isinstance<PyChannel>(list[0]))
        {
            //
            // Channel list: don't create an explicit chlist attribute here,
            // since the channels get created elswhere.
        }
    }
    else if (auto v = py_cast<Compression>(object))
        header.insert(name, CompressionAttribute(static_cast<Compression>(*v)));
    else if (auto v = py_cast<Envmap>(object))
        header.insert(name, EnvmapAttribute(static_cast<Envmap>(*v)));
    else if (py::isinstance<py::int_>(object))
        header.insert(name, IntAttribute(py::cast<py::int_>(object)));
    else if (py::isinstance<py::float_>(object))
        header.insert(name, FloatAttribute(py::cast<py::float_>(object)));
    else if (auto v = py_cast<double>(object))
        header.insert(name, DoubleAttribute(*v));
    else if (auto v = py_cast<KeyCode>(object))
        header.insert(name, KeyCodeAttribute(*v));
    else if (auto v = py_cast<LineOrder>(object))
        header.insert(name, LineOrderAttribute(static_cast<LineOrder>(*v)));
    else if (auto v = py_cast<PyPreviewImage>(object))
    {
        py::buffer_info buf = v->pixels.request();
        auto pixels = static_cast<PreviewRgba*>(buf.ptr);
        auto height = v->pixels.shape(0);
        auto width = v->pixels.shape(1);
        PreviewImage p(width, height, pixels);
        header.insert(name, PreviewImageAttribute(p));
    }
    else if (auto v = py_cast<TileDescription>(object))
        header.insert(name, TileDescriptionAttribute(*v));
    else if (auto v = py_cast<TimeCode>(object))
        header.insert(name, TimeCodeAttribute(*v));
    else if (auto v = py_cast<exr_storage_t>(object))
    {
        std::string type;
        switch (*v)
        {
        case EXR_STORAGE_SCANLINE:
            type = SCANLINEIMAGE;
            break;
        case EXR_STORAGE_TILED:
            type = TILEDIMAGE;
            break;
        case EXR_STORAGE_DEEP_SCANLINE:
            type = DEEPSCANLINE;
            break;
        case EXR_STORAGE_DEEP_TILED:
            type = DEEPTILE;
            break;
        case EXR_STORAGE_LAST_TYPE:
        default:
            throw std::runtime_error("unknown storage type");
            break;
        }
        header.setType(type);
    }
    else if (py::isinstance<py::str>(object))
        header.insert(name, StringAttribute(py::str(object)));
    else if (py::isinstance(object, Fraction))
    {
        int n = py::int_(object.attr("numerator"));
        int d = py::int_(object.attr("denominator"));
        Rational r(n, d);
        header.insert(name, RationalAttribute(r));
    }
    else
    {
        auto t = py::str(object.attr("__class__").attr("__name__"));
        err << "unrecognized type of attribute '" << name << "': type=" << t << " value=" << py::str(object);
        if (py::isinstance<py::array>(object))
        {
            auto a = object.cast<py::array>();
            err << " dtype=" << py::str(a.dtype());
        }
        throw std::runtime_error(err.str());
    }
}

//
// Construct a part from explicit header and channel data.
// 
// Used to construct a file for writing.
//

PyPart::PyPart(const py::dict& header, const py::dict& channels, const std::string& name)
    : header(header), channels(channels), part_index(0)
{
    if (name != "")
        header[py::str("name")] = py::str(name);
    
    for (auto a : header)
    {
        if (!py::isinstance<py::str>(a.first))
            throw std::invalid_argument("header key must be string (attribute name)");
        
        // TODO: confirm it's a valid attribute value
        py::object second = py::cast<py::object>(a.second);
    }
    
    //
    // Validate that all channel dict keys are strings, and initialize the
    // channel name field.
    //
    
    for (auto c : channels)
    {
        if (!py::isinstance<py::str>(c.first))
            throw std::invalid_argument("channels key must be string (channel name)");

        //
        // Accept a py::array as the py::dict value, but replace it with a PyChannel object.
        //
        
        if (py::isinstance<py::array>(c.second))
        {
            std::string channel_name = py::str(c.first);
            py::array a = c.second.cast<py::array>();
            channels[channel_name.c_str()] = PyChannel(channel_name.c_str(), a);
        }
        else if (py::isinstance<PyChannel>(c.second))
        {
            c.second.cast<PyChannel&>().name = py::str(c.first);
        }
        else
            throw std::invalid_argument("Channel value must be a Channel() object or a numpy pixel array");
    }

    auto s = shape();

    if (!header.contains("dataWindow"))
    {
        auto min = make_v2<int>(V2i(0, 0));
        auto max = make_v2<int>(V2i(s[1]-1,s[0]-1));
        header["dataWindow"] =  py::make_tuple(min, max);
    }

    if (!header.contains("displayWindow"))
    {
        auto min = make_v2<int>(V2i(0, 0));
        auto max = make_v2<int>(V2i(s[1]-1,s[0]-1));
        header["displayWindow"] = py::make_tuple(min, max);
    }
}

void
PyChannel::validatePixelArray()
{
    if (pixels.ndim() < 2 ||  pixels.ndim() > 3)
        throw std::invalid_argument("invalid pixel array: must be 2D or 3D numpy array");

    if (pixels.dtype().kind() == 'O')
    {
        auto height = pixels.shape(0);
        auto width = pixels.shape(1);

        auto pixel_objects = static_cast<py::object*>(pixels.mutable_data());

        for (decltype(height) y=0; y<height; y++)
            for (decltype(width) x=0; x<width; x++)
            {
                auto i = y * width + x;
                if (!(py::isinstance<py::array_t<uint32_t>>(pixel_objects[i]) || 
                      py::isinstance<py::array_t<half>>(pixel_objects[i]) ||
                      py::isinstance<py::array_t<float>>(pixel_objects[i]) ||
                      pixel_objects[i].is(py::none())))
                {
                    std::stringstream err;
                    if (py::isinstance<py::array>(pixel_objects[i]))
                        err << "invalid deep pixel array: entry at " << y << "," << x << " is array of unsupported type '" << py::str(pixel_objects[i].cast<py::array>().dtype()) << "'";
                    else
                        err << "invalid deep pixel array: entry at " << y << "," << x << " is of unsupported type '" << py::str(pixel_objects[i].attr("__class__").attr("__name__")) << "'";
                    throw std::invalid_argument(err.str());
                }
            }
    }
    else
    {
        if (!(py::isinstance<py::array_t<uint32_t>>(pixels) ||
              py::isinstance<py::array_t<half>>(pixels) ||
              py::isinstance<py::array_t<float>>(pixels)))
        {
            std::stringstream err;
            err << "invalid pixel array: unsupported type " << py::str(pixels.attr("__class__").attr("__name__"));
            throw std::invalid_argument(err.str());
        }
    }
}
        
V2i
PyPart::shape() const
{
    V2i S(0, 0);
        
    std::string channel_name; // first channel name

    for (auto c : channels)
    {
        auto C = py::cast<PyChannel&>(c.second);

        if (C.pixels.ndim() < 2 ||  C.pixels.ndim() > 3)
            throw std::invalid_argument("error: channel must have a 2D or 3D array");

        V2i c_S(C.pixels.shape(0), C.pixels.shape(1));
            
        if (S == V2i(0, 0))
        {
            S = c_S;
            channel_name = C.name;
        }
        
        if (S != c_S)
        {
            std::stringstream s;
            s << "channel shapes differ: " << channel_name
              << "=" << S
              << ", " << C.name
              << "=" << c_S;
            throw std::invalid_argument(s.str());
        }
    }                

    return S;
}

size_t
PyPart::width() const
{
    return shape()[1];
}

size_t
PyPart::height() const
{
    return shape()[0];
}

std::string
PyPart::name() const
{
    if (header.contains("name"))
        return py::str(header["name"]);
    return "";
}

Compression
PyPart::compression() const
{
    if (header.contains("compression"))
        return header["compression"].cast<Compression>();
    return ZIP_COMPRESSION;
}

exr_storage_t
PyPart::type() const
{
    if (header.contains("type"))
        return header[py::str("type")].cast<exr_storage_t>();
    return EXR_STORAGE_SCANLINE;
}

std::string
PyPart::typeString() const
{
    switch (type())
    {
      case EXR_STORAGE_SCANLINE:
          return SCANLINEIMAGE;
      case EXR_STORAGE_TILED:
          return TILEDIMAGE;
      case EXR_STORAGE_DEEP_SCANLINE:
          return DEEPSCANLINE;
      case EXR_STORAGE_DEEP_TILED:
          return DEEPTILE;
      default:
          throw std::runtime_error("invalid type");
    }       
    return SCANLINEIMAGE;
}

PixelType
PyChannel::pixelType() const
{
    auto buf = py::array::ensure(pixels);
    if (buf)
    {
        if (py::isinstance<py::array_t<uint32_t>>(buf))
            return UINT;
        if (py::isinstance<py::array_t<half>>(buf))
            return HALF;      
        if (py::isinstance<py::array_t<float>>(buf))
            return FLOAT;

        if (py::isinstance<py::dtype>(pixels.dtype())) 
        {
            auto height = pixels.shape(0);
            auto width = pixels.shape(1);

            auto object_array = pixels.unchecked<py::object,2>();

            for (decltype(height) y=0; y<height; y++)
                for (decltype(width) x=0; x<width; x++)
                    if (auto object = object_array.data(y,x))
                    {
                        if (py::isinstance<py::array_t<uint32_t>>(*object))
                            return UINT;
                        if (py::isinstance<py::array_t<half>>(*object))
                            return HALF;      
                        if (py::isinstance<py::array_t<float>>(*object))
                            return FLOAT;
                    }
        }
    }

    return NUM_PIXELTYPES;
}

template <class T>
std::string
repr(const T& v)
{
    std::stringstream s;
    s << v;
    return s.str();
}

} // namespace


PYBIND11_MODULE(OpenEXR, m)
{
    using namespace py::literals;

    m.doc() = "Read and write EXR high-dynamic range image files";
    
    m.attr("__version__") = OPENEXR_VERSION_STRING;
    m.attr("OPENEXR_VERSION") = OPENEXR_VERSION_STRING;

    //
    // Add symbols from the legacy implementation of the bindings for
    // backwards compatibility
    //
    
    init_OpenEXR_old(m.ptr());

    //
    // Enums
    //
    
    py::enum_<LevelRoundingMode>(m, "LevelRoundingMode", "Rounding mode for tiled images")
        .value("ROUND_UP", ROUND_UP)
        .value("ROUND_DOWN", ROUND_DOWN)
        .value("NUM_ROUNDING_MODES", NUM_ROUNDINGMODES)
        .export_values();

    py::enum_<LevelMode>(m, "LevelMode", "Level mode for tiled images")
        .value("ONE_LEVEL", ONE_LEVEL)
        .value("MIPMAP_LEVELS", MIPMAP_LEVELS)
        .value("RIPMAP_LEVELS", RIPMAP_LEVELS)
        .value("NUM_LEVEL_MODES", NUM_LEVELMODES)
        .export_values();

    py::enum_<LineOrder>(m, "LineOrder", "Line order for scanline images")
        .value("INCREASING_Y", INCREASING_Y)
        .value("DECREASING_Y", DECREASING_Y)
        .value("RANDOM_Y", RANDOM_Y)
        .value("NUM_LINE_ORDERS", NUM_LINEORDERS)
        .export_values();

    py::enum_<PixelType>(m, "PixelType", "Data type for pixel arrays")
        .value("UINT", UINT, "32-bit integer")
        .value("HALF", HALF)
        .value("FLOAT", FLOAT)
        .value("NUM_PIXELTYPES", NUM_PIXELTYPES)
        .export_values();

    py::enum_<Compression>(m, "Compression", "Compression method")
        .value("NO_COMPRESSION", NO_COMPRESSION)
        .value("RLE_COMPRESSION", RLE_COMPRESSION)
        .value("ZIPS_COMPRESSION", ZIPS_COMPRESSION)
        .value("ZIP_COMPRESSION", ZIP_COMPRESSION)
        .value("PIZ_COMPRESSION", PIZ_COMPRESSION)
        .value("PXR24_COMPRESSION", PXR24_COMPRESSION)
        .value("B44_COMPRESSION", B44_COMPRESSION)
        .value("B44A_COMPRESSION", B44A_COMPRESSION)
        .value("DWAA_COMPRESSION", DWAA_COMPRESSION)
        .value("DWAB_COMPRESSION", DWAB_COMPRESSION)
        .value("NUM_COMPRESSION_METHODS", NUM_COMPRESSION_METHODS)
        .export_values();
    
    py::enum_<Envmap>(m, "Envmap", "Environment map type")
        .value("ENVMAP_LATLONG", ENVMAP_LATLONG)
        .value("ENVMAP_CUBE", ENVMAP_CUBE)    
        .value("NUM_ENVMAPTYPES", NUM_ENVMAPTYPES)
        .export_values();

    py::enum_<exr_storage_t>(m, "Storage", "Image storage format")
        .value("scanlineimage", EXR_STORAGE_SCANLINE)
        .value("tiledimage", EXR_STORAGE_TILED)
        .value("deepscanline", EXR_STORAGE_DEEP_SCANLINE)
        .value("deeptile", EXR_STORAGE_DEEP_TILED)
        .value("NUM_STORAGE_TYPES", EXR_STORAGE_LAST_TYPE)
        .export_values();

    //
    // Classes for attribute types
    //
    
    py::class_<TileDescription>(m, "TileDescription", "Tile description for tiled images")
        .def(py::init())
        .def("__repr__", [](TileDescription& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("xSize", &TileDescription::xSize)
        .def_readwrite("ySize", &TileDescription::ySize)
        .def_readwrite("mode", &TileDescription::mode)
        .def_readwrite("roundingMode", &TileDescription::roundingMode)
        ;       

    py::class_<Rational>(m, "Rational", "A number expressed as a ratio, n/d")
        .def(py::init())
        .def(py::init<int,unsigned int>())
        .def("__repr__", [](const Rational& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("n", &Rational::n)
        .def_readwrite("d", &Rational::d)
        ;
    
    py::class_<KeyCode>(m, "KeyCode", "Motion picture film characteristics")
        .def(py::init())
        .def(py::init<int,int,int,int,int,int,int>())
        .def(py::self == py::self)
        .def("__repr__", [](const KeyCode& v) { return repr(v); })
        .def_property("filmMfcCode", &KeyCode::filmMfcCode, &KeyCode::setFilmMfcCode)
        .def_property("filmType", &KeyCode::filmType, &KeyCode::setFilmType)
        .def_property("prefix", &KeyCode::prefix, &KeyCode::setPrefix)
        .def_property("count", &KeyCode::count, &KeyCode::setCount)
        .def_property("perfOffset", &KeyCode::perfOffset, &KeyCode::setPerfOffset)
        .def_property("perfsPerFrame", &KeyCode::perfsPerFrame, &KeyCode::setPerfsPerFrame) 
        .def_property("perfsPerCount", &KeyCode::perfsPerCount, &KeyCode::setPerfsPerCount)
        ; 

    py::class_<TimeCode>(m, "TimeCode", "Time and control code")
        .def(py::init())
        .def(py::init<int,int,int,int,int,int,int,int,int,int,int,int,int,int,int,int,int,int>())
        .def("__repr__", [](const TimeCode& v) { return repr(v); })
        .def(py::self == py::self)
        .def_property("hours", &TimeCode::hours, &TimeCode::setHours)
        .def_property("minutes", &TimeCode::minutes, &TimeCode::setMinutes)
        .def_property("seconds", &TimeCode::seconds, &TimeCode::setSeconds)
        .def_property("frame", &TimeCode::frame, &TimeCode::setFrame)
        .def_property("dropFrame", &TimeCode::dropFrame, &TimeCode::setDropFrame)
        .def_property("colorFrame", &TimeCode::colorFrame, &TimeCode::setColorFrame)
        .def_property("fieldPhase", &TimeCode::fieldPhase, &TimeCode::setFieldPhase)
        .def_property("bgf0", &TimeCode::bgf0, &TimeCode::setBgf0)
        .def_property("bgf1", &TimeCode::bgf1, &TimeCode::setBgf1)
        .def_property("bgf2", &TimeCode::bgf2, &TimeCode::setBgf2)
        .def_property("binaryGroup", &TimeCode::binaryGroup, &TimeCode::setBinaryGroup)
        .def_property("userData", &TimeCode::userData, &TimeCode::setUserData)
        .def("timeAndFlags", &TimeCode::timeAndFlags)
        .def("setTimeAndFlags", &TimeCode::setTimeAndFlags)
        ;

    py::class_<PreviewRgba>(m, "PreviewRgba", "Pixel type for the preview image")
        .def(py::init())
        .def(py::init<unsigned char,unsigned char,unsigned char,unsigned char>())
        .def(py::self == py::self)
        .def_readwrite("r", &PreviewRgba::r)
        .def_readwrite("g", &PreviewRgba::g)
        .def_readwrite("b", &PreviewRgba::b)
        .def_readwrite("a", &PreviewRgba::a)
        ;
    
    PYBIND11_NUMPY_DTYPE(PreviewRgba, r, g, b, a);
    
    py::class_<PyPreviewImage>(m, "PreviewImage", "Thumbnail version of the image")
        .def(py::init())
        .def(py::init<int,int>())
        .def(py::init<py::array_t<PreviewRgba>>())
        .def("__repr__", [](const PyPreviewImage& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("pixels", &PyPreviewImage::pixels)
        ;
    
    //
    // The File API: Channel, Part, and File
    //
    
    py::class_<PyChannel>(m, "Channel", R"pbdoc(
         The class object representing a channel in an EXR image file.
         Example
         -------  
         >>> import OpenEXR
         >>> f = OpenEXR.File("image.exr")
         >>> f.channels()['A']
         Channel("A", xSampling=1, ySampling=1)
    )pbdoc")
        .def(py::init(),
             R"pbdoc(
             Construct an empty Channel object.
             )pbdoc")
        .def(py::init<int,int,bool>(),
             py::arg("xSampling"),
             py::arg("ySampling"),
             py::arg("pLinear")=false,
             R"pbdoc(
             Construct Channel object with the given parameters.

             Parameters:
                 int : xSampling
                     The x subsampling value
                 int : ySampling
                     The y subsampling value
                 int : pLinear
                     The pLinear value
             )pbdoc")
        .def(py::init<py::array>(),
             py::arg("pixels"),
             R"pbdoc(
             Construct Channel object with the given pixel array

             Parameters:
                 np.array : pixels
                     The numpy array of pixels. Supported types are uin32, float16, float32
             )pbdoc")
        .def(py::init<py::array,int,int,bool>(),
             py::arg("pixels"),
             py::arg("xSampling"),
             py::arg("ySampling"),
             py::arg("pLinear")=false,
             R"pbdoc(
             Construct Channel object with the given parameters.

             Parameters:
             -----------
             np.array : pixels
                 The numpy array of pixels. Supported types are uin32, float16, float32
             int : xSampling
                 The x subsampling value
             int : ySampling
                 The y subsampling value
             int : pLinear
                 The pLinear value
             )pbdoc")
        .def(py::init<const char*>(),
             py::arg("name"),
             R"pbdoc(
             Construct Channel object with the given name.

             Parameters:
             -----------
             str : name
                 The name of the channel.
             )pbdoc")
        .def(py::init<const char*,int,int,bool>(),
             py::arg("name"),
             py::arg("xSampling"),
             py::arg("ySampling"),
             py::arg("pLinear")=false,
             R"pbdoc(
             Construct Channel object with the given parameters.

             Parameters:
             -----------
             str : name
                 The name of the channel.
             int : xSampling
                 The x subsampling value
             int : ySampling
                 The y subsampling value
             int : pLinear
                 The pLinear value
             )pbdoc")
        .def(py::init<const char*,py::array>(),
             py::arg("name"),
             py::arg("pixels"),
             R"pbdoc(
             Construct Channel object with the given parameters.

             Parameters:
             -----------
             str : name
                 The name of the channel.
             np.array : pixels
                 The numpy array of pixels. Supported types are uin32, float16, float32
             )pbdoc")
        .def(py::init<const char*,py::array,int,int,bool>(),
             py::arg("name"),
             py::arg("pixels"),
             py::arg("xSampling"),
             py::arg("ySampling"),
             py::arg("pLinear")=false,
             R"pbdoc(
             Construct Channel object with the given parameters.

             Parameters:
             -----------
             str : name
                 The name of the channel.
             np.array : pixels
                 The numpy array of pixels. Supported types are uin32, float16, float32
             int : xSampling
                 The x subsampling value
             int : ySampling
                 The y subsampling value
             int : pLinear
                 The pLinear value
             )pbdoc")
        .def("__repr__", [](const PyChannel& c) { return repr(c); })
        .def_readwrite("name", &PyChannel::name,
             R"pbdoc(
              str : The channel name.
             )pbdoc")
        .def("type", &PyChannel::pixelType,
             R"pbdoc(
              OpenEXR.PixelType : The pixel type (UINT, HALF, FLOAT)
             )pbdoc")
        .def_readwrite("xSampling", &PyChannel::xSampling,
             R"pbdoc(
             int : The x subsampling value
             )pbdoc")
        .def_readwrite("ySampling", &PyChannel::ySampling,
             R"pbdoc(
             int : The y subsampling value
             )pbdoc")
        .def_readwrite("pLinear", &PyChannel::pLinear,
             R"pbdoc(
             bool : The pLinear value, used for DWA compression.
             )pbdoc")
        .def_readwrite("pixels", &PyChannel::pixels,
             R"pbdoc(
             np.array : The channel pixel array.
             )pbdoc")
        .def_readonly("channel_index", &PyChannel::channel_index,
             R"pbdoc(
             int : The index of the channel.
             )pbdoc")
        ;
    
    py::class_<PyPart>(m, "Part", R"pbdoc(
         The class object representing a part in a EXR image file.

         Example
         -------  
         >>> import OpenEXR
         >>> Z = np.zeros((200,100), dtype='f')
         >>> P = OpenEXR.Part({}, {"Z" : Z })
         >>> f = OpenEXR.File([P])
         >>> f.parts()
         [Part("Part0", Compression.ZIPS_COMPRESSION, width=100, height=200)] 
    )pbdoc")
        .def(py::init(),
             R"pbdoc(
             Create an empty Part object
             )pbdoc")
        .def(py::init<py::dict,py::dict,std::string>(),
             py::arg("header"),
             py::arg("channels"),
             py::arg("name")="",
             R"pbdoc(
             Create a Part object from dicts for the header and channels.

             Parameters
             ----------
             header : dict
                 Dict of header metadata, with attribute name as key.
             channels : list
                 List of `Channel` objects, which hold pixel numpy arrays.
             name : str
                 The name of the part

             Example
             -------
             >>> Z = np.zeros((200,100), dtype='f')
             >>> P = OpenEXR.Part({}, {"Z" : Z }, "left")
             )pbdoc")
        .def("__repr__", [](const PyPart& p) { return repr(p); })
        .def("name", &PyPart::name,
             R"pbdoc(
              str : The part name.
             )pbdoc")
        .def("type", &PyPart::type,
             R"pbdoc(
              OpenEXR.Storage : The type of the part: scanlineimage, tiledimage, deepscanline, deeptile
             )pbdoc")
        .def("width", &PyPart::width,
             R"pbdoc(
             int : The width of the image, in pixels.
             )pbdoc")
        .def("height", &PyPart::height,
             R"pbdoc(
             int : The height of the image, in pixels.
             )pbdoc")
        .def("compression", &PyPart::compression,
             R"pbdoc(
             OpenEXR.Compression : The compression method:
                 NO_COMPRESSION
                 RLE_COMPRESSION
                 ZIPS_COMPRESSION
                 ZIP_COMPRESSION
                 PIZ_COMPRESSION
                 PXR24_COMPRESSION
                 B44_COMPRESSION
                 B44A_COMPRESSION
                 DWAA_COMPRESSION
                 DWAB_COMPRESSION
             )pbdoc")
        .def_readwrite("header", &PyPart::header,
             R"pbdoc(
             dict : The header metadata.
             )pbdoc")
        .def_readwrite("channels", &PyPart::channels,
             R"pbdoc(
             dict : The channels.
             )pbdoc")
        .def_readonly("part_index", &PyPart::part_index,
             R"pbdoc(
             int : The index of the part.
             )pbdoc")
        ;

    py::class_<PyFile>(m, "File", R"pbdoc(
         The class object representing an EXR image file.

         This class is the interface for reading and writing image
         header and pixel data.

         Example
         -------  
         >>> import OpenEXR
         >>> f = OpenEXR.File("image.exr")
         >>> f.header()["comment"] = "Hello, image."
         >>> f.write("out.exr")
    )pbdoc")
        .def(py::init<>())
        .def(py::init<std::string,bool,bool>(),
             py::arg("filename"),
             py::arg("separate_channels")=false,
             py::arg("header_only")=false,
             R"pbdoc(
             Initialize a File by reading the image from the given filename.

             Parameters
             ----------
             filename : str
                 The path to the image file on disk.
             separate_channels : bool
                 If True, read each channel into a separate 2D numpy array
                 if False (default), read pixel data into a single "RGB" or "RGBA" numpy array of dimension (height,width,3) or (height,width,4);
             header_only : bool
                 If True, read only the header metadata, not the image pixel data.

             Example
             -------  
             >>> f = OpenEXR.File("image.exr", separate_channels=False, header_only=False)
             )pbdoc")
        .def(py::init<py::dict,py::dict>(),
             py::arg("header"),
             py::arg("channels"),
             R"pbdoc(
             Initialize a File with metadata and pixels. Creates a single-part EXR file.

             Parameters
             ----------
             header : dict
                 Dict of header metadata, with attribute name as key.
             channels : list
                 List of `Channel` objects, which hold pixel numpy arrays.

             Example
             -------
             >>> height, width = (20, 10)
             >>> R = np.random.rand(height, width).astype('f')
             >>> G = np.random.rand(height, width).astype('f')
             >>> B = np.random.rand(height, width).astype('f')
             >>> channels = { "R" : R, "G" : G, "B" : B }
             >>> header = { "compression" : OpenEXR.ZIP_COMPRESSION,
                            "type" : OpenEXR.scanlineimage }
             >>> f = OpenEXR.File(header, channels)
        )pbdoc")
        .def(py::init<py::list>(),
             py::arg("parts"),
             R"pbdoc(
             Initialize a File with a list of Part objects.

             Parameters
             ----------
             parts : list
                 List of Part objects

             Example
             -------
             >>> height, width = (20, 10)
             >>> Z0 = np.zeros((height, width), dtype='f')
             >>> Z1 = np.ones((height, width), dtype='f')
             >>> P0 = OpenEXR.Part({}, {"Z" : Z0 })
             >>> P1 = OpenEXR.Part({}, {"Z" : Z1 })
             >>> f = OpenEXR.File([P0, P1])
            )pbdoc")
        .def("__enter__", &PyFile::__enter__)
        .def("__exit__", &PyFile::__exit__)
        .def_readwrite("filename", &PyFile::filename,
             R"pbdoc(
             str : The filename the File was read from.

             Example
             -------
             >>> f = OpenEXR.File("image.exr")
             >>> f.filename
             'image.exr'
             )pbdoc")
        .def_readwrite("parts", &PyFile::parts,
             R"pbdoc(
             list : The image parts. The list has a single element for single-part files.
             Example
             -------
             >>> f = OpenEXR.File("image.exr")
             >>>> f.parts
             [Part("Part0", Compression.ZIPS_COMPRESSION, width=10, height=20)]         
             )pbdoc")
        .def("header", &PyFile::header, py::arg("part_index") = 0,
             R"pbdoc(
             dict : The header metadata for the given part if specified, or for the first part if not.

             Parameters
             ----------
             part_index : int
                 The index of the part. Defaults to 0.

             Example
             -------
             >>> f = OpenEXR.File("image.exr")
             >>> f.header()
             {'dataWindow': (array([0, 0], dtype=int32), array([100, 100], dtype=int32)), 'displayWindow': (array([0, 0], dtype=int32), array([100, 100], dtype=int32))}
             )pbdoc")
        .def("channels", &PyFile::channels, py::arg("part_index") = 0,
             R"pbdoc(
             Return a dict of channels given part if specified, or for the first part if not. The dict key is the channel name.

             Parameters
             ----------
             part_index : int
                 The index of the part. Defaults to 0.

             Example
             -------
             >>> f = OpenEXR.File("image.exr")
             >>> f.channels(0)
             {'A': Channel("A", xSampling=1, ySampling=1), 'B': Channel("B", xSampling=1, ySampling=1), 'G': Channel("G", xSampling=1, ySampling=1), 'R': Channel("R", xSampling=1, ySampling=1)}
             )pbdoc")
        .def("write", &PyFile::write,
             R"pbdoc(
             Write the File to the give file name.

             Parameters
             ----------
             filename : str
                 The output path name.

             Example
             -------
             >>> f = OpenEXR.File("image.exr")
             >>> f.write("out.exr"))pbdoc")
        ;
}

