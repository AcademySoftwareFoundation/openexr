//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "ImfContext.h"

#include "openexr.h"

#include "Iex.h"

// TODO: remove these once we've cleared the legacy stream need
#include "ImfIO.h"
#include "ImfStdIO.h"
#include <mutex>

#include <string.h>

#include <ImfBoxAttribute.h>
#include <ImfChannelListAttribute.h>
#include <ImfChromaticitiesAttribute.h>
#include <ImfCompressionAttribute.h>
#include <ImfCompressor.h>
#include <ImfDeepImageStateAttribute.h>
#include <ImfDoubleAttribute.h>
#include <ImfDwaCompressor.h>
#include <ImfEnvmapAttribute.h>
#include <ImfFloatAttribute.h>
#include <ImfFloatVectorAttribute.h>
#include <ImfIDManifestAttribute.h>
#include <ImfIntAttribute.h>
#include <ImfKeyCodeAttribute.h>
#include <ImfLineOrderAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfOpaqueAttribute.h>
#include <ImfPreviewImageAttribute.h>
#include <ImfRationalAttribute.h>
#include <ImfStringAttribute.h>
#include <ImfStringVectorAttribute.h>
#include <ImfTileDescriptionAttribute.h>
#include <ImfTimeCodeAttribute.h>
#include <ImfVecAttribute.h>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

namespace
{

class MemAttrStream : public OPENEXR_IMF_NAMESPACE::IStream
{
public:
    MemAttrStream (const exr_attr_opaquedata_t *opaque)
        : IStream ("<mem_attr>"),
          _data (static_cast<char*> (opaque->packed_data)),
          _sz (static_cast<uint64_t> (opaque->size)),
          _pos (0)
    {}

    ~MemAttrStream () override {}

    bool isMemoryMapped () const override { return true; }

    bool     read (char c[/*n*/], int n) override
    {
        if (_pos >= _sz && n != 0)
            throw IEX_NAMESPACE::InputExc ("Unexpected end of file.");

        uint64_t n2     = n;
        bool     retVal = true;

        if (_sz - _pos <= n2)
        {
            n2     = _sz - _pos;
            retVal = false;
        }

        memcpy (c, _data + _pos, n2);
        _pos += n2;
        return retVal;
    }

    char*    readMemoryMapped (int n) override
    {
        if (_pos >= _sz)
            throw IEX_NAMESPACE::InputExc ("Unexpected end of file.");

        if (_pos + n > _sz)
            throw IEX_NAMESPACE::InputExc ("Reading past end of file.");

        char* retVal = _data + _pos;
        _pos += n;
        return retVal;
    }
    uint64_t tellg () override { return _pos; }
    void     seekg (uint64_t pos) override { _pos = pos; }
    void     clear () override {}

private:
    char*    _data;
    uint64_t _sz;
    uint64_t _pos;
};

} // namespace

////////////////////////////////////////

Context::Context ()
    : _ctxt (new exr_context_t(), [] (exr_context_t* todel) {
        exr_finish (todel);
        delete todel;
    })
{
}

////////////////////////////////////////

Context::Context (const char* filename, const ContextInitializer& ctxtinit, read_mode_t)
    : Context()
{
    exr_result_t rv;

    rv = exr_start_read (_ctxt.get (), filename, &(ctxtinit._initializer));
    if (EXR_ERR_SUCCESS != rv)
    {
        if (rv == EXR_ERR_MISSING_REQ_ATTR)
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Invalid or missing attribute when attempting to open '"
                << filename << "' for read");
        }
        else
        {
            THROW (
                IEX_NAMESPACE::InputExc,
                "Unable to open '" << filename << "' for read");
        }
    }
}

////////////////////////////////////////

Context::Context (const char* filename, const ContextInitializer& ctxtinit, temp_mode_t)
    : Context()
{
    if (EXR_ERR_SUCCESS != exr_start_temporary_context (
                               _ctxt.get (),
                               filename,
                               &(ctxtinit._initializer)))
    {
        THROW (
            IEX_NAMESPACE::InputExc,
            "Unable to create a temporary context");
    }
}

////////////////////////////////////////

Context::Context (const char* filename, const ContextInitializer& ctxtinit, write_mode_t)
    : Context()
{
    if (EXR_ERR_SUCCESS != exr_start_write (
                               _ctxt.get (),
                               filename,
                               EXR_WRITE_FILE_DIRECTLY,
                               &(ctxtinit._initializer)))
    {
        THROW (
            IEX_NAMESPACE::InputExc,
            "Unable to open '" << filename << "' for write");
    }
}

////////////////////////////////////////

void
Context::setLongNameSupport (bool onoff)
{
    if (EXR_ERR_SUCCESS != exr_set_longname_support (*_ctxt, onoff ? 1 : 0))
    {
        THROW (IEX_NAMESPACE::ArgExc, "Unable to set long name support flag");
    }
}

////////////////////////////////////////

const char*
Context::fileName () const
{
    const char* filename = nullptr;

    if (EXR_ERR_SUCCESS != exr_get_file_name (*_ctxt, &filename))
    {
        THROW (IEX_NAMESPACE::ArgExc, "Unable to get filename from context");
    }

    return filename;
}

////////////////////////////////////////

int
Context::version () const
{
    uint32_t ver = 0;

    if (EXR_ERR_SUCCESS != exr_get_file_version_and_flags (*_ctxt, &ver))
    {
        THROW (
            IEX_NAMESPACE::ArgExc, "Unable to get file version from context");
    }

    return static_cast<int> (ver);
}

////////////////////////////////////////

int
Context::partCount () const
{
    int count = 0;

    if (EXR_ERR_SUCCESS != exr_get_count (*_ctxt, &count))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Unable to get part count for file '" << fileName () << "'");
    }

    return count;
}

////////////////////////////////////////

exr_storage_t
Context::storage (int partidx) const
{
    exr_storage_t storage = EXR_STORAGE_LAST_TYPE;

    if (EXR_ERR_SUCCESS != exr_get_storage (*_ctxt, partidx, &storage))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Unable to get storage type for part " << partidx << " in file '"
                                                   << fileName () << "'");
    }

    return storage;
}

////////////////////////////////////////

exr_attr_box2i_t
Context::dataWindow (int partidx) const
{
    exr_attr_box2i_t dw;

    if (EXR_ERR_SUCCESS != exr_get_data_window (*_ctxt, partidx, &dw))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Unable to get the data window for part " << partidx << " in file '"
                                                      << fileName () << "'");
    }

    return dw;
}

////////////////////////////////////////

const exr_attr_chlist_t*
Context::channels (int partidx) const
{
    const exr_attr_chlist_t* cl;

    if (EXR_ERR_SUCCESS != exr_get_channels (*_ctxt, partidx, &cl))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Unable to get the channel list for part "
                << partidx << " in file '" << fileName () << "'");
    }

    return cl;
}

////////////////////////////////////////

bool
Context::hasChannel (int partidx, const char* name) const
{
    return findChannel (partidx, name) != nullptr;
}

const exr_attr_chlist_entry_t* Context::findChannel (int partidx, const char* name) const
{
    const exr_attr_chlist_t* cl  = channels (partidx);
    int32_t                  len = strlen (name);

    for (int i = 0; i < cl->num_channels; ++i)
    {
        const exr_attr_chlist_entry_t* curc = cl->entries + i;
        if (curc->name.length == len && 0 == memcmp (name, curc->name.str, len))
        {
            return curc;
        }
    }
    return nullptr;
}

////////////////////////////////////////

exr_lineorder_t
Context::lineOrder (int partidx) const
{
    exr_lineorder_t lo;

    if (EXR_ERR_SUCCESS != exr_get_lineorder (*_ctxt, partidx, &lo))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Unable to get the line order for part " << partidx << " in file '"
                                                     << fileName () << "'");
    }

    return lo;
}

////////////////////////////////////////

int
Context::attrCount (int partidx) const
{
    int32_t attrcnt = 0;

    if (EXR_ERR_SUCCESS != exr_get_attribute_count (*_ctxt, partidx, &attrcnt))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Unable to get attribute for part " << partidx << " in file '"
                                                << fileName () << "'");
    }

    return 0;
}

////////////////////////////////////////

const exr_attribute_t*
Context::getAttr (int partidx, int attridx) const
{
    const exr_attribute_t* attr = nullptr;

    if (EXR_ERR_SUCCESS !=
        exr_get_attribute_by_index (
            *_ctxt, partidx, EXR_ATTR_LIST_FILE_ORDER, attridx, &attr))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Unable to get attribute index " << attridx << " for part "
                                             << partidx << " in file '"
                                             << fileName () << "'");
    }

    return attr;
}

////////////////////////////////////////

const exr_attribute_t*
Context::getAttr (int partidx, const char* name) const
{
    const exr_attribute_t* attr = nullptr;
    exr_result_t           res;

    res = exr_get_attribute_by_name (*_ctxt, partidx, name, &attr);

    if (res == EXR_ERR_SUCCESS || res == EXR_ERR_NO_ATTR_BY_NAME) return attr;

    THROW (
        IEX_NAMESPACE::ArgExc,
        "Unable to find attribute '" << name << "' for part " << partidx
                                     << " in file '" << fileName () << "'");
}

////////////////////////////////////////

Header
Context::header (int partidx) const
{
    Header                 hdr;
    int32_t                attrcnt = 0;
    const exr_attribute_t* cur     = nullptr;

    if (EXR_ERR_SUCCESS != exr_get_attribute_count (*_ctxt, partidx, &attrcnt))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Unable to get attribute for part " << partidx << " in file '"
                                                << fileName () << "'");
    }

    for (int32_t idx = 0; idx < attrcnt; ++idx)
    {
        if (EXR_ERR_SUCCESS !=
            exr_get_attribute_by_index (
                *_ctxt, partidx, EXR_ATTR_LIST_FILE_ORDER, idx, &cur))
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Unable to get attribute index " << idx << " for part "
                                                 << partidx << " in file '"
                                                 << fileName () << "'");
        }

        switch (cur->type)
        {
            case EXR_ATTR_INT:
                hdr.insert (cur->name, IntAttribute (cur->i));
                break;
            case EXR_ATTR_DOUBLE:
                hdr.insert (cur->name, DoubleAttribute (cur->d));
                break;
            case EXR_ATTR_FLOAT:
                hdr.insert (cur->name, FloatAttribute (cur->f));
                break;

            case EXR_ATTR_BOX2I:
                hdr.insert (
                    cur->name,
                    Box2iAttribute (IMATH_NAMESPACE::Box2i (
                        IMATH_NAMESPACE::V2i (cur->box2i->min),
                        IMATH_NAMESPACE::V2i (cur->box2i->max))));
                break;
            case EXR_ATTR_BOX2F:
                hdr.insert (
                    cur->name,
                    Box2fAttribute (IMATH_NAMESPACE::Box2f (
                        IMATH_NAMESPACE::V2f (cur->box2f->min),
                        IMATH_NAMESPACE::V2f (cur->box2f->max))));
                break;

            case EXR_ATTR_V2I:
                hdr.insert (
                    cur->name,
                    V2iAttribute (IMATH_NAMESPACE::V2i (*(cur->v2i))));
                break;
            case EXR_ATTR_V2F:
                hdr.insert (
                    cur->name,
                    V2fAttribute (IMATH_NAMESPACE::V2f (*(cur->v2f))));
                break;
            case EXR_ATTR_V2D:
                hdr.insert (
                    cur->name,
                    V2dAttribute (IMATH_NAMESPACE::V2d (*(cur->v2d))));
                break;
            case EXR_ATTR_V3I:
                hdr.insert (
                    cur->name,
                    V3iAttribute (IMATH_NAMESPACE::V3i (*(cur->v3i))));
                break;
            case EXR_ATTR_V3F:
                hdr.insert (
                    cur->name,
                    V3fAttribute (IMATH_NAMESPACE::V3f (*(cur->v3f))));
                break;
            case EXR_ATTR_V3D:
                hdr.insert (
                    cur->name,
                    V3dAttribute (IMATH_NAMESPACE::V3d (*(cur->v3d))));
                break;

            case EXR_ATTR_STRING:
                hdr.insert (
                    cur->name,
                    StringAttribute (
                        std::string (cur->string->str, cur->string->length)));
                break;

            case EXR_ATTR_COMPRESSION:
                hdr.insert (
                    cur->name, CompressionAttribute (Compression (cur->uc)));
                break;
            case EXR_ATTR_ENVMAP:
                hdr.insert (cur->name, EnvmapAttribute (Envmap (cur->uc)));
                break;
            case EXR_ATTR_LINEORDER:
                hdr.insert (
                    cur->name, LineOrderAttribute (LineOrder (cur->uc)));
                break;

            case EXR_ATTR_CHLIST: {
                ChannelList& chans = hdr.channels ();

                for (int c = 0; c < cur->chlist->num_channels; ++c)
                {
                    const exr_attr_chlist_entry_t& curc =
                        cur->chlist->entries[c];
                    chans.insert (
                        curc.name.str,
                        Channel (
                            PixelType (curc.pixel_type),
                            curc.x_sampling,
                            curc.y_sampling,
                            curc.p_linear != 0));
                }
                break;
            }

            case EXR_ATTR_TILEDESC:
                hdr.insert (
                    cur->name, TileDescriptionAttribute (
                        TileDescription (
                            cur->tiledesc->x_size,
                            cur->tiledesc->y_size,
                            (LevelMode)(EXR_GET_TILE_LEVEL_MODE (*cur->tiledesc)),
                            (LevelRoundingMode)(EXR_GET_TILE_ROUND_MODE (*cur->tiledesc)))));
                break;

            case EXR_ATTR_FLOAT_VECTOR:
                hdr.insert (
                    cur->name, FloatVectorAttribute (
                        FloatVector (
                            cur->floatvector->arr,
                            cur->floatvector->arr + cur->floatvector->length)));
                break;

            case EXR_ATTR_M33F:
                hdr.insert (
                    cur->name, M33fAttribute (
                        IMATH_NAMESPACE::M33f (
                            *reinterpret_cast<float (*)[3][3]> (cur->m33f->m))));
                break;
            case EXR_ATTR_M33D:
                hdr.insert (
                    cur->name, M33dAttribute (
                        IMATH_NAMESPACE::M33d (
                            *reinterpret_cast<double (*)[3][3]> (cur->m33d->m))));
                break;
            case EXR_ATTR_M44F:
                hdr.insert (
                    cur->name, M44fAttribute (
                        IMATH_NAMESPACE::M44f (
                            *reinterpret_cast<float (*)[4][4]> (cur->m44f->m))));
                break;
            case EXR_ATTR_M44D:
                hdr.insert (
                    cur->name, M44dAttribute (
                        IMATH_NAMESPACE::M44d (
                            *reinterpret_cast<double (*)[4][4]> (cur->m44d->m))));
                break;

            case EXR_ATTR_CHROMATICITIES:
                hdr.insert (
                    cur->name, ChromaticitiesAttribute (
                        Chromaticities (
                            IMATH_NAMESPACE::V2f (
                                cur->chromaticities->red_x,
                                cur->chromaticities->red_y),
                            IMATH_NAMESPACE::V2f (
                                cur->chromaticities->green_x,
                                cur->chromaticities->green_y),
                            IMATH_NAMESPACE::V2f (
                                cur->chromaticities->blue_x,
                                cur->chromaticities->blue_y),
                            IMATH_NAMESPACE::V2f (
                                cur->chromaticities->white_x,
                                cur->chromaticities->white_y))));
                break;
            case EXR_ATTR_KEYCODE:
                hdr.insert (
                    cur->name, KeyCodeAttribute (
                        KeyCode (
                            cur->keycode->film_mfc_code,
                            cur->keycode->film_type,
                            cur->keycode->prefix,
                            cur->keycode->count,
                            cur->keycode->perf_offset,
                            cur->keycode->perfs_per_frame,
                            cur->keycode->perfs_per_count)));
                break;
            case EXR_ATTR_RATIONAL:
                hdr.insert (
                    cur->name, RationalAttribute (
                        Rational (
                            cur->rational->num,
                            cur->rational->denom)));
                break;
            case EXR_ATTR_TIMECODE:
                hdr.insert (
                    cur->name, TimeCodeAttribute (
                        TimeCode (
                            cur->timecode->time_and_flags,
                            cur->timecode->user_data)));
                break;
            case EXR_ATTR_PREVIEW:
                hdr.insert (
                    cur->name, PreviewImageAttribute (
                        PreviewImage (
                            cur->preview->width,
                            cur->preview->height,
                            reinterpret_cast<const PreviewRgba*> (cur->preview->rgba))));
                break;
            case EXR_ATTR_STRING_VECTOR:
            {
                std::vector<std::string> svec;
                svec.resize (cur->stringvector->n_strings);
                for (int s = 0; s < cur->stringvector->n_strings; ++s)
                {
                    svec[s] = std::string (
                        cur->stringvector->strings[s].str,
                        cur->stringvector->strings[s].length);
                }
                hdr.insert (
                    cur->name, StringVectorAttribute (svec));
                break;
            }

            case EXR_ATTR_DEEP_IMAGE_STATE:
                hdr.insert (
                    cur->name, DeepImageStateAttribute (DeepImageState (cur->uc)));
                break;

            case EXR_ATTR_OPAQUE:
                if (Attribute::knownType (cur->type_name))
                {
                    MemAttrStream mas {cur->opaque};

                    std::unique_ptr<Attribute> attr;
                    attr.reset (Attribute::newAttribute (cur->type_name));

                    attr->readValueFrom (mas, cur->opaque->size, version ());
                    // TODO: can we avoid a double copy?
                    hdr.insert (cur->name, *attr);
                }
                else
                {
                    hdr.insert (
                        cur->name, OpaqueAttribute (
                            cur->type_name,
                            cur->opaque->size,
                            cur->opaque->packed_data));
                }
                break;

            case EXR_ATTR_UNKNOWN:
            case EXR_ATTR_LAST_KNOWN_TYPE:
            default: {
                THROW (IEX_NAMESPACE::LogicExc,
                       "Unknown attribute '"
                       << cur->name
                       << "' of type '"
                       << cur->type << "', conversion to legacy header not yet implemented");
            }
        }
    }

    return hdr;
}

////////////////////////////////////////


#define EXR_SET_ATTR(type, cpptype, attrval)                            \
    if (!strcmp (attrT, #type))                                         \
    {                                                                   \
        const cpptype *attr = dynamic_cast<const cpptype*> (&a);        \
        if (!attr)                                                      \
            throw IEX_NAMESPACE::ArgExc ("unexpected type mismatch");   \
        rv = exr_attr_set_##type (*_ctxt, partnum, i.name (), attrval); \
        if (rv != EXR_ERR_SUCCESS)                                      \
            throw IEX_NAMESPACE::ArgExc ("Unable to copy attribute");   \
        continue;                                                       \
    }


void Context::addHeader (int partnum, const Header &h)
{
    exr_result_t rv;
    // TODO: ImfHeader iterator types should support operator* so they can
    // use ranged-based for
    for (auto i = h.begin (); i != h.end (); ++i)
    {
        const Attribute& a = i.attribute ();
        const char* attrT = a.typeName ();

        if (!strcmp (i.name (), "channels"))
        {
            const ChannelList& chans = h.channels ();

            for (auto c = chans.begin (); c != chans.end (); ++c)
            {
                const Channel &cdef = c.channel ();
                rv = exr_add_channel (
                    *_ctxt, partnum, c.name (),
                    (exr_pixel_type_t)cdef.type,
                    cdef.pLinear ? EXR_PERCEPTUALLY_LINEAR : EXR_PERCEPTUALLY_LOGARITHMIC,
                    cdef.xSampling,
                    cdef.ySampling);
                if (rv != EXR_ERR_SUCCESS)
                    throw IEX_NAMESPACE::ArgExc ("Unable to copy channel information");
            }
            continue;
        }

        if (!strcmp (i.name (), "lineOrder"))
        {
            const LineOrderAttribute *attr =
                dynamic_cast<const LineOrderAttribute*> (&a);
            if (!attr)
                throw IEX_NAMESPACE::ArgExc ("unexpected type mismatch");
            rv = exr_set_lineorder (
                *_ctxt, partnum, (exr_lineorder_t)attr->value ());
            if (rv != EXR_ERR_SUCCESS)
                throw IEX_NAMESPACE::ArgExc ("Unable to copy attribute");
            continue;
        }

        if (!strcmp (attrT, "tiledesc"))
        {
            const TileDescriptionAttribute *attr =
                dynamic_cast<const TileDescriptionAttribute*> (&a);
            if (!attr)
                throw IEX_NAMESPACE::ArgExc ("unexpected type mismatch");
            TileDescription td = attr->value ();

            rv = exr_set_tile_descriptor (
                *_ctxt, partnum,
                td.xSize,
                td.ySize,
                (exr_tile_level_mode_t)td.mode,
                (exr_tile_round_mode_t)td.roundingMode);

            if (rv != EXR_ERR_SUCCESS)
                throw IEX_NAMESPACE::ArgExc ("Unable to copy attribute");
            continue;
        }

        EXR_SET_ATTR(string, StringAttribute, attr->value ().c_str ());
        EXR_SET_ATTR(int, IntAttribute, attr->value ());
        EXR_SET_ATTR(float, FloatAttribute, attr->value ());
        EXR_SET_ATTR(double, DoubleAttribute, attr->value ());
        EXR_SET_ATTR(compression, CompressionAttribute, (exr_compression_t)attr->value ());
        EXR_SET_ATTR(envmap, EnvmapAttribute, (exr_envmap_t)attr->value ());
        EXR_SET_ATTR(v2i, V2iAttribute,
                     reinterpret_cast<const exr_attr_v2i_t*>(&(attr->value ())));
        EXR_SET_ATTR(v2f, V2fAttribute,
                     reinterpret_cast<const exr_attr_v2f_t*>(&(attr->value ())));
        EXR_SET_ATTR(v2d, V2dAttribute,
                     reinterpret_cast<const exr_attr_v2d_t*>(&(attr->value ())));
        EXR_SET_ATTR(v3i, V3iAttribute,
                     reinterpret_cast<const exr_attr_v3i_t*>(&(attr->value ())));
        EXR_SET_ATTR(v3f, V3fAttribute,
                     reinterpret_cast<const exr_attr_v3f_t*>(&(attr->value ())));
        EXR_SET_ATTR(v3d, V3dAttribute,
                     reinterpret_cast<const exr_attr_v3d_t*>(&(attr->value ())));
        EXR_SET_ATTR(m33f, M33fAttribute,
                     reinterpret_cast<const exr_attr_m33f_t*>(&(attr->value ())));
        EXR_SET_ATTR(m33d, M33dAttribute,
                     reinterpret_cast<const exr_attr_m33d_t*>(&(attr->value ())));
        EXR_SET_ATTR(m44f, M44fAttribute,
                     reinterpret_cast<const exr_attr_m44f_t*>(&(attr->value ())));
        EXR_SET_ATTR(m44d, M44dAttribute,
                     reinterpret_cast<const exr_attr_m44d_t*>(&(attr->value ())));
        EXR_SET_ATTR(box2i, Box2iAttribute,
                     reinterpret_cast<const exr_attr_box2i_t*>(&(attr->value ())));
        EXR_SET_ATTR(box2f, Box2fAttribute,
                     reinterpret_cast<const exr_attr_box2f_t*>(&(attr->value ())));

        if (!strcmp (attrT, "chromaticities"))
        {
            const ChromaticitiesAttribute *attr =
                dynamic_cast<const ChromaticitiesAttribute*> (&a);
            if (!attr)
                throw IEX_NAMESPACE::ArgExc ("unexpected type mismatch");
            Chromaticities ac = attr->value ();
            exr_attr_chromaticities_t cac;

            cac.red_x = ac.red.x;
            cac.red_y = ac.red.y;
            cac.green_x = ac.green.x;
            cac.green_y = ac.green.y;
            cac.blue_x = ac.blue.x;
            cac.blue_y = ac.blue.y;
            cac.white_x = ac.white.x;
            cac.white_y = ac.white.y;

            rv = exr_attr_set_chromaticities (
                *_ctxt, partnum, i.name (), &cac);

            if (rv != EXR_ERR_SUCCESS)
                throw IEX_NAMESPACE::ArgExc ("Unable to copy attribute");
            continue;
        }

        if (!strcmp (attrT, "stringvector"))
        {
            continue;
        }

        if (!strcmp (attrT, "floatvector"))
        {
            continue;
        }

        if (!strcmp (attrT, "preview"))
        {
            continue;
        }

        if (!strcmp (attrT, "rational"))
        {
            continue;
        }

        if (!strcmp (attrT, "timecode"))
        {
            continue;
        }

        if (!strcmp (attrT, "keycode"))
        {
            continue;
        }

        if (!strcmp (attrT, "deepImageState"))
        {
            continue;
        }

        //THROW (
        //    IEX_NAMESPACE::LogicExc,
        //    "Conversion of attribute '" << i.name () << "' of type '" << attrT
        //    << "' to core value not yet implemented");
    }
}

////////////////////////////////////////

bool
Context::chunkTableValid (int partidx) const
{
    return exr_validate_chunk_table (*_ctxt, partidx) == EXR_ERR_SUCCESS;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
