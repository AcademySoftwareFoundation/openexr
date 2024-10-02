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

/// \brief Context provides a wrapper around the Core library context
/// object
///
/// This is the main vehicle by which the core library provides
/// concurrent behavior, avoiding globals, and allowing each part of
/// any application to use its own custom allocators or any other
/// feature they prefer.
///
/// The context is logically comprised of referencing a file plus the
/// additional helper utilities needed for doing so, and so should be
/// the main entrypoint for querying any of the metadata for all the
/// component parts of the file, along with the global header
/// information.
class IMF_EXPORT_TYPE Context
{
public:
    struct read_mode_t { explicit read_mode_t() = default; };
    struct temp_mode_t { explicit temp_mode_t() = default; };
    struct write_mode_t { explicit write_mode_t() = default; };

    Context ();

    Context (const char* filename,
             const ContextInitializer& ctxtinit,
             read_mode_t t);

    Context (const char* filename,
             const ContextInitializer& ctxtinit,
             temp_mode_t t);

    Context (const char* filename,
             const ContextInitializer& ctxtinit,
             write_mode_t t);

    operator exr_context_t () const noexcept { return *(_ctxt); }

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
    IMF_EXPORT const exr_attr_chlist_entry_t* findChannel (int partidx, const char* name) const;

    IMF_EXPORT exr_lineorder_t lineOrder (int partidx) const;

    // access to generic attributes

    IMF_EXPORT int attrCount (int partidx) const;

    IMF_EXPORT const exr_attribute_t* getAttr (int partidx, int attridx) const;
    IMF_EXPORT const exr_attribute_t*
    getAttr (int partidx, const char* name) const;

    // C++ header interface support

    IMF_EXPORT Header header (int partnum) const;
    IMF_EXPORT void addHeader (int partnum, const Header &h);

    // validation and things

    IMF_EXPORT bool chunkTableValid (int partidx) const;

private:
    std::shared_ptr<exr_context_t> _ctxt;
}; // class Context

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif // INCLUDED_IMF_CONTEXT_H
