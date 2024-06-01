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

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <numpy/arrayobject.h>

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

PyFile::PyFile(const py::list& p)
    : parts(p)
{
    for (size_t part_index = 0; part_index < parts.size(); part_index++)
    {
        auto p = parts[part_index];
        if (!py::isinstance<PyPart>(p))
            throw std::invalid_argument("must be a list of OpenEXR.Part() objects");

        auto P = p.cast<PyPart&>();
        P.part_index = part_index;
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
// If 'rgba' is true, gather 'R', 'G', 'B', and 'A' channels and interleave
// them into  a 3- or 4- (if 'A' is present) element numpy array. In the case
// of raw 'R', 'G', 'B', and 'A' channels, the corresponding key in the
// channels dict is "RGB" or "RGBA".  For channels with a prefix,
// e.g. "left.R", "left.G", etc, the channel key is the prefix.
//

PyFile::PyFile(const std::string& filename, bool rgba, bool header_only)
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
            P.header[py::str(name)] = get_attribute_object(name, &attribute);
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
        
        std::set<std::string> rgba_channels;
        if (rgba)
        {
            for (auto c = header.channels().begin(); c != header.channels().end(); c++)
            {
                std::string py_channel_name;
                char channel_name;
                if (P.rgba_channel(header.channels(), c.name(), py_channel_name, channel_name))
                    rgba_channels.insert(c.name());
            }
        }
        
        std::vector<size_t> shape ({height, width});

        //
        // Read the channel data, different for image vs. deep
        //
        
        auto type = header.type();
        if (type == SCANLINEIMAGE || type == TILEDIMAGE)
        {
            P.readPixels(infile, header.channels(), shape, rgba_channels, dw, rgba);
        }
        else if (type == DEEPSCANLINE || type == DEEPTILE)
        {
            P.readDeepPixels(infile, type, header.channels(), shape, rgba_channels, dw, rgba);
        }
        parts.append(py::cast<PyPart>(PyPart(P)));
    } // for parts
}

void
PyPart::readPixels(MultiPartInputFile& infile, const ChannelList& channel_list,
                   const std::vector<size_t>& shape, const std::set<std::string>& rgba_channels,
                   const Box2i& dw, bool rgba)
{
    FrameBuffer frameBuffer;

    for (auto c = channel_list.begin(); c != channel_list.end(); c++)
    {
        std::string py_channel_name = c.name();
        char channel_name; 
        int nrgba = 0;
        if (rgba)
            nrgba = rgba_channel(channel_list, c.name(), py_channel_name, channel_name);
            
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
            
            if (rgba_channels.find(c.name()) != rgba_channels.end())
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

#if DEBUG_VERBOSE
            std::cout << ":: creating PyChannel name=" << C.name
                      << " ndim=" << C.pixels.ndim()
                      << " size=" << C.pixels.size()
                      << " itemsize=" << C.pixels.dtype().itemsize()
                      << std::endl;
#endif
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
            
#if DEBUG_VERBOSE
        std::cout << "Creating slice from PyChannel name=" << C.name
                  << " ndim=" << C.pixels.ndim()
                  << " size=" << C.pixels.size()
                  << " itemsize=" << C.pixels.dtype().itemsize()
                  << " name=" << c.name()
                  << " type=" << c.channel().type
                  << std::endl;
#endif
        
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
PyPart::readDeepPixels(MultiPartInputFile& infile, const std::string& type, const ChannelList& channel_list,
                       const std::vector<size_t>& shape, const std::set<std::string>& rgba_channels,
                       const Box2i& dw, bool rgba)
{
    DeepFrameBuffer frameBuffer;

    size_t width  = dw.max.x - dw.min.x + 1;
    size_t height = dw.max.y - dw.min.y + 1;

    Array2D<unsigned int> sampleCount (height, width);

    frameBuffer.insertSampleCountSlice (Slice (
                                            UINT,
                                            (char*) (&sampleCount[0][0] - dw.min.x - dw.min.y * width),
                                            sizeof (unsigned int) * 1,       // xStride
                                            sizeof (unsigned int) * width)); // yStride

    for (auto c = channel_list.begin(); c != channel_list.end(); c++)
    {
        std::string py_channel_name = c.name();
#if XXX
        char channel_name; 
        int nrgba = 0;
        if (rgba)
            nrgba = rgba_channel(channel_list, c.name(), py_channel_name, channel_name);
#endif
        
        auto py_channel_name_str = py::str(py_channel_name);
            
        if (!channels.contains(py_channel_name_str))
        {
            // We haven't add a PyChannel yet, so add one now.
                
            PyChannel C;

            C.name = py_channel_name;
            C.xSampling = c.channel().xSampling;
            C.ySampling = c.channel().ySampling;
            C.pLinear = c.channel().pLinear;
            C._type = c.channel().type;
            
            C.deep_samples = new Array2D<void*>(height, width);
                    
            channels[py_channel_name.c_str()] = C;

#if DEBUG_VERBOSE
            std::cout << ":: creating deep PyChannel name=" << C.name
                      << " type=" << C._type
                      << std::endl;
#endif
        }

        auto v = channels[py_channel_name.c_str()];
        auto C = v.cast<PyChannel&>();

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

            
        auto &S = *C.deep_samples;
        auto basePtr = &S[0][0];
        
#if DEBUG_VERBOSE
        std::cout << "Creating deep slice from PyChannel name=" << C.name
                  << " size=" << C.pixels.size()
                  << " xStride=" << xStride
                  << " yStride=" << yStride
                  << " sampleStride=" << sampleStride
                  << std::endl;
#endif

        frameBuffer.insert (c.name(),
                            DeepSlice (c.channel().type,
                                       (char*) (basePtr - dw.min.x - dw.min.y * width),
                                       xStride,
                                       yStride,
                                       sampleStride,
                                       C.xSampling,
                                       C.ySampling));
    } // for header.channels()

    if (type == DEEPSCANLINE)
    {
        DeepScanLineInputPart part (infile, part_index);

        part.setFrameBuffer (frameBuffer);

        std::cout << "part.readPixelSampleCounts..." << std::endl;
        part.readPixelSampleCounts (dw.min.y, dw.max.y);
        std::cout << "part.readPixelSampleCounts...done." << std::endl;

        for (auto c : channels)
        {
            std::cout << "allocating..." << std::endl;

            auto C = c.second.cast<const PyChannel&>();
            auto &S = *C.deep_samples;
            for (size_t y=0; y<height; y++)
                for (size_t x=0; x<width; x++)
                {
                    auto size = sampleCount[y][x];
                    std::cout << "sampleCount[" << y << "][" << x << "]=" << sampleCount[y][x] << std::endl;
                    switch (C._type)
                    {
                      case UINT:
                          S[y][x] = static_cast<void*>(new uint32_t[size]);
                          break;
                      case HALF:
                          S[y][x] = static_cast<void*>(new half[size]);
                        break;
                      case FLOAT:
                          S[y][x] = static_cast<void*>(new float[size]);
                          break;
                      default:
                          throw std::runtime_error("invalid pixel type");
                    } // switch c->type
                }


            std::cout << "clearing..." << std::endl;
            
            for (size_t y=0; y<height; y++)
                for (size_t x=0; x<width; x++)
                {
                    auto size = sampleCount[y][x];
                    switch (C._type)
                    {
                      case UINT:
                          for (size_t i=0; i<size; i++)
                          {
                              auto s = static_cast<uint32_t*>(S[y][x]);
                              s[i] = 0;
                          }
                          break;
                      case HALF:
                          for (size_t i=0; i<size; i++)
                          {
                              auto s = static_cast<half*>(S[y][x]);
                              s[i] = 0;
                          }
                          break;
                      case FLOAT:
                          for (size_t i=0; i<size; i++)
                          {
                              auto s = static_cast<float*>(S[y][x]);
                              s[i] = 0;
                          }
                          break;
                      default:
                          throw std::runtime_error("invalid pixel type");
                    } // switch c->type
                }
        }
        
        std::cout << "part.readPixels..." << std::endl;
        part.readPixels (dw.min.y, dw.max.y);
        std::cout << "part.readPixels...done." << std::endl;

        for (auto c : channels)
        {
            auto C = c.second.cast<const PyChannel&>();
            for (size_t y=0; y<height; y++)
                for (size_t x=0; x<width; x++)
                {
                    auto size = sampleCount[y][x];
                    auto &S = *C.deep_samples;
                    switch (C._type)
                    {
                      case UINT:
                          for (size_t i=0; i<size; i++)
                          {
                              auto s = static_cast<uint32_t*>(S[y][x]);
                              std::cout << "sample: " << C.name << "[" << y << "][" << x << "][" << i << "]=" << s[i] << std::endl;
                          }
                          break;
                      case HALF:
                          for (size_t i=0; i<size; i++)
                          {
                              auto s = static_cast<half*>(S[y][x]);
                              std::cout << "sample: " << C.name << "[" << y << "][" << x << "][" << i << "]=" << s[i] << std::endl;
                          }
                          break;
                      case FLOAT:
                          for (size_t i=0; i<size; i++)
                          {
                              auto s = static_cast<float*>(S[y][x]);
                              std::cout << "sample: " << C.name << "[" << y << "][" << x << "][" << i << "]=" << s[i] << std::endl;
                          }
                          break;
                      default:
                          throw std::runtime_error("invalid pixel type");
                    } // switch c->type
                }
        }
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
PyPart::rgba_channel(const ChannelList& channel_list, const std::string& name,
                     std::string& py_channel_name, char& channel_name)
{
    py_channel_name = name;
    channel_name = py_channel_name.back();
    if (channel_name == 'R' ||
        channel_name == 'G' ||
        channel_name == 'B' ||
        channel_name == 'A')
    {
        // has the right final character. The preceding character is either a
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
            insert_attribute(header, name, second);
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
            auto t = static_cast<PixelType>(C.pixelType());

            if (C.pixels.ndim() == 3)
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

                header.channels ().insert(name_prefix + "R", Channel (t, C.xSampling, C.ySampling, C.pLinear));
                header.channels ().insert(name_prefix + "G", Channel (t, C.xSampling, C.ySampling, C.pLinear));
                header.channels ().insert(name_prefix + "B", Channel (t, C.xSampling, C.ySampling, C.pLinear));
                int nrgba = C.pixels.shape(2);
                if (nrgba > 3)
                    header.channels ().insert(name_prefix + "A", Channel (t, C.xSampling, C.ySampling, C.pLinear));
            }
            else
                header.channels ().insert(C.name, Channel (t, C.xSampling, C.ySampling, C.pLinear));
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
            FrameBuffer frameBuffer;
        
            for (auto c : P.channels)
            {
                auto C = c.second.cast<const PyChannel&>();

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
                    size_t yStride = xStride * P.width() / C.xSampling;
                    
                    auto rPtr = basePtr;
                    frameBuffer.insert (name_prefix + "R",
                                        Slice::Make (static_cast<PixelType>(C.pixelType()),
                                                     static_cast<void*>(rPtr),
                                                     dw, xStride, yStride,
                                                     C.xSampling,
                                                     C.ySampling));

                    auto gPtr = &basePtr[dt.itemsize()];
                    frameBuffer.insert (name_prefix + "G",
                                        Slice::Make (static_cast<PixelType>(C.pixelType()),
                                                     static_cast<void*>(gPtr),
                                                     dw, xStride, yStride,
                                                     C.xSampling,
                                                     C.ySampling));

                    auto bPtr = &basePtr[2*dt.itemsize()];
                    frameBuffer.insert (name_prefix + "B",
                                        Slice::Make (static_cast<PixelType>(C.pixelType()),
                                                     static_cast<void*>(bPtr),
                                                     dw, xStride, yStride,
                                                     C.xSampling,
                                                     C.ySampling));

                    if (nrgba == 4)
                    {
                        auto aPtr = &basePtr[3*dt.itemsize()];
                        frameBuffer.insert (name_prefix + "A",
                                            Slice::Make (static_cast<PixelType>(C.pixelType()),
                                                         static_cast<void*>(aPtr),
                                                         dw, xStride, yStride,
                                                         C.xSampling,
                                                         C.ySampling));
                    }
                }
                else
                {
                    frameBuffer.insert (C.name,
                                        Slice::Make (static_cast<PixelType>(C.pixelType()),
                                                     static_cast<void*>(C.pixels.request().ptr),
                                                     dw, 0, 0,
                                                     C.xSampling,
                                                     C.ySampling));
                }
            }
                
            if (P.type() == EXR_STORAGE_SCANLINE)
            {
                OutputPart part(outfile, part_index);
                part.setFrameBuffer (frameBuffer);
                part.writePixels (P.height());
            }
            else
            {
                TiledOutputPart part(outfile, part_index);
                part.setFrameBuffer (frameBuffer);
                part.writeTiles (0, part.numXTiles() - 1, 0, part.numYTiles() - 1);
            }
        }
        else if (P.type() == EXR_STORAGE_DEEP_SCANLINE ||
                 P.type() == EXR_STORAGE_DEEP_TILED)
        {
            DeepFrameBuffer frameBuffer;
        
            for (auto c : P.channels)
            {
                auto C = c.second.cast<const PyChannel&>();
                frameBuffer.insert (C.name,
                                    DeepSlice (static_cast<PixelType>(C.pixelType()),
                                               static_cast<char*>(C.pixels.request().ptr),
                                               0, 0, 0,
                                               C.xSampling,
                                               C.ySampling));
            }
        
            if (P.type() == EXR_STORAGE_DEEP_SCANLINE)
            {
                DeepScanLineOutputPart part(outfile, part_index);
                part.setFrameBuffer (frameBuffer);
                part.writePixels (P.height());
            }
            else 
            {
                DeepTiledOutputPart part(outfile, part_index);
                part.setFrameBuffer (frameBuffer);
                part.writeTiles (0, part.numXTiles() - 1, 0, part.numYTiles() - 1);
            }
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
PyFile::get_attribute_object(const std::string& name, const Attribute* a)
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
is_v2(const py::object& object, Vec2<T>& v)
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
is_v2i(const py::object& object, V2i& v)
{
    return is_v2<py::int_, int>(object, v);
}

bool
is_v2f(const py::object& object, V2f& v)
{
    return is_v2<py::float_, float>(object, v);
}

bool
is_v2d(const py::object& object, V2d& v)
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
is_v3(const py::object& object, Vec3<T>& v)
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
        if (a.ndim() == 1 && a.size() == 2)
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
is_v3i(const py::object& object, V3i& v)
{
    return is_v3<py::int_, int>(object, v);
}

bool
is_v3f(const py::object& object, V3f& v)
{
    return is_v3<py::float_, float>(object, v);
}

bool
is_v3d(const py::object& object, V3d& v)
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
is_m33(const py::object& object, Matrix33<T>& m)
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
is_m44(const py::object& object, Matrix44<T>& m)
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
is_box2i(const py::object& object, Box2i& b)
{
    if (py::isinstance<py::tuple>(object))
    {
        auto tup = object.cast<py::tuple>();
        if (tup.size() == 2)
            if (is_v2i(tup[0], b.min) && is_v2i(tup[1], b.max))
                return true;
    }

    return false;
}
         
bool
is_box2f(const py::object& object, Box2f& b)
{
    if (py::isinstance<py::tuple>(object))
    {
        auto tup = object.cast<py::tuple>();
        if (tup.size() == 2)
        {
            Box2f box;
            if (is_v2f(tup[0], box.min) && is_v2f(tup[1], box.max))
                return true;
        }
    }

    return false;
}
         
bool
is_chromaticities(const py::object& object, Chromaticities& v)
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

#if XXX

template <class T>
Vec2<T>
get_v2(const py::array& a)
{
    py::buffer_info buf = a.request();
    auto v = static_cast<const T*>(buf.ptr);
    return Vec2<T>(v[0], v[1]);
}

template <class T>
Vec3<T>
get_v3(const py::array& a)
{
    py::buffer_info buf = a.request();
    auto v = static_cast<const T*>(buf.ptr);
    return Vec3<T>(v[0], v[1], v[2]);
}

template <class T>
Matrix33<T>
get_m33(const py::array& a)
{
    py::buffer_info buf = a.request();
    auto v = static_cast<const T*>(buf.ptr);
    return Matrix33<T>(v[0], v[1], v[2],
                       v[3], v[4], v[5],
                       v[6], v[7], v[8]);
}

template <class T>
Matrix44<T>
get_m44(const py::array& a)
{
    py::buffer_info buf = a.request();
    auto v = static_cast<const T*>(buf.ptr);
    return Matrix44<T>(v[0], v[1], v[2], v[3],
                       v[4], v[5], v[6], v[7],
                       v[8], v[9], v[10], v[11],
                       v[12], v[13], v[14], v[15]);
}
#endif

void
PyFile::insert_attribute(Header& header, const std::string& name, const py::object& object)
{
#if XXX
    std::cout << "insert_attribute " << name << ": " << py::str(object) << std::endl;
#endif
    
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
        if (is_box2i(object, b))
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
        if (is_v2f(object, v))
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
        if (is_chromaticities(object, c))
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
    if (is_v2i(object, v2i))
    {       
        header.insert(name, V2iAttribute(v2i));
        return;
    }

    V2f v2f;
    if (is_v2f(object, v2f))
    {       
        header.insert(name, V2fAttribute(v2f));
        return;
    }

    V2d v2d;
    if (is_v2d(object, v2d))
    {       
        header.insert(name, V2dAttribute(v2d));
        return;
    }

    V3i v3i;
    if (is_v3i(object, v3i))
    {       
        header.insert(name, V3iAttribute(v3i));
        return;
    }

    V3f v3f;
    if (is_v3f(object, v3f))
    {       
        header.insert(name, V3fAttribute(v3f));
        return;
    }

    V3d v3d;
    if (is_v3d(object, v3d))
    {       
        header.insert(name, V3dAttribute(v3d));
        return;
    }

    M33f m33f;
    if (is_m33(object, m33f))
    {       
        header.insert(name, M33fAttribute(m33f));
        return;
    }

    M33d m33d;
    if (is_m33(object, m33d))
    {       
        header.insert(name, M33dAttribute(m33d));
        return;
    }

    M44f m44f;
    if (is_m44(object, m44f))
    {       
        header.insert(name, M44fAttribute(m44f));
        return;
    }

    M44d m44d;
    if (is_m44(object, m44d))
    {       
        header.insert(name, M44dAttribute(m44d));
        return;
    }

    //
    // Recognize 2-tuples of 2-vectors as boxes
    //
    
    Box2i box2i;
    if (is_box2i(object, box2i))
    {       
        header.insert(name, Box2iAttribute(box2i));
        return;
    }

    Box2f box2f;
    if (is_box2f(object, box2f))
    {       
        header.insert(name, Box2fAttribute(box2f));
        return;
    }

    //
    // Recognize an 8-tuple as chromaticities
    //
    
    Chromaticities c;
    if (is_chromaticities(object, c))
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
    // Validate that all channel dict keys are strings, and initialze the
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
PyChannel::validate_pixel_array()
{
    if (!(py::isinstance<py::array_t<uint32_t>>(pixels) ||
          py::isinstance<py::array_t<half>>(pixels) ||
          py::isinstance<py::array_t<float>>(pixels)))
        throw std::invalid_argument("invalid pixel array: unrecognized type: must be uint32, half, or float");

    if (pixels.ndim() < 2 ||  pixels.ndim() > 3)
        throw std::invalid_argument("invalid pixel array: must be 2D or 3D numpy array");
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
    auto buf = pybind11::array::ensure(pixels);
    if (buf)
    {
        if (py::isinstance<py::array_t<uint32_t>>(buf))
            return UINT;
        if (py::isinstance<py::array_t<half>>(buf))
            return HALF;      
        if (py::isinstance<py::array_t<float>>(buf))
            return FLOAT;
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


// Helper function to create a 1D NumPy array of integers
static PyObject* create_1d_array(int size) {
    npy_intp dims[1] = {size};
    PyObject *array = PyArray_SimpleNew(1, dims, NPY_INT);
    int *data = (int *)PyArray_DATA((PyArrayObject *)array);
    for (int i = 0; i < size; ++i) {
        data[i] = i;
    }
    return array;
}

// Main function to create the 2D array of arrays
static PyObject* create_2d_array_of_arrays(void) {

    Py_Initialize();
    import_array(); // Initialize NumPy

    npy_intp dims[2] = {3, 3};
    PyObject *array_of_arrays = PyArray_SimpleNew(2, dims, NPY_OBJECT);

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            int size = i * 3 + j;
            PyObject *subarray = create_1d_array(size);
            // Get a pointer to the element in the 2D array and set the subarray
            *(PyObject **)PyArray_GETPTR2((PyArrayObject *)array_of_arrays, i, j) = subarray;
        }
    }

    return array_of_arrays;
}

py::object
py_create_2d_array_of_arrays(void)
{
    auto py_obj = create_2d_array_of_arrays();
    return py::reinterpret_borrow<py::object>(py_obj);
}

#if XXX
int main(int argc, char *argv[]) {
    // Initialize the Python interpreter
    Py_Initialize();
    import_array(); // Initialize NumPy

    // Create the 2D array of arrays
    PyObject *array_of_arrays = create_2d_array_of_arrays();

    // Print the result (optional, for verification)
    PyObject_Print(array_of_arrays, stdout, 0);
    printf("\n");

    // Clean up and exit
    Py_DECREF(array_of_arrays);
    Py_Finalize();
    return 0;
}
#endif

Chromaticities
chromaticities(float redx,
               float redy, 
               float greenx,
               float greeny,
               float bluex,
               float bluey,
               float whitex,
               float whitey)
{
    return Chromaticities(V2f(redx, redy), V2f(greenx, greeny), V2f(bluex, bluey), V2f(whitex, whitey));
}

#define DEEP_EXAMPLE 0

#if DEEP_EXAMPLE
#include "deepExample.cpp"
#endif

PYBIND11_MODULE(OpenEXR, m)
{
    using namespace py::literals;

    m.doc() = "OpenEXR - read and write high-dynamic range image files";
    
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
    
    py::class_<PyChannel>(m, "Channel")
        .def(py::init())
        .def(py::init<int,int,bool>(),
             py::arg("xSampling"),
             py::arg("ySampling"),
             py::arg("pLinear")=false)
        .def(py::init<py::array>())
        .def(py::init<py::array,int,int,bool>(),
             py::arg("pixels"),
             py::arg("xSampling"),
             py::arg("ySampling"),
             py::arg("pLinear")=false)
        .def(py::init<const char*>(),
             py::arg("name"))
        .def(py::init<const char*,int,int,bool>(),
             py::arg("name"),
             py::arg("xSampling"),
             py::arg("ySampling"),
             py::arg("pLinear")=false)
        .def(py::init<const char*,py::array>(),
             py::arg("name"),
             py::arg("pixels"))
        .def(py::init<const char*,py::array,int,int,bool>(),
             py::arg("name"),
             py::arg("pixels"),
             py::arg("xSampling"),
             py::arg("ySampling"),
             py::arg("pLinear")=false)
        .def("__repr__", [](const PyChannel& c) { return repr(c); })
#if XXX
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("diff", &PyChannel::diff)
#endif
        .def_readwrite("name", &PyChannel::name)
        .def("type", &PyChannel::pixelType)
        .def_readwrite("xSampling", &PyChannel::xSampling)
        .def_readwrite("ySampling", &PyChannel::ySampling)
        .def_readwrite("pLinear", &PyChannel::pLinear)
        .def_readwrite("pixels", &PyChannel::pixels)
        .def_readonly("channel_index", &PyChannel::channel_index)
        ;
    
    py::class_<PyPart>(m, "Part")
        .def(py::init())
        .def(py::init<py::dict,py::dict,std::string>(),
             py::arg("header"),
             py::arg("channels"),
             py::arg("name")="")
        .def("__repr__", [](const PyPart& p) { return repr(p); })
#if XXX
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("diff", &PyPart::diff)
#endif
        .def("name", &PyPart::name)
        .def("type", &PyPart::type)
        .def("width", &PyPart::width)
        .def("height", &PyPart::height)
        .def("compression", &PyPart::compression)
        .def_readwrite("header", &PyPart::header)
        .def_readwrite("channels", &PyPart::channels)
        .def_readonly("part_index", &PyPart::part_index)
        ;

    py::class_<PyFile>(m, "File")
        .def(py::init<>())
        .def(py::init<std::string,bool,bool>(),
             py::arg("filename"),
             py::arg("rgba")=false,
             py::arg("header_only")=false)
        .def(py::init<py::dict,py::dict>(),
             py::arg("header"),
             py::arg("channels"))
        .def(py::init<py::list>(),
             py::arg("parts"))
        .def("__enter__", &PyFile::__enter__)
        .def("__exit__", &PyFile::__exit__)
#if XXX
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("diff", &PyFile::diff)
#endif
        .def_readwrite("filename", &PyFile::filename)
        .def_readwrite("parts", &PyFile::parts)
        .def("header", &PyFile::header, py::arg("part_index") = 0)
        .def("channels", &PyFile::channels, py::arg("part_index") = 0)
        .def("write", &PyFile::write)
        ;

#if DEEP_EXAMPLE
//    import_array();
    
    m.def("create_2d_array_of_arrays", &py_create_2d_array_of_arrays);
    m.def("writeDeepExample", &writeDeepExample);
    m.def("readDeepExample", &readDeepExample);
#endif
}

