//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_CONTEXT_H
#define INCLUDED_IMF_CONTEXT_H

#include "ImfContextInit.h"

#include "ImfHeader.h"

#include <memory>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

/// @brief Context provides
class IMF_EXPORT_TYPE Context
{
public:
    Context ();

    operator exr_context_t () const noexcept { return *(_ctxt); }

    IMF_EXPORT void
    startRead (const char* filename, const ContextInitializer& ctxtinit);
    IMF_EXPORT void
    startWrite (const char* filename, const ContextInitializer& ctxtinit);

    IMF_EXPORT void setLongNameSupport (bool onoff);

    // generic file values

    IMF_EXPORT const char* fileName () const;

    IMF_EXPORT int version () const;

    IMF_EXPORT int partCount () const;

    IMF_EXPORT exr_storage_t storage (int partidx) const;

    // access to commonly used attributes

    IMF_EXPORT exr_attr_box2i_t dataWindow (int partidx) const;

    IMF_EXPORT const exr_attr_chlist_t* channels (int partidx) const;
    IMF_EXPORT bool hasChannel (int partidx, const char* name) const;

    IMF_EXPORT exr_lineorder_t lineOrder (int partidx) const;

    // access to generic attributes

    IMF_EXPORT int attrCount (int partidx) const;

    IMF_EXPORT const exr_attribute_t* getAttr (int partidx, int attridx) const;
    IMF_EXPORT const exr_attribute_t*
    getAttr (int partidx, const char* name) const;

    // validation and legacy things

    IMF_EXPORT bool chunkTableValid (int partidx) const;

    IMF_EXPORT Header header (int partnum) const;

    // TODO: remove once the rest has been ported
    IMF_EXPORT IStream* legacyIStream (int partnum) const;

private:
    std::shared_ptr<exr_context_t> _ctxt;

    // TODO: remove both these
    std::shared_ptr<IStream> _legacy;
    IStream*                 _prov_stream = nullptr;
}; // class Context

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif // INCLUDED_IMF_CONTEXT_H
