///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////



#ifndef INCLUDED_IMF_CHANNEL_LIST_H
#define INCLUDED_IMF_CHANNEL_LIST_H

//-----------------------------------------------------------------------------
//
//	class Channel
//	class ChannelList
//
//-----------------------------------------------------------------------------

#include <ImfName.h>
#include <ImfPixelType.h>
#include <map>


namespace Imf {


struct Channel
{
    //------------------------------
    // Data type; see ImfPixelType.h
    //------------------------------

    PixelType		type;


    //--------------------------------------------
    // Subsampling: pixel (x, y) is present in the
    // channel only if 
    //
    //  x % xSampling == 0 && y % ySampling == 0
    //
    //--------------------------------------------

    int			xSampling;
    int			ySampling;


    //------------
    // Constructor
    //------------
    
    Channel (PixelType type = HALF,
	     int xSampling = 1,
	     int ySampling = 1);


    //------------
    // Operator ==
    //------------

    bool		operator == (const Channel &other) const;
};


class ChannelList
{
  public:

    //--------------
    // Add a channel
    //--------------

    void			insert (const char name[],
					const Channel &channel);

    //------------------------------------------------------------------
    // Access to existing channels:
    //
    // [n]		Returns a reference to the channel with name n.
    //			If no channel with name n exists, an Iex::ArgExc
    //			is thrown.
    //
    // findChannel(n)	Returns a pointer to the channel with name n,
    //			or 0 if no channel with name n exists.
    //
    //------------------------------------------------------------------

    Channel &			operator [] (const char name[]);
    const Channel &		operator [] (const char name[]) const;

    Channel *			findChannel (const char name[]);
    const Channel *		findChannel (const char name[]) const;


    //-------------------------------------------
    // Iterator-style access to existing channels
    //-------------------------------------------

    typedef std::map <Name, Channel> ChannelMap;

    class Iterator;
    class ConstIterator;

    Iterator			begin ();
    ConstIterator		begin () const;
    Iterator			end ();
    ConstIterator		end () const;
    Iterator			find (const char name[]);
    ConstIterator		find (const char name[]) const;

    //------------
    // Operator ==
    //------------

    bool			operator == (const ChannelList &other) const;

  private:

    ChannelMap			_map;
};


//----------
// Iterators
//----------

class ChannelList::Iterator
{
  public:

    Iterator ();
    Iterator (const ChannelList::ChannelMap::iterator &i);

    Iterator &			operator ++ ();
    Iterator 			operator ++ (int);

    const char *		name () const;
    Channel &			channel () const;

  private:

    friend class ChannelList::ConstIterator;

    ChannelList::ChannelMap::iterator _i;
};


class ChannelList::ConstIterator
{
  public:

    ConstIterator ();
    ConstIterator (const ChannelList::ChannelMap::const_iterator &i);
    ConstIterator (const ChannelList::Iterator &other);

    ConstIterator &		operator ++ ();
    ConstIterator 		operator ++ (int);

    const char *		name () const;
    const Channel &		channel () const;

  private:

    friend bool operator == (const ConstIterator &, const ConstIterator &);
    friend bool operator != (const ConstIterator &, const ConstIterator &);

    ChannelList::ChannelMap::const_iterator _i;
};


//-----------------
// Inline Functions
//-----------------

inline
ChannelList::Iterator::Iterator (): _i()
{
    // empty
}


inline
ChannelList::Iterator::Iterator (const ChannelList::ChannelMap::iterator &i):
    _i (i)
{
    // empty
}


inline ChannelList::Iterator &		
ChannelList::Iterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline ChannelList::Iterator 	
ChannelList::Iterator::operator ++ (int)
{
    Iterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
ChannelList::Iterator::name () const
{
    return *_i->first;
}


inline Channel &	
ChannelList::Iterator::channel () const
{
    return _i->second;
}


inline
ChannelList::ConstIterator::ConstIterator (): _i()
{
    // empty
}

inline
ChannelList::ConstIterator::ConstIterator
    (const ChannelList::ChannelMap::const_iterator &i): _i (i)
{
    // empty
}


inline
ChannelList::ConstIterator::ConstIterator (const ChannelList::Iterator &other):
    _i (other._i)
{
    // empty
}

inline ChannelList::ConstIterator &
ChannelList::ConstIterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline ChannelList::ConstIterator 		
ChannelList::ConstIterator::operator ++ (int)
{
    ConstIterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
ChannelList::ConstIterator::name () const
{
    return *_i->first;
}

inline const Channel &	
ChannelList::ConstIterator::channel () const
{
    return _i->second;
}


inline bool
operator == (const ChannelList::ConstIterator &x,
	     const ChannelList::ConstIterator &y)
{
    return x._i == y._i;
}


inline bool
operator != (const ChannelList::ConstIterator &x,
	     const ChannelList::ConstIterator &y)
{
    return !(x == y);
}


} // namespace Imf

#endif
