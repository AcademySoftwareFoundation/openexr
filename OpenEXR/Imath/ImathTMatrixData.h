///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
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


#ifndef INCLUDED_IMATHTMATRIXDATA_H
#define INCLUDED_IMATHTMATRIXDATA_H

#include <ImathTMatrixBase.h>

//-------------------------------------------------------------------------
//
//    TMatrixData -- A generic two-dimensional matrix container class.
// 
//    This is a specialization of Imath::TMatrixBase that uses 
//    storage provided by the client to hold the matrix emelents. The
//    client is thus responsible for ensuring that the given storage is
//    as large as needed. Also, no sotrage deletion is performed by the
//    class destructor: it is up to the clent to ensure that no memory is
//    leaked when the matrix data are no longer needed.
//
//-------------------------------------------------------------------------

namespace Imath
{

template<typename T>
class TMatrixData : public TMatrixBase<T>
{
  public:

    ~TMatrixData();


    //----------------------------------------
    // Matrix of given size, content undefined.
    //----------------------------------------

    TMatrixData(int numRows,
		int numColumns,
		T * data,
		typename TMatrixBase<T>::Order ord =TMatrixBase<T>::ROW_MAJOR);


    //--------------------------------------------------
    // Initialized with a given value upon construction.
    //--------------------------------------------------
    
    TMatrixData(int numRows,
		int numColumns,
		const T & initVal,
		T * data,
		typename TMatrixBase<T>::Order ord =TMatrixBase<T>::ROW_MAJOR);


    //-----------------------------------------------------------------
    // Copy from another a TMatrixData. All copies share the same data.
    //-----------------------------------------------------------------

    TMatrixData(const TMatrixData & m);
    TMatrixData & operator = (const TMatrixData & m);


  private:

    //--------------------------
    // Non default-constructible
    //--------------------------

    TMatrixData();
};

	
//-----------------------
//
//    Implementation.
//
//-----------------------

template<typename T>
inline
TMatrixData<T>::TMatrixData()
{
}

template<typename T>
inline
TMatrixData<T>::~TMatrixData()
{
}

template<typename T>
inline
TMatrixData<T>::TMatrixData(int numRows,
			    int numColumns,
			    T * data,
			    typename TMatrixBase<T>::Order ord)
    : TMatrixBase<T>(numRows, numColumns, ord)
{
    if (! data)
	throw Iex::ArgExc("TMatrixData<T> created passing a null pointer "
			  "to the data");
    setData(data);
}

template<typename T>
inline
TMatrixData<T>::TMatrixData(int numRows,
			    int numColumns,
			    const T & t,
			    T * data,
			    typename TMatrixBase<T>::Order ord)
    : TMatrixBase<T>(numRows, numColumns, ord)
{
    if (! data)
	throw Iex::ArgExc("TMatrixData<T> created passing a null pointer "
			  "to the data");
    setData(data);
    std::fill(data, data + numRows * numColumns, t);
}


// Copy
template<typename T>
inline
TMatrixData<T>::TMatrixData(TMatrixData<T> const & m)
    : TMatrixBase<T>(m)
{
    setData(const_cast<T *>(m.data()));
}

template<typename T>
inline
TMatrixData<T> & TMatrixData<T>::operator = (TMatrixData<T> const & m)
{
    TMatrixBase<T>::operator = (m);
    setData(const_cast<T *>(m.data()));
    return *this;
}

} // namespace Imath

#endif // INCLUDED_IMATHTMATRIXDATA_H 
