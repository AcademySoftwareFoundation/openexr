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


#ifndef INCLUDED_IMATHTMATRIX_H
#define INCLUDED_IMATHTMATRIX_H

#include <ImathTMatrixBase.h>
#include <ImathMatrix.h>
#include <vector>
#include <algorithm>
#include <iterator>

//-------------------------------------------------------------------------
//
//    TMatrix -- A generic two-dimensional matrix container class.
// 
//    This is a specialization of Imath::TMatrixBase that uses a
//    std::vector to manage the storage of matrix elements.
//
//    Additional methods are provided for clearing, resizing and changing
//    the storage order. It also provides all the iterators of
//    std::vector. They iterate  in row-major order if the storage mode
//    is "C", otherwise in column-major order.
//
//-------------------------------------------------------------------------

namespace Imath
{

template<typename T>
class TMatrix : public TMatrixBase<T>
{
  public:
    using TMatrixBase<T>::order;
    using TMatrixBase<T>::data;
    using TMatrixBase<T>::numColumns;
    using TMatrixBase<T>::numRows;

    ~TMatrix();


    //-------------
    // Empty matrix
    //-------------

    TMatrix(typename TMatrixBase<T>::Order ord = TMatrixBase<T>::ROW_MAJOR);


    //----------------------------------------
    // Matrix of given size, content undefined.
    //----------------------------------------

    TMatrix(int numRows, int numColumns,
	    typename TMatrixBase<T>::Order ord = TMatrixBase<T>::ROW_MAJOR);


    //--------------------------------
    // Initialized with a given value.
    //--------------------------------

    TMatrix(int numRows, int numColumns, const T & initVal,
	    typename TMatrixBase<T>::Order ord = TMatrixBase<T>::ROW_MAJOR);


    //------------------------------------
    // Initialized with Matrix33, Matrix44.
    //------------------------------------

    explicit TMatrix(const Matrix33<T> & m33,
		     typename TMatrixBase<T>::Order ord = TMatrixBase<T>::ROW_MAJOR);
    explicit TMatrix(const Matrix44<T> & m44,
		     typename TMatrixBase<T>::Order ord = TMatrixBase<T>::ROW_MAJOR);


    //--------------------------------------------------------------
    // Conversion to the above. They return respectively the
    // upper-left 3x3 and 4x4 quadrants of the TMatrix<T>, and throw
    // Iex::ArgExc if the size of the TMatrix is smaller than needed.
    //--------------------------------------------------------------

    Matrix33<T> asMatrix33() const;
    Matrix44<T> asMatrix44() const;
	

    //------------------------------------------------------------------
    // Identity and null matrices: unit is defined as T(1), null as
    // T(0): such T contructors must be defined when these method are
    // specialized.
    //------------------------------------------------------------------	
    static TMatrix<T> identity(int siz,
			       typename TMatrixBase<T>::Order ord = TMatrixBase<T>::ROW_MAJOR);
    static TMatrix<T> null(int numRows, int numColumns,
			   typename TMatrixBase<T>::Order ord = TMatrixBase<T>::ROW_MAJOR);


    //---------------------------------------------------------------
    // Copy, from TMatrix and (generically) from another TMatrixBase.
    //---------------------------------------------------------------

    TMatrix(const TMatrix & m);
    TMatrix & operator = (const TMatrix & m);

    TMatrix(const TMatrixBase<T> & m);
    TMatrix & operator = (const TMatrixBase<T> & m);


    //-------------------------------------
    // Size lookup (overload parent's).
    //-------------------------------------

    size_t size() const;
    bool empty() const;


    //---------------------------------------------------------------------
    // Delete all content and zero the size. Does not change the currently
    // selected storage order. This method invalidates all currently
    // defined TMatrix iterators. 
    //---------------------------------------------------------------------

    void clear();
	

    //------------------------------------------------------------------
    // Resize method. Saves previous content within size intersection if
    // any. Newly allocated memory is filled with the given value.
    // This method invalidates all currently defined TMatrix iterators. 
    //------------------------------------------------------------------

    void resize(int newNumRows, int newNumColumns, T value = T());


    //-----------------------------------------------------------
    // Transposition out-of-place. The result matrix has the same
    // storage order as the current one.
    //-----------------------------------------------------------

    TMatrix<T> transposed() const;


    //----------------------------------------------------------------
    // TMatrix iterators. They iterate along the rows if storage order
    // is ROW_MAJOR, otherwise along the columns. Overloaded
    // w.r.t. those in the base class.
    //----------------------------------------------------------------
	
    typedef typename std::vector<T>::iterator                iterator;
    typedef typename std::vector<T>::const_iterator          const_iterator;
    typedef typename std::vector<T>::reverse_iterator        reverse_iterator;
    typedef typename std::vector<T>::const_reverse_iterator  const_reverse_iterator;


    //----------------------------------------------------------------
    // Iterators pointing to the begin and end of storage, forward and
    // reverse. Overloaded w.r.t. those in the base class.
    //----------------------------------------------------------------

    iterator        begin();
    iterator        end();
    const_iterator  begin() const;
    const_iterator  end() const;

    reverse_iterator        rbegin();
    reverse_iterator        rend();
    const_reverse_iterator  rbegin() const;
    const_reverse_iterator  rend() const;
    
  protected:
    using TMatrixBase<T>::setData;
    using TMatrixBase<T>::setSize;
    using TMatrixBase<T>::setStride;
    
  private:
    mutable std::vector<T> _data;
};

	
//------------------------------------------------------------------------
//
// IMPLEMENTATION
//
//------------------------------------------------------------------------

// Clear
template<typename T>
inline void
TMatrix<T>::clear() 
{
    _data.clear();
    TMatrixBase<T>::setData(0);
    setSize(0, 0);
    if (order() == TMatrixBase<T>::ROW_MAJOR)
	setStride(0, 1);
    else
	setStride(1, 0);
}

template<typename T>
inline
TMatrix<T>::~TMatrix()
{
}

template<typename T>
inline
TMatrix<T>::TMatrix(typename TMatrixBase<T>::Order ord)
    : TMatrixBase<T>(ord)
{
}

template<typename T>
inline
TMatrix<T>::TMatrix(int numRows, int numColumns,
		    typename TMatrixBase<T>::Order ord)
    : TMatrixBase<T>(numRows, numColumns, ord),
      _data(numRows * numColumns)
{
    setData(&(_data.front()));
}

template<typename T>
inline
TMatrix<T>::TMatrix(int numRows, int numColumns, const T & t,
		    typename TMatrixBase<T>::Order ord)
    : TMatrixBase<T>(numRows, numColumns, ord),
      _data(numRows * numColumns, t)
{
    setData(&(_data.front()));
}

// Overloaded w.r.t. parent's
template<typename T>
inline size_t
TMatrix<T>::size() const
{
    return _data.size();
}

// Overloaded w.r.t. parent's
template<typename T>
inline bool
TMatrix<T>::empty() const
{
    return _data.empty();
}

// Copy
template<typename T>
inline
TMatrix<T>::TMatrix(TMatrixBase<T> const & m)
    : TMatrixBase<T>(m), _data(m.begin(), m.end())
{
    setData(&(_data.front()));
}

template<typename T>
inline TMatrix<T> &
TMatrix<T>::operator = (TMatrixBase<T> const & m)
{
    TMatrixBase<T>::operator = (m);
    _data.resize(m.size());
    std::copy(m.begin(), m.end(), _data.begin());
    setData(&(_data.front()));
    return *this;
}

template<typename T>
inline
TMatrix<T>::TMatrix(TMatrix<T> const & m)
    : TMatrixBase<T>(m), _data(m._data)
{
    setData(&(_data.front()));
}

template<typename T>
inline TMatrix<T> &
TMatrix<T>::operator = (TMatrix<T> const & m)
{
    TMatrixBase<T>::operator = (m);
    _data = m._data;
    setData(&(_data.front()));
    return *this;
}


// Resize
template<typename T>
void
TMatrix<T>::resize(int nRows, int nColumns, T val)
{
    // Special cases
    if (nRows == numRows() && nColumns == numColumns())
	return;

    if (nRows == 0 || nColumns == 0)
    {
	clear();
	return;
    }
	
    if (empty())
    {
	_data.resize(nRows * nColumns);
	std::fill(_data.begin(), _data.end(), val);
	setData(&(_data.front()));
	setSize(nRows, nColumns);
	if (order() == TMatrixBase<T>::ROW_MAJOR)
	    setStride(nColumns, 1);
	else
	    setStride(1, nRows);
	return;
    } 
	
    // Actual resize
    std::vector<T> odata(_data);
    _data.resize(nRows * nColumns);
    std::fill(_data.begin(), _data.end(), val);
	
    typename std::vector<T>::iterator n = _data.begin();
    typename std::vector<T>::iterator ob = odata.begin();
    size_t sz = std::min(_data.size(), odata.size());
    if (order() == TMatrixBase<T>::ROW_MAJOR) 
    {
	size_t cnt = std::min(nColumns, numColumns());
	typename std::vector<T>::iterator oe = ob;
	std::advance(oe, cnt);
	for (size_t s = 0; s < sz; s += cnt)
	{
	    std::copy(ob, oe, n);
	    std::advance(n, nColumns);
	    std::advance(ob, numColumns());
	    std::advance(oe, numColumns());
	}
	
	setStride(nColumns, 1);
    }
    else
    {
	size_t cnt = std::min(nRows, numRows());
	typename std::vector<T>::iterator oe = ob;
	std::advance(oe, cnt);
	for (size_t s = 0; s < sz; s += cnt)
	{
	    std::copy(ob, oe, n);
	    std::advance(n, nRows);
	    std::advance(ob, numRows());
	    std::advance(oe, numRows());
	}
	setStride(1, nRows);
    }

    setSize(nRows, nColumns);
    setData(&(_data.front()));
}

// Iterators
template<typename T>
inline typename TMatrix<T>::iterator
TMatrix<T>::begin()
{
    return _data.begin();
}

template<typename T>
inline typename TMatrix<T>::iterator
TMatrix<T>::end()
{
    return _data.end();
}

template<typename T>
inline typename TMatrix<T>::const_iterator
TMatrix<T>::begin() const
{
    return _data.begin();
}

template<typename T>
inline typename TMatrix<T>::const_iterator
TMatrix<T>::end() const
{
    return _data.end();
}

template<typename T>
inline typename TMatrix<T>::reverse_iterator
TMatrix<T>::rbegin()
{
    return _data.rbegin();
}

template<typename T>
inline typename TMatrix<T>::reverse_iterator
TMatrix<T>::rend()
{
    return _data.rend();
}

template<typename T>
inline typename TMatrix<T>::const_reverse_iterator
TMatrix<T>::rbegin() const
{
    return _data.rbegin();
}

template<typename T>
inline typename TMatrix<T>::const_reverse_iterator
TMatrix<T>::rend() const
{
    return _data.rend();
}

// Transposition

template<typename T>
TMatrix<T>
TMatrix<T>::transposed() const
{
    TMatrix<T> tra(numColumns(), numRows(), order());
    if (! empty())
	transposeData(data(),numRows(),numColumns(),order(),tra.data());
    return tra;
}


// Special matrices
template<typename T>
TMatrix<T>
TMatrix<T>::identity(int siz, typename TMatrixBase<T>::Order ord)
{
    TMatrix<T> id(siz, siz, T(0), ord);
    typename TMatrix<T>::iterator it = id.begin(), ie = id.end();
    const T unit = T(1);
    while (it < ie)
    {
	*it = unit;
	std::advance(it, siz + 1);
    }

    return id;
}

template<typename T>
TMatrix<T>
TMatrix<T>::null(int numRows, int numColumns,
		 typename TMatrixBase<T>::Order ord)
{
    return TMatrix<T>(numRows, numColumns, T(0), ord);
}


// Conversions
template<typename T>
TMatrix<T>::TMatrix(const Matrix33<T> & m33,
		    typename TMatrixBase<T>::Order ord)
    : TMatrixBase<T>(3, 3, ord), _data(9)
{
    typename std::vector<T>::iterator it = _data.begin();
    if (ord == TMatrixBase<T>::ROW_MAJOR)
    {
	for (int i = 0; i < 3; ++i)
	    for (int j = 0; j < 3; ++j)
		*it++ = m33[i][j];
    }
    else
    {
	for (int j = 0; j < 3; ++j)
	    for (int i = 0; i < 3; ++i)
		*it++ = m33[i][j];
    }

    setData(&(_data.front()));
}

template<typename T>
Matrix33<T>
TMatrix<T>::asMatrix33() const
{
    if (numRows() < 3 || numColumns() < 3)
	throw Iex::ArgExc("Conversion of TMatrix<T> to Matrix33 attempted, "
			  "but TMatrix<T> has less than 3 rows and/or "
			  "3 columns.");

    Matrix33<T> m33;
    const TMatrix<T> & M = *this;
    
    for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j)
	    m33[i][j] = M[i][j];

    return m33;
}

template<typename T>
TMatrix<T>::TMatrix(const Matrix44<T> & m44,
		    typename TMatrixBase<T>::Order ord)
    : TMatrixBase<T>(4, 4, ord), _data(16)
{
    typename std::vector<T>::iterator it = _data.begin();
    if (ord == TMatrixBase<T>::ROW_MAJOR)
    {
	for (int i = 0; i < 4; ++i)
	    for (int j = 0; j < 4; ++j)
		*it++ = m44[i][j];
    }
    else
    {
	for (int j = 0; j < 4; ++j)
	    for (int i = 0; i < 4; ++i)
		*it++ = m44[i][j];
    }

    setData(&(_data.front()));
}

template<typename T>
Matrix44<T>
TMatrix<T>::asMatrix44() const
{
    if (numRows() < 4 || numColumns() < 4)
	throw Iex::ArgExc("Conversion of TMatrix<T> to Matrix44 attempted, "
			  "but TMatrix<T> has less than 4 rows and/or "
			  "4 columns.");

    Matrix44<T> m44;
    const TMatrix<T> & M = *this;
    
    for (int i = 0; i < 4; ++i)
	for (int j = 0; j < 4; ++j)
	    m44[i][j] = M[i][j];

    return m44;
}

} // namespace Imath

#endif // INCLUDED_IMATHTMATRIX_H 
