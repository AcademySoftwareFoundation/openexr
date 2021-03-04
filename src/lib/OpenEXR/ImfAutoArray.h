//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_AUTO_ARRAY_H
#define INCLUDED_IMF_AUTO_ARRAY_H

//-----------------------------------------------------------------------------
//
//	class AutoArray -- a workaround for systems with
//	insufficient stack space for large auto arrays.
//
//-----------------------------------------------------------------------------

#include "ImfNamespace.h"
#include <string.h>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


#if !defined (OPENEXR_HAVE_LARGE_STACK)


    template <class T, int size>
    class AutoArray
    {
      public:

	 AutoArray (): _data (new T [size]) { memset(_data, 0, size*sizeof(T)); }
	~AutoArray () {delete [] _data;}

        AutoArray (const AutoArray& other) = delete;
        AutoArray& operator = (const AutoArray& other) = delete;
        AutoArray (AutoArray&& other) = delete;
        AutoArray& operator = (AutoArray&& other) = delete;
        
	operator T * ()			{return _data;}
	operator const T * () const	{return _data;}
      
      private:

	T *_data;
    };


#else


    template <class T, int size>
    class AutoArray
    {
      public:

	operator T * ()			{return _data;}
	operator const T * () const	{return _data;}
      
      private:

	T _data[size];
    };


#endif

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


#endif
