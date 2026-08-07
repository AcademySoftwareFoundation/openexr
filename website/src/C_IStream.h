//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project
//

#ifndef INCLUDED_C_ISTREAM_H
#define INCLUDED_C_ISTREAM_H

#include <cstdint>
#include <cstdio>

#include <ImfIO.h>

class C_IStream : public OPENEXR_IMF_NAMESPACE::IStream
{
  public:

    C_IStream (FILE* file, const char fileName[]) :
        IStream (fileName), _file (file)
    {
    }

    virtual bool     read (char c[], int n);
    virtual uint64_t tellg ();
    virtual void     seekg (uint64_t pos);
    virtual void     clear ();

  private:

    FILE* _file;
};

#endif
