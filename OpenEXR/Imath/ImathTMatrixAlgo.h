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


#ifndef INCLUDED_IMATHTMATRIXALGO_H
#define INCLUDED_IMATHTMATRIXALGO_H

#include <IexBaseExc.h>
#include <ImathTMatrixBase.h>
#include <ImathTMatrix.h>
#include <algorithm>
#include <functional>

//-----------------------------------------------------------
//
//    Algorithms operating on TMatrix<T> objects.
//
//-----------------------------------------------------------

namespace Imath
{

//-------------------------------------------------------------------------
//
//    Basic matrix algebra on TMatrix objects
//
//    In comments below, uppercase letters denote matrices, lowercase
//    letters denote scalars.
//
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
//
//    TMatrix<T> OP scalar .
//    These operations do not change the storage order of the TMatrix.
//
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
// B = A + a. The result is returned in the same storage order as A.
//-------------------------------------------------------------------------
template<typename T>
TMatrix<T> operator + (const TMatrixBase<T> & A, const T & a);

//--------------
// A += a	
//--------------
template<typename T>
TMatrixBase<T> & operator += (TMatrixBase<T> & A, const T & a);

//-------------------------------------------------------------------------
// B = A * a. The result is returned in the same storage order as A.	
//-------------------------------------------------------------------------
template<typename T>
TMatrix<T> operator * (const TMatrixBase<T> & A, const T & a);


//--------------
// A *= a	
//--------------
template<typename T>
TMatrixBase<T> & operator *= (TMatrixBase<T> & A, const T & a);
	
//-------------------------------------------------------------------------
//
//    TMatrix<T> OP TMatrix<T>.
//
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
// B = -A. Does not change storage order.  The result is returned in
// the same storage order as A.	 
//-------------------------------------------------------------------------
template<typename T>
TMatrix<T> operator - (const TMatrixBase<T> & A);

//-------------------------------------------------------------------------
// B = -A. Does not change storage order. Throws Iex::ArgExc if the
// sizes of A and MinusA do not match. The storage order of B is left unchanged.
//-------------------------------------------------------------------------
template<typename T>
void opposite(const TMatrixBase<T> & A, TMatrixBase<T> & MinusA);

//---------------------------------------------------------------
// A += B. Throws Iex::ArgExc if B is not the same size as A. The
// storage order of A is unchanged regardless of B's.
//---------------------------------------------------------------
template<typename T>
TMatrixBase<T> & operator += (TMatrixBase<T> & A, const TMatrixBase<T> & B);

//-------------------------------------------------------------------
// C = A + B. Throws Iex::ArgExc if B is not the same size as A. C is
// returned in the same storage order as A.
//-------------------------------------------------------------------	
template<typename T>
TMatrix<T> operator + (const TMatrixBase<T> & A, const TMatrixBase<T> & B);

//-------------------------------------------------------------------	
// add(A, B, C). Throws Iex::Argexc if the size of A, B and C do not
// match. The storage order of C is left unchanged. 
//-------------------------------------------------------------------	
template<typename T>
void add(const TMatrixBase<T> & A, const TMatrixBase<T> & B, TMatrixBase<T> & C);

//-------------------------------------------------------------------	
// A -= B. Throws Iex::ArgExc if B is not the same size as A. The
// storage order of A is unchanged regardless of B's.
//-------------------------------------------------------------------	
template<typename T>
TMatrixBase<T> & operator -= (TMatrixBase<T> & A, const TMatrixBase<T> & B);

//-------------------------------------------------------------------	
// C = A - B. Throws Iex::ArgExc if B is not the same size as A. C is
// returned in the same storage order as A.
//-------------------------------------------------------------------	
template<typename T>
TMatrix<T> operator - (const TMatrixBase<T> & A, const TMatrixBase<T> & B);

//-------------------------------------------------------------------	
// subtract(A, B, C). Throws Iex::Argexc if the size of A, B and C do not
// match. The storage order of C is left unchanged. 
//-------------------------------------------------------------------	
template<typename T>
void subtract(const TMatrixBase<T> &A, const TMatrixBase<T> &B, TMatrixBase<T> &C);

//-------------------------------------------------------------------	
// C = A * B. Throws Iex::ArgExc if B's number of rows is not the same
// as A' number of columns. C is returned in the same storage order as
// A.
//-------------------------------------------------------------------
template<typename T>
TMatrix<T> operator * (const TMatrixBase<T> & A, const TMatrixBase<T> & B);

//-------------------------------------------------------------------	
// mult(A, B, C). Throws Iex::ArgExc if the size of A, B and C are
// not conformant. The storage order of C is left unchanged. 
//-------------------------------------------------------------------	
template<typename T>
void mult(const TMatrixBase<T> & A, const TMatrixBase<T> & B, TMatrixBase<T> & C);

//-------------------------------------------------------------------	
// C = A .* B (a.k.a. memberwise multiplication). Throws Iex::ArgExc
// if the sizes of A and B do not match. C is returned in the same
// storage order as A.
//-------------------------------------------------------------------
template<typename T>
TMatrix<T> operator % (const TMatrixBase<T> & A, const TMatrixBase<T> & B);

//-------------------------------------------------------------------	
// dotmult(A, B, C). Throws Iex::ArgExc if the sizes of A, B and C do
// not match. The storage order of C is left unchanged. 
//-------------------------------------------------------------------	
template<typename T>
void dotmult(const TMatrixBase<T> & A, const TMatrixBase<T> & B, TMatrixBase<T> & C);


//-----------------------------------------------
//
//    Implementation
//
//-----------------------------------------------

// B = A + a	
template<typename T>
struct scalSum_
{
    const T & _a;
    scalSum_(const T & a) : _a(a) {}
    T operator () (const T & b) { return _a + b; }
};

// A += a	
template<typename T>
inline TMatrixBase<T> & operator += (TMatrixBase<T> & A, const T & a)
{
    std::transform(A.begin(), A.end(), A.begin(), scalSum_<T>(a));
    return A;
}

template<typename T>
inline TMatrix<T>
operator + (const TMatrixBase<T> & A, const T & a)
{
    TMatrix<T> B(A);
    return B += a;
}

// B = A * a 
template<typename T>
struct scalProd_ 
{
    const T & _a;
    scalProd_(const T & a) : _a(a) {}
    T operator () (const T & b) { return _a * b; }
};

// A *= a
template<typename T>
inline TMatrixBase<T> & operator *= (TMatrixBase<T> & A, const T & a)
{
    std::transform(A.begin(), A.end(), A.begin(), scalProd_<T>(a));
    return A;
}

// B = A * a 
template<typename T>
inline TMatrix<T> operator * (const TMatrixBase<T> & A, const T & a)
{
    TMatrix<T> B(A);
    return B *= a;
}

template<typename T>
void opposite(const TMatrixBase<T> & A, const TMatrixBase<T> & B)
{
    int ma = A.numRows(), na = A.numColumns();
    {
	int mb = B.numRows(), nb = B.numColumns();
	
	if (! (ma == mb && na == nb))
	    throw Iex::ArgExc("TMatrixAlgo opposite() called with "
			      "arguments having different sizes.");
    }
    if (A.order() == B.order())
    {
	std::transform(A.begin(), A.end(), B.begin(),
		       std::negate<typename TMatrix<T>::value_type>());
    }
    else
    {
	for (int i = A.numRows() -1; i >= 0; --i)
	    for (int j = A.numColumns() -1; j >= 0; --j)
		B[i][j] = -A[i][j];
    }
}

// B = -A
template<typename T>
TMatrix<T> operator - (const TMatrixBase<T> & A)
{
    TMatrix<T> B(A.numRows(), A.numColumns(), A.order());
    std::transform(A.begin(), A.end(), B.begin(),
		   std::negate<typename TMatrix<T>::value_type>());
    return B;
}


// A += B
template<typename T>
TMatrixBase<T> & operator += (TMatrixBase<T> & A, const TMatrixBase<T> & B)
{
    if (A.numRows() != B.numRows() || A.numColumns() != B.numColumns())
	throw Iex::ArgExc("TMatrixAlgo operator += () called with arguments "
			  "having different sizes.");

    if (A.order() == B.order())
    {
	std::transform(A.begin(), A.end(), B.begin(), A.begin(),
		       std::plus<typename TMatrix<T>::value_type>());
    }
    else
    {
	for (int i = A.numRows() -1; i >= 0; --i)
	    for (int j = A.numColumns() -1; j >= 0; --j)
		A[i][j] += B[i][j];
    }

    return A;
}

// add(A, B, C)	
template<typename T>
void add(const TMatrixBase<T> & A, const TMatrixBase<T> & B, TMatrixBase<T> & C)
{
    int ma = A.numRows(), na = A.numColumns();
    {
	int mb = B.numRows(), nb = B.numColumns();
	int mc = C.numRows(), nc = C.numColumns();
	
	if (! (ma == mb && na == nb && ma == mc && na == nc))
	    throw Iex::ArgExc("TMatrixAlgo add() called with "
			      "arguments having different sizes.");
    }

    if (A.order() == B.order() && A.order() == C.order())
    {
	std::transform(A.begin(), A.end(), B.begin(), C.begin(),
		       std::plus<typename TMatrix<T>::value_type>());
    }
    else
    {
	for (int i = 0; i < ma; ++i)
	    for (int j = 0; j < na; ++j)
		C[i][j] = A[i][j] + B[i][j];
    }
}

// C = A + B
template<typename T>
inline TMatrix<T> operator + (const TMatrixBase<T> & A, const TMatrixBase<T> & B)
{
    TMatrix<T> C(A.numRows(), A.numColumns(), A.order());
    add(A, B, C);
    return C;
}

// A -= B
template<typename T>
TMatrixBase<T> & operator -= (TMatrixBase<T> & A, const TMatrixBase<T> & B)
{
    if (A.numRows() != B.numRows() || A.numColumns() != B.numColumns())
	throw Iex::ArgExc("TMatrixAlgo operator -= () called with arguments "
			  "having different sizes.");

    if (A.order() == B.order())
    {
	std::transform(A.begin(), A.end(), B.begin(), A.begin(),
		       std::minus<typename TMatrix<T>::value_type>());
    }
    else
    {
	for (int i = A.numRows() -1; i >= 0; --i)
	    for (int j = A.numColumns() -1; j >= 0; --j)
		A[i][j] -= B[i][j];
    }

    return A;
}

// subtract(A, B, C)	
template<typename T>
void subtract(const TMatrixBase<T> & A, const TMatrixBase<T> & B, TMatrixBase<T> & C)
{
    int ma = A.numRows(), na = A.numColumns();
    {
	int mb = B.numRows(), nb = B.numColumns();
	int mc = C.numRows(), nc = C.numColumns();
	
	if (! (ma == mb && na == nb && ma == mc && na == nc))
	    throw Iex::ArgExc("TMatrixAlgo subtract() called with "
			      "arguments having different sizes.");
    }
    if (A.order() == B.order() && A.order() == C.order())
    {
	std::transform(A.begin(), A.end(), B.begin(), C.begin(),
		       std::minus<typename TMatrix<T>::value_type>());
    }
    else
    {
	for (int i = 0; i < ma; ++i)
	    for (int j = 0; j < na; ++j)
		C[i][j] = A[i][j] - B[i][j];
    }
}

// C = A - B
template<typename T>
inline TMatrix<T> operator - (const TMatrixBase<T> & A, const TMatrixBase<T> & B)
{
    TMatrix<T> C(A.numRows(), A.numColumns(), A.order());
    subtract(A, B, C);
    return C;
}

// mult(A, B, C)	
template<typename T>
void mult(const TMatrixBase<T> & A, const TMatrixBase<T> & B, TMatrixBase<T> & C)
{
    int ma = A.numRows(), na = A.numColumns();
    int mb = B.numRows(), nb = B.numColumns();
    int mc = C.numRows(), nc = C.numColumns();
    {  
	if (na != mb)
	    throw Iex::ArgExc("TMatrixAlgo mult() called with arguments "
			      "having non-conforming sizes: the number of"
			      "columns of the first operand is not equal to " 
			      "the number of rows of the second operand.");
	if (mc != ma)
	    throw Iex::ArgExc("TMatrixAlgo mult() called with arguments "
			      "having non-conforming sizes: the number of"
			      "rows of the first operand is not equal to " 
			      "the number of rows of the result.");
	if (nc != nb)
	    throw Iex::ArgExc("TMatrixAlgo mult() called with arguments "
			      "having non-conforming sizes: the number of"
			      "columns of the second operand is not equal to " 
			      "the number of rows of the result.");
    }

    TMatrixBase<T> * CP = &C;
    TMatrix<T> * CN = 0;
    bool newc = false;
    
    if (CP == &A || CP == &B)
    {
	newc = true;
	CP = CN = new TMatrix<T>(ma, nb, T(0), C.order());
    }
    
    for (int i = 0; i < ma; ++i) 
    {
	for (int j = 0; j < nb; ++j)
	{
	    typename TMatrix<T>::value_type sum = 
		typename TMatrix<T>::value_type(0);
	    for (int k = 0; k < mb; ++k) {
		sum += A[i][k] * B[k][j];
	    }
	    (*CP)[i][j] =sum;
	}
    }

    if (newc)
    {
	std::copy(CN->begin(), CN->end(), C.begin());
	delete CN;
    }
}

// C = A * B	
template<typename T>
inline TMatrix<T> operator * (const TMatrixBase<T> & A, const TMatrixBase<T> & B)
{
    TMatrix<T> C(A.numRows(), B.numColumns(), A.order());
    mult(A, B, C);
    return C;
}

// dotmult(A, B, C)	
template<typename T>
void dotmult(const TMatrixBase<T> & A, const TMatrixBase<T> & B, TMatrixBase<T> & C)
{
    int ma = A.numRows(), na = A.numColumns();
    {
	int mb = B.numRows(), nb = B.numColumns();
	int mc = C.numRows(), nc = C.numColumns();
	
	if (! (ma == mb && na == nb && ma == mc && na == nc))
	    throw Iex::ArgExc("TMatrixAlgo dotmult() called with "
			      "arguments having different sizes.");
    }

    if (A.order() == B.order() && A.order() == C.order())
    {
	std::transform(A.begin(), A.end(), B.begin(), C.begin(),
		       std::multiplies<typename TMatrix<T>::value_type>());
    }
    else
    {
	for (int i = 0; i < ma; ++i)
	    for (int j = 0; j < na; ++j)
		C[i][j] = A[i][j] * B[i][j];
    }
}

// C = A % B
template<typename T>
inline TMatrix<T> operator % (const TMatrixBase<T> & A, const TMatrixBase<T> & B)
{
    TMatrix<T> C(A.numRows(), A.numColumns(), A.order());
    dotmult(A, B, C);
    return C;
}


} // namespace Imath

#endif // INCLUDED_IMATHTMATRIXALGO_H
