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


#ifndef INCLUDED_IMATHTMATRIXBASE_H
#define INCLUDED_IMATHTMATRIXBASE_H

#include <IexBaseExc.h>
#include <vector>

//-------------------------------------------------------------------------
//
//    TMatrixBase A generic two-dimensional matrix container class.
//
//    Allows representation of the storage order of the elements in memory,
//    whether row-major (as is usual in C/C++), or column major
//    (as in FORTRAN). This allows for easy interfacing with several
//    FORTRAN-derived math libraries, e.g. Lapack, Minpack, Slatec, etc.
// 
//    Elements are accessed via the usual operator [], as in M[i][j], or
//    via operator (), as in M(i,j). The first form is unckecked for
//    bounds, the second throws an exception for out of bounds access..
// 
//    This template class does hold or manage any storage for the
//    representation of the matrix elements, and is intended to be used
//    as a "pure virtual" base class, providing methods for matrix-like
//    access for concrete implementations in derived classes.
// 
//    See TMatrix and TMatrixData for sample concrete specializations.
// 
//-------------------------------------------------------------------------

namespace Imath
{

template<typename T>	
class TMatrixBase
{
  public:

    //-----------------------------------------------------------------
    // Storage order:
    // In ROW_MAJOR order M(i,j+1) follows M(i,j) in memory, whereas in
    // COLUMN_MAJOR order  M(i+1,j) follows M(i,j).
    // Default order is ROW_MAJOR.
    //-----------------------------------------------------------------

    enum Order { ROW_MAJOR = 0, COLUMN_MAJOR = 1 };

    typedef T         value_type;
    typedef T &       reference;
    typedef const T & const_reference;
	
    virtual ~TMatrixBase();


    //--------------
    // Empty matrix.
    //--------------

    TMatrixBase(Order ord = ROW_MAJOR);


    //----------------------------------------
    // Matrix of given size, content undefined.
    //----------------------------------------

    TMatrixBase(int numRows, int numColumns, Order ord = ROW_MAJOR);


    //------
    // Copy.
    //------

    TMatrixBase(const TMatrixBase & m);
    TMatrixBase & operator = (const TMatrixBase & m);


    //-------------
    // Size lookup.
    //-------------

    int numRows() const;
    int numColumns() const;
    size_t size() const;
    bool empty() const;


    //-----------------------
    // Current storage order.
    //-----------------------

    Order order() const;


    //-----------------------------------------------------------------
    // The row and column strides, i.e. number of places in between two
    // elements respectively in neighboring rows and columns.
    // ----------------------------------------------------------------

    size_t rowStride() const;
    size_t columnStride() const;

    
    //----------------------------------------------------------------
    // Dummy objects returned by operator [], to allow for indexing in
    // row major order regardless of the current storage order. 
    //----------------------------------------------------------------

    class matrix_row 
    {
      private:
	T * _row;
	size_t _colstrd;
    
      public:
	matrix_row(T * row, size_t colstrd)
	    : _row(row), _colstrd(colstrd) {}
	T & operator [] (int col)	{ return *(_row + col * _colstrd); }
    };

    class const_matrix_row 
    {
      private:
	const T * _row;
	size_t _colstrd;

      public:
	const_matrix_row(const T * row, size_t colstrd)
	    : _row(row), _colstrd(colstrd) {}
	const T & operator [] (int col){return *(_row+col*_colstrd);}
    };


    //-------------------------------------------------------------
    // Element access in row major order as M[k][h]. This works as
    // expected in C++ semantics regardless of the current storage
    // order. No bound checking is performed on the passed indexes.
    //-------------------------------------------------------------

    matrix_row       operator [] (int row);
    const_matrix_row operator [] (int row) const;


    //----------------------------------------------------------------
    // Checked element access as M(row,col). Works in both order modes
    // and throws Iex::ArgExc if the passed indexes are out of bounds.
    //----------------------------------------------------------------

    T & operator () (int row, int col);
    const T & operator () (int row, int col) const;


    //-----------------------------------------------
    // Pointer to the raw data, use at your own risk.
    //-----------------------------------------------

    T * data();
    const T * data() const;


    //--------------------------------------------------------------------
    // TMatrixBase iterators. They are just pointers into the data,
    // and iterate along the rows if storage order is ROW_MAJOR,
    // otherwise along the columns.  
    //--------------------------------------------------------------------
    
    typedef T * iterator;
    typedef const T * const_iterator;


    //----------------------------------------------------------------
    // Iterators pointing to the begin and end of storage.
    //----------------------------------------------------------------
    
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;


    //---------------------------------------------------------------
    // Transposition in-place. The storage order is not changed. This
    // method invalidates all currently defined iterators. 
    //---------------------------------------------------------------

    void transpose();


    //-------------------------------------------------------------
    // Order conversion in-place. This is a null operation is the
    // argument matches the current order, and actually re-sorts the
    // data in memory if not. This method invalidates all currently
    // defined iterators.  
    //-------------------------------------------------------------

    void setOrder(typename TMatrixBase<T>::Order ord);
	

  protected:

    void setData(T * p) const;
    void setSize(int numRows, int numColumns) const;
    void setStride(size_t rowStride, size_t columnStride) const;


    //------------------------------------------------------------------------
    // Utility non-member function to perform the actual data transposition.
    //------------------------------------------------------------------------

    template<typename U>
	friend void	transposeData(const U * odata,
				      int nr, int nc,
				      typename TMatrixBase<U>::Order ord,
				      U * data);


  private:
    
    //------------------------------------------------------------------
    // Pointer to the beginning of the matrix element storage. Must be
    //------------------------------------------------------------------
    // set appropriately by a derived class.

    mutable T * _dataP;
    mutable int _nrows, _ncols;
    mutable size_t _rowStride, _colStride;
};
 
} // namespace Imath


//-------------------------------------------------------------------------
// 
//                             IMPLEMENTATION
// 
//-------------------------------------------------------------------------

namespace Imath
{
	
#include <assert.h>


// Protected accessors
template<typename T>
inline void
TMatrixBase<T>::setData(T * p) const
{
    _dataP = p;
}

template<typename T>
inline void
TMatrixBase<T>::setStride(size_t rowStride, size_t columnStride) const
{
    assert(rowStride == 1 || columnStride == 1);
    
    _rowStride = rowStride;
    _colStride = columnStride;
}

template<typename T>
inline void
TMatrixBase<T>::setSize(int numRows, int numColumns) const
{
    _nrows = numRows;
    _ncols = numColumns;
}

// Constructors, destructors
template<typename T>
inline
TMatrixBase<T>::~TMatrixBase()
{
}

template<typename T>
inline
TMatrixBase<T>::TMatrixBase(Order ord)
{
    setData(0);
    setSize(0, 0);
    if (ord == ROW_MAJOR)
	setStride(0, 1);
    else
	setStride(1, 0);
}

template<typename T>
inline
TMatrixBase<T>::TMatrixBase(int numRows, int numColumns, Order ord)
{
    setData(0);
    setSize(numRows, numColumns);
    if (ord == ROW_MAJOR)
	setStride(numColumns, 1);
    else
	setStride(1, numRows);
}


// Size lookup
template<typename T>
inline int
TMatrixBase<T>::numRows() const
{
    return _nrows;
}

template<typename T>
inline int
TMatrixBase<T>::numColumns() const
{
    return _ncols;
}

template<typename T>
inline size_t
TMatrixBase<T>::size() const
{
    return size_t (_nrows) * size_t (_ncols);
}

template<typename T>
inline bool
TMatrixBase<T>::empty() const
{
    return ! _dataP;
}

// Stride lookup
template<typename T>
inline size_t
TMatrixBase<T>::rowStride() const
{
    return _rowStride;
}

template<typename T>
inline size_t
TMatrixBase<T>::columnStride() const
{
    return _colStride;
}

// Order lookup
template<typename T>
inline typename TMatrixBase<T>::Order
TMatrixBase<T>::order() const
{
    return columnStride() == 1 ? ROW_MAJOR : COLUMN_MAJOR;
}

// Data access
template<typename T>
inline T *
TMatrixBase<T>::data()
{
    return _dataP;
}

template<typename T>
inline const T *
TMatrixBase<T>::data() const
{
    return _dataP;
}

//-----------------------------------------------------------------
// Copy. Note carefully: derived classes must provide appropriate
// semantics for member _dataP duplication, depending on how they
// manage matrix element storage.
//-----------------------------------------------------------------

template<typename T>
inline
TMatrixBase<T>::TMatrixBase(const TMatrixBase<T> & m)
    : _nrows(m._nrows), _ncols(m._ncols),
    _rowStride(m._rowStride), _colStride(m._colStride)
{}

template<typename T>
inline TMatrixBase<T> & 
TMatrixBase<T>::operator = (const TMatrixBase<T> & m)
{
    _nrows = m._nrows;
    _ncols = m._ncols;
    _rowStride = m._rowStride;
    _colStride = m._colStride;

    return *this;
}

// MatrixBase's operator []
template<typename T>
inline typename TMatrixBase<T>::matrix_row
TMatrixBase<T>::operator [] (int row)
{
    return matrix_row(data() + row * rowStride(), columnStride());
}

template<typename T>
inline typename TMatrixBase<T>::const_matrix_row
TMatrixBase<T>::operator [] (int row) const
{
    return const_matrix_row(data() + row * rowStride(), columnStride());
}

// Checked access as M(i,j).
template<typename T>
inline T &
TMatrixBase<T>::operator () (int row, int col)
{
    if (empty() || row < 0 || row >= numRows() ||
	col < 0 || col >= numColumns())
	throw Iex::ArgExc("Attempted access to TMatrixBase element "
			  "via operator () with indexes out of range");

    return *(data() + row * rowStride() + col * columnStride());
}

template<typename T>
inline const T &
TMatrixBase<T>::operator () (int row, int col) const
{
    if (empty() || row < 0 || row >= numRows() ||
	col < 0 || col >= numColumns())
	throw Iex::ArgExc("Attempted access to TMatrixBase element "
			  "via operator () with indexes out of range");

    return *(data() + row * rowStride() + col * columnStride());
}

// Iterators
template<typename T>
inline typename TMatrixBase<T>::iterator
TMatrixBase<T>::begin()
{
    return data();
}

template<typename T>
inline typename TMatrixBase<T>::iterator
TMatrixBase<T>::end()
{
    return data() + size();
}

template<typename T>
inline typename TMatrixBase<T>::const_iterator
TMatrixBase<T>::begin() const
{
    return data();
}

template<typename T>
inline typename TMatrixBase<T>::const_iterator
TMatrixBase<T>::end() const
{
    return data() + size();
}

// Transposition

// This nonmember, does the actual job.
template<typename U>
void
transposeData(const U * odata, int nr, int nc,
	      typename TMatrixBase<U>::Order ord, U * ndata)
{
    // FIXME - this should be done in-place, using an auxiliary bit
    // array to store the swaps. For now we just copy.

    int stride, nreps;
    if (ord == TMatrixBase<U>::ROW_MAJOR) 
    {
	stride = nr; 
	nreps = nc;
    }
    else
    {
	stride = nc; 
	nreps = nr;
    }

    U * nd = ndata;
    const U * it = odata;
    const U * ie = it + nr*nc;
	
    while (it != ie) 
    {
	U * nnd = nd;
	for (int k = nreps; k; --k) 
	{
	    *nnd = *it++;
	    nnd += stride;
	}
	++nd;
    }
}

template<typename T>
void
TMatrixBase<T>::transpose()	
{
    if (empty())
	return;

    std::vector<T> odata(data(), data() + size());
    transposeData(&(odata.front()), numRows(), numColumns(), order(),  data());
    setSize(numColumns(), numRows());
    if (order() == TMatrixBase<T>::ROW_MAJOR)
	setStride(numColumns(), 1);
    else
	setStride(1, numRows());
}

// Order change
template<typename T>
void
TMatrixBase<T>::setOrder(typename TMatrixBase<T>::Order ord)
{
    if (order() == ord)
	return;
	
    if (! empty())
    {
	std::vector<T> odata(data(), data() + size());	
	transposeData(&(odata.front()), numRows(), numColumns(),
		      order(), data());
    }
	
    if (order() == TMatrixBase<T>::ROW_MAJOR)
	setStride(1, numRows());
    else
	setStride(numColumns(), 1);
}

} // namespace Imath


#endif // INCLUDED_IMATHTMATRIX_H 
