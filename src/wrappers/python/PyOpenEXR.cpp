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

#include <typeinfo>
#include <sys/types.h>

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

// WIP
void
PyPart::readDeepPixels(MultiPartInputFile& infile, const std::string& type, const ChannelList& channel_list,
                       const std::vector<size_t>& shape, const std::set<std::string>& rgba_channels,
                       const Box2i& dw, bool rgba)
{
    DeepFrameBuffer frameBuffer;

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
            // We haven't add a PyChannel yet, so add one one.
                
            PyChannel C;

            C.name = py_channel_name;
            C.xSampling = c.channel().xSampling;
            C.ySampling = c.channel().ySampling;
            C.pLinear = c.channel().pLinear;
                
            const auto style = py::array::c_style | py::array::forcecast;

            std::vector<size_t> c_shape = shape;

            // If this channel belongs to one of the rgba's, give
            // the PyChannel the proper shape
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

        auto v = channels[py_channel_name.c_str()];
        auto C = v.cast<PyChannel&>();

        py::buffer_info buf = C.pixels.request();
        auto basePtr = static_cast<uint8_t*>(buf.ptr);
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

        size_t yStride = xStride * shape[1];
        size_t sampleStride = 0;

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
                            DeepSlice (c.channel().type,
                                       (char*) basePtr,
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
        part.readPixels (dw.min.y, dw.max.y);
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

bool
PyFile::operator==(const PyFile& other) const
{
    if (parts.size() != other.parts.size())
    {
        std::cout << "PyFile:: #parts differs." << std::endl;
        return false;
    }
    
    for (size_t part_index = 0; part_index<parts.size(); part_index++)
    {
        auto a = parts[part_index].cast<const PyPart&>();
        auto b = other.parts[part_index].cast<const PyPart&>();
        if (a != b)
        {
            std::cout << "PyFile: part " << part_index << " differs." << std::endl;
            return false;
        }
    }
    
    return true;
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
            else
                std::cout << "> no tile description" << std::endl;
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

py::object
PyFile::get_attribute_object(const std::string& name, const Attribute* a)
{
    if (auto v = dynamic_cast<const Box2iAttribute*> (a))
        return py::cast(Box2i(v->value()));

    if (auto v = dynamic_cast<const Box2fAttribute*> (a))
        return py::cast(Box2f(v->value()));

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
        return py::cast(v->value());

    if (auto v = dynamic_cast<const CompressionAttribute*> (a))
        return py::cast(v->value());

    if (auto v = dynamic_cast<const DoubleAttribute*> (a))
        return py::cast(PyDouble(v->value()));

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
        return py::cast(M33f(v->value()[0][0],
                             v->value()[0][1],
                             v->value()[0][2],
                             v->value()[1][0],
                             v->value()[1][1],
                             v->value()[1][2],
                             v->value()[2][0],
                             v->value()[2][1],
                             v->value()[2][2]));

    if (auto v = dynamic_cast<const M33dAttribute*> (a))
        return py::cast(M33d(v->value()[0][0],
                             v->value()[0][1],
                             v->value()[0][2],
                             v->value()[1][0],
                             v->value()[1][1],
                             v->value()[1][2],
                             v->value()[2][0],
                             v->value()[2][1],
                             v->value()[2][2]));

    if (auto v = dynamic_cast<const M44fAttribute*> (a))
        return py::cast(M44f(v->value()[0][0],
                             v->value()[0][1],
                             v->value()[0][2],
                             v->value()[0][3],
                             v->value()[1][0],
                             v->value()[1][1],
                             v->value()[1][2],
                             v->value()[1][3],
                             v->value()[2][0],
                             v->value()[2][1],
                             v->value()[2][2],
                             v->value()[2][3],
                             v->value()[3][0],
                             v->value()[3][1],
                             v->value()[3][2],
                             v->value()[3][3]));

    if (auto v = dynamic_cast<const M44dAttribute*> (a))
        return py::cast(M44d(v->value()[0][0],
                             v->value()[0][1],
                             v->value()[0][2],
                             v->value()[0][3],
                             v->value()[1][0],
                             v->value()[1][1],
                             v->value()[1][2],
                             v->value()[1][3],
                             v->value()[2][0],
                             v->value()[2][1],
                             v->value()[2][2],
                             v->value()[2][3],
                             v->value()[3][0],
                             v->value()[3][1],
                             v->value()[3][2],
                             v->value()[3][3]));

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
        return py::cast(v->value());

    if (auto v = dynamic_cast<const TileDescriptionAttribute*> (a))
        return py::cast(v->value());

    if (auto v = dynamic_cast<const TimeCodeAttribute*> (a))
        return py::cast(v->value());

    if (auto v = dynamic_cast<const V2iAttribute*> (a))
        return py::cast(V2i(v->value().x, v->value().y));

    if (auto v = dynamic_cast<const V2fAttribute*> (a))
        return py::cast(V2f(v->value().x, v->value().y));

    if (auto v = dynamic_cast<const V2dAttribute*> (a))
        return py::cast(V2d(v->value().x, v->value().y));

    if (auto v = dynamic_cast<const V3iAttribute*> (a))
        return py::cast(V3i(v->value().x, v->value().y, v->value().z));
    
    if (auto v = dynamic_cast<const V3fAttribute*> (a))
        return py::cast(V3f(v->value().x, v->value().y, v->value().z));
    
    if (auto v = dynamic_cast<const V3dAttribute*> (a))
        return py::cast(V3d(v->value().x, v->value().y, v->value().z));
    
    throw std::runtime_error("unrecognized attribute type");
    
    return py::none();
}
    
void
PyFile::insert_attribute(Header& header, const std::string& name, const py::object& object)
{
    if (auto v = py_cast<Box2i>(object))
        header.insert(name, Box2iAttribute(*v));
    else if (auto v = py_cast<Box2f>(object))
        header.insert(name, Box2fAttribute(*v));
    else if (py::isinstance<py::list>(object))
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
    else if (auto v = py_cast<Chromaticities>(object))
        header.insert(name, ChromaticitiesAttribute(static_cast<Chromaticities>(*v)));
    else if (auto v = py_cast<Compression>(object))
        header.insert(name, CompressionAttribute(static_cast<Compression>(*v)));
    else if (auto v = py_cast<Envmap>(object))
        header.insert(name, EnvmapAttribute(static_cast<Envmap>(*v)));
    else if (py::isinstance<py::float_>(object))
        header.insert(name, FloatAttribute(py::cast<py::float_>(object)));
    else if (py::isinstance<PyDouble>(object))
        header.insert(name, DoubleAttribute(py::cast<PyDouble>(object).d));
    else if (py::isinstance<py::int_>(object))
        header.insert(name, IntAttribute(py::cast<py::int_>(object)));
    else if (auto v = py_cast<KeyCode>(object))
        header.insert(name, KeyCodeAttribute(*v));
    else if (auto v = py_cast<LineOrder>(object))
        header.insert(name, LineOrderAttribute(static_cast<LineOrder>(*v)));
    else if (auto v = py_cast<M33f>(object))
        header.insert(name, M33fAttribute(*v));
    else if (auto v = py_cast<M33d>(object))
        header.insert(name, M33dAttribute(*v));
    else if (auto v = py_cast<M44f>(object))
        header.insert(name, M44fAttribute(*v));
    else if (auto v = py_cast<M44d>(object))
        header.insert(name, M44dAttribute(*v));
    else if (auto v = py_cast<PyPreviewImage>(object))
    {
        py::buffer_info buf = v->pixels.request();
        auto pixels = static_cast<PreviewRgba*>(buf.ptr);
        auto height = v->pixels.shape(0);
        auto width = v->pixels.shape(1);
        PreviewImage p(width, height, pixels);
        header.insert(name, PreviewImageAttribute(p));
    }
    else if (auto v = py_cast<Rational>(object))
        header.insert(name, RationalAttribute(*v));
    else if (auto v = py_cast<TileDescription>(object))
        header.insert(name, TileDescriptionAttribute(*v));
    else if (auto v = py_cast<TimeCode>(object))
        header.insert(name, TimeCodeAttribute(*v));
    else if (auto v = py_cast<V2i>(object))
        header.insert(name, V2iAttribute(*v));
    else if (auto v = py_cast<V2f>(object))
        header.insert(name, V2fAttribute(*v));
    else if (auto v = py_cast<V2d>(object))
        header.insert(name, V2dAttribute(*v));
    else if (auto v = py_cast<V3i>(object))
        header.insert(name, V3iAttribute(*v));
    else if (auto v = py_cast<V3f>(object))
        header.insert(name, V3fAttribute(*v));
    else if (auto v = py_cast<V3d>(object))
        header.insert(name, V3dAttribute(*v));
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
    else
    {
        std::stringstream s;
        s << "unknown attribute type: " << py::str(object);
        throw std::runtime_error(s.str());
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

        c.second.cast<PyChannel&>().name = py::str(c.first);
    }

    auto s = shape();

    if (!header.contains("dataWindow"))
        header["dataWindow"] = py::cast(Box2i(V2i(0,0), V2i(s[1]-1,s[0]-1)));

    if (!header.contains("displayWindow"))
        header["displayWindow"] = py::cast(Box2i(V2i(0,0), V2i(s[1]-1,s[0]-1)));
}

bool
is_required_attribute(const std::string& name)
{
    return (name == "channels" ||
            name == "compression" ||
            name == "dataWindow" ||
            name == "displayWindow" ||
            name == "lineOrder" || 
            name == "pixelAspectRatio" ||
            name == "screenWindowCenter" ||
            name == "screenWindowWidth" ||
            name == "tiles" ||
            name == "type" ||
            name == "name" ||
            name == "version" ||
            name == "chunkCount");
}
            
bool
equal_header(const py::dict& A, const py::dict& B)
{
    std::set<std::string> names;
    
#if DEBUG_VERBOSE
    std::cout << "A:";
#endif
    for (auto a : A)
    {
#if DEBUG_VERBOSE
        std::cout << " " << py::str(a.first);
#endif
        names.insert(py::str(a.first));
    }
#if DEBUG_VERBOSE
    std::cout << std::endl;

    std::cout << "B:";
#endif
    for (auto b : B)
    {
#if DEBUG_VERBOSE
        std::cout << " " << py::str(b.first);
#endif
        names.insert(py::str(b.first));
    }
#if DEBUG_VERBOSE
    std::cout << std::endl;
#endif
    
    for (auto name : names)
    {
        if (name == "channels")
            continue;
                
        if (!A.contains(name))
        {
            if (is_required_attribute(name))
                continue;
            std::cout << "lhs part does not contain " << name << std::endl;
            return false;
        }

        if (!B.contains(name))
        {
            if (is_required_attribute(name))
                continue;
            std::cout << "rhs part does not contain " << name << std::endl;
            return false;
        }
            
        py::object a = A[py::str(name)];
        py::object b = B[py::str(name)];
        if (!a.equal(b))
        {
            if (py::isinstance<py::float_>(a))
            {                
                float f = py::cast<py::float_>(a);
                float of = py::cast<py::float_>(b);
                if (f == of)
                    return true;
                
                if (equalWithRelError(f, of, 1e-8f))
                {
                    float df = f - of;
                    std::cout << "float values are very close: "
                              << std::scientific << std::setprecision(12)
                              << f << " "
                              << of << " ("
                              << df << ")"
                              << std::endl;
                    return true;
                }
            }
            std::cout << "attribute values differ: " << name << " lhs='" << py::str(a) << "' rhs='" << py::str(b) << "'" << std::endl;
            return false;
        }
    }
    

    return true;
}
    

bool
PyPart::operator==(const PyPart& other) const
{
    if (!equal_header(header, other.header))
    {
        std::cout << "PyPart: !equal_header" << std::endl;
        return false;
    }
        
    //
    // The channel dicts might not be in alphabetical order
    // (they're sorted on write), so don't just compare the dicts
    // directly, compare each entry by key/name.
    //
    
    if (channels.size() != other.channels.size())
    {
        std::cout << "PyPart: #channels differs." << std::endl;
        return false;
    }
        
    for (auto c : channels)
    {
        auto name = py::str(c.first);
        auto C = c.second.cast<const PyChannel&>();
        auto O = other.channels[py::str(name)].cast<const PyChannel&>();
        if (C != O)
        {
            std::cout << "channel " << name << " differs." << std::endl;
            return false;
        }
    }
        
    return true;
}

template <class T>
bool
py_nan(T a)
{
    return std::isnan(a);
}

template <>
bool
py_nan<half>(half a)
{
    return a.isNan();
}

template <>
bool
py_nan<uint32_t>(uint32_t a)
{
    return false;
}

template <class T>
bool
py_inf(T a)
{
    return !std::isfinite(a);
}

template <>
bool
py_inf<half>(half a)
{
    return a.isInfinity();
}

template <>
bool
py_inf<uint32_t>(uint32_t a)
{
    return false;
}


template <class T>
bool
array_equals(const py::buffer_info& a, const py::buffer_info& b,
             const std::string& name, int width, int height, int depth = 1)
{
    const T* apixels = static_cast<const T*>(a.ptr);
    const T* bpixels = static_cast<const T*>(b.ptr);

    for (int y=0; y<height; y++)
        for (int x=0; x<width; x++)
        {
            int i = (y * width + x) * depth;
            for (int j=0; j<depth; j++)
            {
                int k = i + j;
                if (py_nan(apixels[k]) && py_nan(bpixels[k]))
                    continue;
                if (py_inf(apixels[k]) && py_inf(bpixels[k]))
                    continue;
                double ap = static_cast<double>(apixels[k]);
                double bp = static_cast<double>(bpixels[k]);
                if (!equalWithRelError(ap, bp, 1e-5))
                {
                    std::cout << " a[" << y
                              << "][" << x
                              << "][" << j
                              << "]=" << apixels[k]
                              << " b=[" << y
                              << "][" << x
                              << "][" << j
                              << "]=" << bpixels[k]
                              << std::endl;
                    return false;
                }
            }
        }

    return true;
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

bool
PyChannel::operator==(const PyChannel& other) const
{
    if (name == other.name && 
        xSampling == other.xSampling && 
        ySampling == other.ySampling &&
        pLinear == other.pLinear &&
        pixels.ndim() == other.pixels.ndim() && 
        pixels.size() == other.pixels.size())
    {
        if (pixels.size() == 0)
            return true;
        
        py::buffer_info buf = pixels.request();
        py::buffer_info obuf = other.pixels.request();

        int width = pixels.shape(1);
        int height = pixels.shape(0);
        int depth = pixels.ndim() == 3 ? pixels.shape(2) : 1;
        
        if (py::isinstance<py::array_t<uint32_t>>(pixels) && py::isinstance<py::array_t<uint32_t>>(other.pixels))
            if (array_equals<uint32_t>(buf, obuf, name, width, height, depth))
                return true;
        if (py::isinstance<py::array_t<half>>(pixels) && py::isinstance<py::array_t<half>>(other.pixels))
            if (array_equals<half>(buf, obuf, name, width, height, depth))
                return true;
        if (py::isinstance<py::array_t<float>>(pixels) && py::isinstance<py::array_t<float>>(other.pixels))
            if (array_equals<float>(buf, obuf, name, width, height, depth))
                return true;
    }

    return false;
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

    py::class_<Chromaticities>(m, "Chromaticities", "CIE (x,y) chromaticities of the primaries and the white point")
        .def(py::init<V2f,V2f,V2f,V2f>())
        .def(py::self == py::self)
        .def("__repr__", [](const Chromaticities& v) { return repr(v); })
        .def_readwrite("red", &Chromaticities::red)
        .def_readwrite("green", &Chromaticities::green)
        .def_readwrite("blue", &Chromaticities::blue)
        .def_readwrite("white", &Chromaticities::white)
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
    
    py::class_<PyDouble>(m, "Double")
        .def(py::init<double>())
        .def("__repr__", [](const PyDouble& d) { return repr(d.d); })
        .def(py::self == py::self)
        ;

    //
    // Stand-in Imath classes - these should really come from the Imath module.
    //
    
    py::class_<V2i>(m, "V2i")
        .def(py::init())
        .def(py::init<int,int>())
        .def("__repr__", [](const V2i& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("x", &Imath::V2i::x)
        .def_readwrite("y", &Imath::V2i::y)
        ;

    py::class_<V2f>(m, "V2f")
        .def(py::init())
        .def(py::init<float,float>())
        .def("__repr__", [](const V2f& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("x", &Imath::V2f::x)
        .def_readwrite("y", &Imath::V2f::y)
        ;

    py::class_<V2d>(m, "V2d")
        .def(py::init())
        .def(py::init<double,double>())
        .def("__repr__", [](const V2d& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("x", &Imath::V2d::x)
        .def_readwrite("y", &Imath::V2d::y)
        ;

    py::class_<V3i>(m, "V3i")
        .def(py::init())
        .def(py::init<int,int,int>())
        .def("__repr__", [](const V3i& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("x", &Imath::V3i::x)
        .def_readwrite("y", &Imath::V3i::y)
        .def_readwrite("z", &Imath::V3i::z)
        ;

    py::class_<V3f>(m, "V3f")
        .def(py::init())
        .def(py::init<float,float,float>())
        .def("__repr__", [](const V3f& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("x", &Imath::V3f::x)
        .def_readwrite("y", &Imath::V3f::y)
        .def_readwrite("z", &Imath::V3f::z)
        ;

    py::class_<V3d>(m, "V3d")
        .def(py::init())
        .def(py::init<double,double,double>())
        .def("__repr__", [](const V3d& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("x", &Imath::V3d::x)
        .def_readwrite("y", &Imath::V3d::y)
        .def_readwrite("z", &Imath::V3d::z)
        ;

    py::class_<Box2i>(m, "Box2i")
        .def(py::init())
        .def(py::init<V2i,V2i>())
        .def("__repr__", [](const Box2i& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("min", &Box2i::min)
        .def_readwrite("max", &Box2i::max)
        ;
    
    py::class_<Box2f>(m, "Box2f")
        .def(py::init())
        .def(py::init<V2f,V2f>())
        .def("__repr__", [](const Box2f& v) { return repr(v); })
        .def(py::self == py::self)
        .def_readwrite("min", &Box2f::min)
        .def_readwrite("max", &Box2f::max)
        ;
    
    py::class_<M33f>(m, "M33f")
        .def(py::init())
        .def(py::init<float,float,float,float,float,float,float,float,float>())
        .def("__repr__", [](const M33f& m) { return repr(m); })
        .def(py::self == py::self)
        ;
    
    py::class_<M33d>(m, "M33d")
        .def(py::init())
        .def(py::init<double,double,double,double,double,double,double,double,double>())
        .def("__repr__", [](const M33d& m) { return repr(m); })
        .def(py::self == py::self)
        ;
    
    py::class_<M44f>(m, "M44f")
        .def(py::init<float,float,float,float,
                      float,float,float,float,
                      float,float,float,float,
                      float,float,float,float>())
        .def(py::self == py::self)
        .def("__repr__", [](const M44f& m) { return repr(m); })
        ;
    
    py::class_<M44d>(m, "M44d")
        .def(py::init<double,double,double,double,
                      double,double,double,double,
                      double,double,double,double,
                      double,double,double,double>())
        .def("__repr__", [](const M44d& m) { return repr(m); })
        .def(py::self == py::self)
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
        .def(py::self == py::self)
        .def(py::self != py::self)
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
        .def(py::self == py::self)
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
        .def(py::self == py::self)
        .def_readwrite("filename", &PyFile::filename)
        .def_readwrite("parts", &PyFile::parts)
        .def("header", &PyFile::header, py::arg("part_index") = 0)
        .def("channels", &PyFile::channels, py::arg("part_index") = 0)
        .def("write", &PyFile::write)
        ;
}

