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

#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <ImathMatrix.h>
#include <ImathTMatrix.h>
#include <ImathTMatrixAlgo.h>


using namespace Imath;

template<typename T>
static void randMatrix(TMatrix<T> & M)
{
    srand(time(0));
    T * p = M.data();
    for (int i = M.size(); i; --i) 
	*p++ = T(rand());
}
 
static void testConstructorsAndAccessors()
{
    std::cout << "Testing constructors and accessors" << std::endl;
    
    {
	std::cout << "  empty" << std::endl;
	{
	    TMatrix<float> M(TMatrix<float>::ROW_MAJOR);
	    assert(M.order() == TMatrix<float>::ROW_MAJOR);
	    assert(M.empty());
	    assert(M.size() == 0);
	    assert(M.columnStride() == 1);
	}
	{
	    TMatrix<float> M(TMatrix<float>::COLUMN_MAJOR);
	    assert(M.order() == TMatrix<float>::COLUMN_MAJOR);
	    assert(M.empty());
	    assert(M.size() == 0);
	    assert(M.rowStride() == 1);
	}
    }
    {
	std::cout << "  sized" << std::endl;
	{
	    TMatrix<float> M(5,12,TMatrix<float>::ROW_MAJOR);
	    assert(M.order() == TMatrix<float>::ROW_MAJOR);
	    assert(M.numRows() == 5);
	    assert(M.numColumns() == 12);
	    assert(M.size() == M.numRows() * M.numColumns());
	    assert(! M.empty());
	    assert(M.columnStride() == 1);
	    assert(M.rowStride() == M.numColumns());
	    assert(M.data());
	}
	{
	    TMatrix<float> M(9,22,TMatrix<float>::COLUMN_MAJOR);
	    assert(M.order() == TMatrix<float>::COLUMN_MAJOR);
	    assert(M.numRows() == 9);
	    assert(M.numColumns() == 22);
	    assert(M.size() == M.numRows() * M.numColumns());
	    assert(! M.empty());
	    assert(M.columnStride() == M.numRows());
	    assert(M.rowStride() == 1);
	    assert(M.data());
	}
    }
    {   
	std::cout << "  sized and initialized" << std::endl;
	{
	    unsigned val = 19u;
	    TMatrix<unsigned> M(4, 9, val, TMatrix<unsigned>::ROW_MAJOR);
	    assert(M.order() == TMatrix<unsigned>::ROW_MAJOR);
	    assert(M.numRows() == 4);
	    assert(M.numColumns() == 9);
	    assert(M.size() == M.numRows() * M.numColumns());
	    assert(! M.empty());
	    assert(M.columnStride() == 1);
	    assert(M.rowStride() == M.numColumns());
	    assert(M.data());
	    unsigned * p = M.data();
	    for (int i = M.size(); i; --i)
		assert(*p++ == val);
	}
	{
	    unsigned val = 19u;
	    TMatrix<unsigned> M(4, 9, val, TMatrix<unsigned>::COLUMN_MAJOR);
	    assert(M.order() == TMatrix<unsigned>::COLUMN_MAJOR);
	    assert(M.numRows() == 4);
	    assert(M.numColumns() == 9);
	    assert(M.size() == M.numRows() * M.numColumns());
	    assert(! M.empty());
	    assert(M.columnStride() == M.numRows());
	    assert(M.rowStride() == 1);
	    assert(M.data());
	    unsigned * p = M.data();
	    for (int i = M.size(); i; --i)
		assert(*p++ == val);
	}
    }
    {
	std::cout << "  copy" << std::endl;
	{
	    TMatrix<double> M(4, 9, TMatrix<double>::ROW_MAJOR);
	    randMatrix(M);
	    TMatrix<double> N(M);
	    assert(N.order() == M.order());
	    assert(N.numRows() == M.numRows());
	    assert(N.numColumns() == M.numColumns());
	    assert(N.size() == M.size());
	    assert(! N.empty());
	    assert(N.columnStride() == M.columnStride());
	    assert(N.rowStride() == M.rowStride());
	    assert(N.data());
	    double * p = N.data(), * q = M.data();
	    for (int i = M.size(); i; --i)
		assert(*p++ == *q++);
	}
	{
	    TMatrix<double> M(4, 9, TMatrix<double>::COLUMN_MAJOR);
	    randMatrix(M);
	    TMatrix<double> N(M);
	    assert(N.order() == M.order());
	    assert(N.numRows() == M.numRows());
	    assert(N.numColumns() == M.numColumns());
	    assert(N.size() == M.size());
	    assert(! N.empty());
	    assert(N.columnStride() == M.columnStride());
	    assert(N.rowStride() == M.rowStride());
	    assert(N.data());
	    double * p = N.data(), * q = M.data();
	    for (int i = M.size(); i; --i)
		assert(*p++ == *q++);
	}
    }
    {
	std::cout << "  assignment" << std::endl;
	{
	    TMatrix<int> M(2, 12, TMatrix<int>::ROW_MAJOR);
	    randMatrix(M);
	    TMatrix<int> N(7, 11, TMatrix<int>::COLUMN_MAJOR);
	    randMatrix(N);
	    N = M;
	    assert(N.order() == M.order());
	    assert(N.numRows() == M.numRows());
	    assert(N.numColumns() == M.numColumns());
	    assert(N.size() == M.size());
	    assert(! N.empty());
	    assert(N.columnStride() == M.columnStride());
	    assert(N.rowStride() == M.rowStride());
	    assert(N.data());
	    int * p = N.data(), * q = M.data();
	    for (int i = M.size(); i; --i)
		assert(*p++ == *q++);
	}
	{
	    TMatrix<int> M(2, 12, TMatrix<int>::COLUMN_MAJOR);
	    randMatrix(M);
	    TMatrix<int> N(7, 11, TMatrix<int>::ROW_MAJOR);
	    randMatrix(N);
	    N = M;
	    assert(N.order() == M.order());
	    assert(N.numRows() == M.numRows());
	    assert(N.numColumns() == M.numColumns());
	    assert(N.size() == M.size());
	    assert(! N.empty());
	    assert(N.columnStride() == M.columnStride());
	    assert(N.rowStride() == M.rowStride());
	    assert(N.data());
	    int * p = N.data(), * q = M.data();
	    for (int i = M.size(); i; --i)
		assert(*p++ == *q++);
	}
    }
    {
	std::cout << "  clear" << std::endl;
	TMatrix<short> M(7, 5, TMatrix<short>::ROW_MAJOR);
	M.clear();
	assert(M.empty());
	assert(M.numRows() == 0);
	assert(M.numColumns() == 0);
	assert(M.size() == 0);
	assert(M.order() == TMatrix<short>::ROW_MAJOR);
	assert(! M.data());
    }
}


static void testNull()
{
    std::cout << "Test null matrix " << std::endl;
    {
	TMatrix<int> M = TMatrix<int>::null(3u,4u, TMatrix<int>::ROW_MAJOR);
	int * p = M.data();
	for (int i = M.size(); i; --i)
	    assert (*p++ == 0);
    }
    {
	TMatrix<int> M = TMatrix<int>::null(3u,4u, TMatrix<int>::COLUMN_MAJOR);
	int * p = M.data();
	for (int i = M.size(); i; --i)
	    assert (*p++ == 0);
    }
}

static void testIdentity()
{
    std::cout << "Test identity matrix " << std::endl;
    {
	TMatrix<int> M =
	    TMatrix<int>::identity(4u, TMatrix<int>::ROW_MAJOR);
	int * p = M.data();
	for (int i = 0; i < M.numRows(); ++i)
	    for (int j = 0; j < M.numColumns(); ++j, ++p)
		assert((i == j && *p==1) || (i != j && *p == 0));
    }
    {
	TMatrix<int> M =
	    TMatrix<int>::identity(4u, TMatrix<int>::COLUMN_MAJOR);
	int * p = M.data();
	for (int i = 0; i < M.numRows(); ++i)
	    for (int j = 0; j < M.numColumns(); ++j, ++p)
		assert((i == j && *p==1) || (i != j && *p == 0));
    }
}

static void testUncheckedAccess()
{
    std::cout << "Test row-major access via operator [] " << std::endl;

    int m = 7, n = 17;
	
    {
	TMatrix<int> M(m,n,TMatrix<int>::ROW_MAJOR);
	randMatrix(M);
	int * p = M.data();
	for (int i = 0; i < m; ++i) 
	    for (int j = 0; j < n; ++j)
		assert(M[i][j] == *p++);

	const TMatrix<int> & N = M;
	const int * q = N.data();
	for (int i = 0; i < m; ++i) 
	    for (int j = 0; j < n; ++j)
		assert(N[i][j] == *q++);		
    }
    {
	TMatrix<int> M(m,n,TMatrix<int>::COLUMN_MAJOR);
	randMatrix(M);
	int * p = M.data();
	for (int j = 0; j < n; ++j)
	    for (int i = 0; i < m; ++i) 
		assert(M[i][j] == *p++);

	const TMatrix<int> & N = M;
	const int * q = N.data();
	for (int j = 0; j < n; ++j)
	    for (int i = 0; i < m; ++i) 
		assert(N[i][j] == *q++);
    }
}

static void testCheckedAccess()
{
    std::cout << "Test access via operator () " << std::endl;

    int m = 4, n = 13;
	
    {
	TMatrix<int> M(m,n,TMatrix<int>::ROW_MAJOR);
	randMatrix(M);
	int * p = M.data();
	for (int i = 0; i < m; ++i) 
	    for (int j = 0; j < n; ++j)
		assert(M(i,j) == *p++);
		
	const TMatrix<int> & N = M;
	const int * q = N.data();
	for (int i = 0; i < m; ++i) 
	    for (int j = 0; j < n; ++j)
		assert(N(i,j) == *q++);
		
	try
	{
	    M(m,n+1);
	}
	catch (Iex::ArgExc e)
	{
	    std::cout << "  bound check" << std::endl;
	}
    }
    {
	TMatrix<int> M(m,n,TMatrix<int>::COLUMN_MAJOR);
	randMatrix(M);
	int * p = M.data();
	for (int j = 0; j < n; ++j)
	    for (int i = 0; i < m; ++i) 
		assert(M(i,j) == *p++);

	const TMatrix<int> & N = M;
	const int * q = N.data();
	for (int j = 0; j < n; ++j)
	    for (int i = 0; i < m; ++i) 
		assert(N(i,j) == *q++);
		
	try
	{
	    M(m+1,n);
	}
	catch (Iex::ArgExc e)
	{
	    std::cout << "  bound check" << std::endl;
	}
    }
}

static void testConversions()
{
    std::cout << "Test conversion from Matrix33" << std::endl;
    {
	Matrix33<int> m33;
	srand(time(0));
	for (int i = 0; i < 3; ++i)
	    for (int j = 0; j < 3; ++j)
		m33[i][j] = ::rand();

	TMatrix<int> M(m33);
	assert(M.order() == TMatrix<int>::ROW_MAJOR);
	assert(M.numRows() == 3);
	assert(M.numColumns() == 3);
	int * p = M.data();
	for (int i = 0; i < 3; ++i)
	    for (int j = 0; j < 3; ++j)
		assert(m33[i][j] == *p++);
    }
    {
	Matrix33<int> m33;
	srand(time(0));
	for (int i = 0; i < 3; ++i)
	    for (int j = 0; j < 3; ++j)
		m33[i][j] = rand();

	TMatrix<int> M(m33, TMatrix<int>::COLUMN_MAJOR);
	assert(M.order() == TMatrix<int>::COLUMN_MAJOR);
	assert(M.numRows() == 3);
	assert(M.numColumns() == 3);
	int  * p = M.data();
	for (int j = 0; j < 3; ++j)
	    for (int i = 0; i < 3; ++i)
		assert(m33[i][j] == *p++);
    }
    std::cout << "  to Matrix33" << std::endl;
    {
	TMatrix<int> M(5,6);
	randMatrix(M);
			
	Matrix33<int> m33 = M.asMatrix33();

	for (int i = 0; i < 3; ++i)
	    for (int j = 0; j < 3; ++j)
		assert(m33[i][j] == M[i][j]);
    }
    {
	TMatrix<int> M(5,6,TMatrix<int>::COLUMN_MAJOR);
	randMatrix(M);
			
	Matrix33<int> m33 = M.asMatrix33();

	for (int i = 0; i < 3; ++i)
	    for (int j = 0; j < 3; ++j)
		assert(m33[i][j] == M[i][j]);
    }
    std::cout << "  from Matrix44" << std::endl;
    {
	Matrix44<int> m44;
	srand(time(0));
	for (int i = 0; i < 4; ++i)
	    for (int j = 0; j < 4; ++j)
		m44[i][j] = rand();
		
	TMatrix<int> M(m44);
	assert(M.order() == TMatrix<int>::ROW_MAJOR);
	assert(M.numRows() == 4);
	assert(M.numColumns() == 4);
	int * p = M.data();
	for (int i = 0; i < 4; ++i)
	    for (int j = 0; j < 4; ++j)
		assert(m44[i][j] == *p++);
		
    }
    {
	Matrix44<int> m44;
	srand(time(0));
	for (int i = 0; i < 4; ++i)
	    for (int j = 0; j < 4; ++j)
		m44[i][j] = rand();
		
	TMatrix<int> M(m44, TMatrix<int>::COLUMN_MAJOR);
	assert(M.order() == TMatrix<int>::COLUMN_MAJOR);
	assert(M.numRows() == 4);
	assert(M.numColumns() == 4);
	int * p = M.data();
	for (int j = 0; j < 4; ++j)
	    for (int i = 0; i < 4; ++i)
		assert(m44[i][j] == *p++);
    }
    std::cout << "  to Matrix44" << std::endl;
    {
	TMatrix<int> M(5,6);
	randMatrix(M);
		
	Matrix44<int> m44 = M.asMatrix44();
		
	for (int i = 0; i < 4; ++i)
	    for (int j = 0; j < 4; ++j)
		assert(m44[i][j] == M[i][j]);
    }
    {
	TMatrix<int> M(5,6,TMatrix<int>::COLUMN_MAJOR);
	randMatrix(M);
		
	Matrix44<int> m44 = M.asMatrix44();
		
	for (int i = 0; i < 4; ++i)
	    for (int j = 0; j < 4; ++j)
		assert(m44[i][j] == M[i][j]);
    }
}

static void testTranspose()
{
    std::cout << "Testing transposition" << std::endl;

    int m = 4, n = 13;
	
    {
	std::cout << "  in place" << std::endl;
		
	TMatrix<int> M(m,n,TMatrix<int>::ROW_MAJOR);
	randMatrix(M);
	
	TMatrix<int> N(M);

	M.transpose();

	assert(M.numRows() == N.numColumns());
	assert(N.numRows() == M.numColumns());
	assert(M.size() == N.size());
	assert(M.order() == N.order());
		
	for (int i = M.numRows()-1; i >= 0; --i)
	    for (int j = M.numColumns()-1; j >= 0; --j)
		assert(M(i,j) == N(j,i));

	std::cout << "  out place" << std::endl;

	N = M.transposed();
		
	assert(M.numRows() == N.numColumns());
	assert(N.numRows() == M.numColumns());
	assert(M.size() == N.size());
	assert(M.order() == N.order());
		
	for (int i = M.numRows()-1; i >= 0; --i)
	    for (int j = M.numColumns()-1; j >= 0; --j)
		assert(M(i,j) == N(j,i));

    }
    {
	std::cout << "  in place" << std::endl;

	TMatrix<int> M(m,n,TMatrix<int>::COLUMN_MAJOR);
	randMatrix(M);
	
	TMatrix<int> N(M);

	M.transpose();

	assert(M.numRows() == N.numColumns());
	assert(N.numRows() == M.numColumns());
	assert(M.size() == N.size());
	assert(M.order() == N.order());
		
	for (int i = M.numRows()-1; i >= 0; --i)
	    for (int j = M.numColumns()-1; j >= 0; --j)
		assert(M(i,j) == N(j,i));

	std::cout << "  out place" << std::endl;

	N = M.transposed();
		
	assert(M.numRows() == N.numColumns());
	assert(N.numRows() == M.numColumns());
	assert(M.size() == N.size());
	assert(M.order() == N.order());
		
	for (int i = M.numRows()-1; i >= 0; --i)
	    for (int j = M.numColumns()-1; j >= 0; --j)
		assert(M(i,j) == N(j,i));
    }
}

static void testSetOrder()
{
    std::cout << "Testing set order" << std::endl;

    int m = 8, n = 3;
	
    {
	TMatrix<int> M(m,n,TMatrix<int>::ROW_MAJOR);
	randMatrix(M);
	
	TMatrix<int> N(M);

	std::cout << "  idempotence" << std::endl;
		
	M.setOrder(TMatrix<int>::ROW_MAJOR);

	assert(M.numRows() == N.numRows());
	assert(N.numColumns() == M.numColumns());
	assert(M.size() == N.size());
	assert(M.order() == N.order());
	assert(M.rowStride() == N.rowStride());
	assert(M.columnStride() == N.columnStride());
		
	for (int i = M.numRows()-1; i >= 0; --i)
	    for (int j = M.numColumns()-1 ; j >= 0; --j)
		assert(M(i,j) == N(i,j));

	std::cout << "  order change" << std::endl;

	M.setOrder(TMatrix<int>::COLUMN_MAJOR);
		
	assert(M.numRows() == N.numRows());
	assert(N.numColumns() == M.numColumns());
	assert(M.size() == N.size());
	assert(M.order() != N.order());
	assert(M.rowStride() != N.rowStride());
	assert(M.columnStride() != N.columnStride());
		
	for (int i = M.numRows()-1; i >= 0; --i)
	    for (int j = M.numColumns()-1 ; j >= 0; --j)
		assert(M(i,j) == N(i,j));

	int * p = M.data();
	for (int j = 0; j  < N.numColumns(); ++j)
	    for (int i = 0; i < N.numRows(); ++i)
		assert(N(i,j) == *p++);
    }
    {
	TMatrix<int> M(m,n,TMatrix<int>::COLUMN_MAJOR);
	randMatrix(M);
	
	TMatrix<int> N(M);

	std::cout << "  idempotence" << std::endl;
		
	M.setOrder(TMatrix<int>::COLUMN_MAJOR);

	assert(M.numRows() == N.numRows());
	assert(N.numColumns() == M.numColumns());
	assert(M.size() == N.size());
	assert(M.order() == N.order());
	assert(M.rowStride() == N.rowStride());
	assert(M.columnStride() == N.columnStride());
		
	for (int i = M.numRows()-1; i >= 0; --i)
	    for (int j = M.numColumns()-1; j >= 0; --j)
		assert(M(i,j) == N(i,j));

	std::cout << "  order change" << std::endl;

	M.setOrder(TMatrix<int>::ROW_MAJOR);
		
	assert(M.numRows() == N.numRows());
	assert(N.numColumns() == M.numColumns());
	assert(M.size() == N.size());
	assert(M.order() != N.order());
	assert(M.rowStride() != N.rowStride());
	assert(M.columnStride() != N.columnStride());
		
	for (int i = M.numRows()-1; i >= 0; --i)
	    for (int j = M.numColumns()-1; j >= 0; --j)
		assert(M(i,j) == N(i,j));

	int * p = M.data();
	for (int i = 0; i < N.numRows(); ++i)
	    for (int j = 0; j < N.numColumns(); ++j)
		assert(N(i,j) == *p++);
    }
}

static void testResize()
{
    std::cout << "Testing resize" << std::endl;
    int m = 3, n = 19;
	
    {
	std::cout << "  enlarging" << std::endl;
		
	TMatrix<int> M(m, n, TMatrix<int>::ROW_MAJOR);
	randMatrix(M);

	TMatrix<int> N(M);
	M.resize(m+4, n+7, 5);
	assert(M.numRows() == m+4);
	assert(M.numColumns() == n+7);
	assert(M.size() == (m+4)*(n+7)); 
		
	int i = 0, j = 0;
	for (i=0; i < N.numRows(); ++i)
	{
	    for (j = 0; j < N.numColumns(); ++j) 
		assert(M(i,j) == N(i,j));
	    for (; j < M.numColumns(); ++j) 
		assert(5 == M(i, j));
	}
	for (; i < M.numRows(); ++i)
	    for (j = 0; j < M.numColumns(); ++j) 
		assert(5 == M(i, j));

	std::cout << "  shrinking" << std::endl;

	M.resize(m-1, n-2);
	assert(M.numRows() == m-1);
	assert(M.numColumns() == n-2);
	assert(M.size() == (m-1)*(n-2));

	for (int i=0; i < M.numRows(); ++i) 
	    for (int j = 0; j < M.numColumns(); ++j)
		assert(M(i,j) == N(i,j));
    }
    {
	std::cout << "  enlarging" << std::endl;
		
	TMatrix<int> M(m, n, TMatrix<int>::COLUMN_MAJOR);
	randMatrix(M);

	TMatrix<int> N(M);

	M.resize(m+3, n+6, 0);
	assert(M.numRows() == m+3);
	assert(M.numColumns() == n+6);
	assert(M.size() == (m+3)*(n+6));

	int i = 0, j = 0;
	for (i=0; i < N.numRows(); ++i)
	{
	    for (j = 0; j < N.numColumns(); ++j)
		assert(M(i,j) == N(i,j));
	    for (; j < M.numColumns(); ++j)
		assert(0 == M(i, j));
	}
	for (; i < M.numRows(); ++i)
	    for (j = 0; j < M.numColumns(); ++j)
		assert(0 == M(i, j));

	std::cout << "  shrinking" << std::endl;

	M.resize(m-1, n-4);
	assert(M.numRows() == m-1);
	assert(M.numColumns() == n-4);
	assert(M.size() == (m-1)*(n-4));

	for (int i=0; i < M.numRows(); ++i) 
	    for (int j = 0; j < M.numColumns(); ++j)
		assert(M(i,j) == N(i,j));
    }
}

static void testIterators()
{
    std::cout << "Testing iterators" << std::endl;

    int m = 2, n = 5;
	
    {
	std::cout << "  iterator, reverse_iterator" << std::endl;
	TMatrix<int> M(m,n,TMatrix<int>::ROW_MAJOR);
	randMatrix(M);

	assert(&(*(M.begin())) == M.data());
	assert(&(*(M.rbegin())) == M.data() + M.size() -1);
	assert(std::distance(M.begin(), M.end()) == M.size());
	assert(std::distance(M.rbegin(), M.rend()) == M.size());

	int * p = M.data();
	TMatrix<int>::iterator it = M.begin(), ie = M.end();
	while (it != ie)
	    assert(*it++ == *p++);
	p = M.data() + M.size() -1;
	TMatrix<int>::reverse_iterator rit = M.rbegin(), rie = M.rend();
	while (rit != rie)
	    assert(*rit++ == *p--);
		
	std::cout << "  const_iterator, const_reverse_iterator" << std::endl;
		
	const TMatrix<int> & N = M;

	assert(&(*(N.begin())) == N.data());
	assert(&(*(N.rbegin())) == N.data() + N.size() -1);
	assert(std::distance(N.begin(), N.end()) == N.size());
	assert(std::distance(N.rbegin(), N.rend()) == N.size());

	const int * q = N.data();
	TMatrix<int>::const_iterator cit = N.begin(), cie = N.end();
	while (cit != cie)
	    assert(*cit++ == *q++);
	q = N.data() + N.size() -1;
	TMatrix<int>::const_reverse_iterator crit =N.rbegin(), crie =N.rend();
	while (crit != crie)
	    assert(*crit++ == *q--);
    }

    {
	std::cout << "  iterator, reverse_iterator" << std::endl;
		
	TMatrix<int> M(m,n,TMatrix<int>::COLUMN_MAJOR);
	randMatrix(M);

	assert(&(*(M.begin())) == M.data());
	assert(&(*(M.rbegin())) == M.data() + M.size() -1);
	assert(std::distance(M.begin(), M.end()) == M.size());
	assert(std::distance(M.rbegin(), M.rend()) == M.size());

	int * p = M.data();
	TMatrix<int>::iterator it = M.begin(), ie = M.end();
	while (it != ie)
	    assert(*it++ == *p++);
	p = M.data() + M.size() -1;
	TMatrix<int>::reverse_iterator rit = M.rbegin(), rie = M.rend();
	while (rit != rie)
	    assert(*rit++ == *p--);
		
	std::cout << "  const_iterator, const_reverse_iterator" << std::endl;
		
	const TMatrix<int> & N = M;

	assert(&(*(N.begin())) == N.data());
	assert(&(*(N.rbegin())) == N.data() + N.size() -1);
	assert(std::distance(N.begin(), N.end()) == N.size());
	assert(std::distance(N.rbegin(), N.rend()) == N.size());

	const int * q = N.data();
	TMatrix<int>::const_iterator cit = N.begin(), cie = N.end();
	while (cit != cie)
	    assert(*cit++ == *q++);
	q = N.data() + N.size() -1;
	TMatrix<int>::const_reverse_iterator crit =N.rbegin(), crie =N.rend();
	while (crit != crie)
	    assert(*crit++ == *q--);
    }
}

template<typename T>
static void sameordie(const TMatrix<T> & M, const TMatrix<T> & N)
{
    assert(M.numRows() == N.numRows());
    assert(N.numColumns() == M.numColumns());
    assert(M.size() == N.size());
    assert(M.order() == N.order());
    assert(M.rowStride() == N.rowStride());
    assert(M.columnStride() == N.columnStride());
}

// std::iota is apparently not standard.
template<typename iterator, typename T>
void
iota(iterator first, iterator last, T value)
{
    while (first != last) {
	*first++ = value;
	value += T(1);
    }
}

static void testAlgebra()
{	
    std::cout << "Testing algebraic operators.." << std::endl;
    {
	std::cout << "  A * a" << std::endl;
	TMatrix<long> M(9,22);
	iota(M.begin(), M.end(), -5L);

	TMatrix<long> P(M * 44L);

	sameordie(M, P);
	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == 44L * M(i,j));
    }
    {
	std::cout << "  A + a" << std::endl;
	TMatrix<long> M(3,13);
	iota(M.begin(), M.end(), -9L);

	TMatrix<long> P(M + 9L);

	sameordie(M, P);
	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == 9L + M(i,j));
    }
    {
	std::cout << "  A += a" << std::endl;
	TMatrix<long> M(3,11);
	iota(M.begin(), M.end(), -6L);
	TMatrix<long> N(M);

	N += 32L;

	sameordie(M, N);
	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(N(i,j) == 32L + M(i,j));
    }
    {
	std::cout << "  A *= a" << std::endl; 
	TMatrix<long> M(4,18);
	iota(M.begin(), M.end(), -1L);
	TMatrix<long> N(M); 

	N *= -7L;

	sameordie(M, N);
	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(N(i,j) == -7L * M(i,j));
    }
    {
	std::cout << "  B = -A" << std::endl;
	TMatrix<double> M(3,5);
	randMatrix(M);

	TMatrix<double> N(-M);

	sameordie(M, N);
	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(M(i,j) == -N(i,j));
    }
    {
	std::cout << "  A += B" << std::endl;
	TMatrix<long> M(3,5);
	randMatrix(M);
	TMatrix<long> N(8,7);
	randMatrix(N);
	TMatrix<long> P(M);
	try
	{
	    P += N;
	}
	catch (Iex::ArgExc)
	{
	    std::cout << "    (size exc caught OK)" << std::endl;
	    N.resize(M.numRows(), M.numColumns());
	    sameordie(P, N);
	    sameordie(M, N);
	}
	P += N;

	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == M(i,j) + N(i,j));

	P= M;
	P.setOrder(TMatrix<long>::COLUMN_MAJOR);
	P += N;
		
	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == M(i,j) + N(i,j));
    }
    {
	std::cout << "  C = A + B " << std::endl;
	TMatrix<long> M(3,5);
	randMatrix(M);
	TMatrix<long> N(8,7);
	randMatrix(N);
	TMatrix<long> P;
	try
	{
	    P = M + N;
	}
	catch (Iex::ArgExc)
	{
	    std::cout << "    (size exc caught OK)" << std::endl;
	    N.resize(M.numRows(), M.numColumns());
	    sameordie(M, N);
	}
	P = M + N;
	sameordie(P, N);

	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == M(i,j) + N(i,j));
    }
    {
	std::cout << "  add(A, B, C) " << std::endl;
	TMatrix<long> M(3,5);
	randMatrix(M);
	TMatrix<long> N(8,7);
	randMatrix(N);
	TMatrix<long> P;
	try
	{
	    add(M,N,P);
	}
	catch (Iex::ArgExc)
	{
	    std::cout << "    (size exc caught OK)" << std::endl;
	    N.resize(M.numRows(), M.numColumns());
	    sameordie(M, N);
	}
	add(M,N,P);
	sameordie(P, N);

	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == M(i,j) + N(i,j));

	N.setOrder(TMatrix<long>::COLUMN_MAJOR);
	add(N,M,P);

	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == M(i,j) + N(i,j));
    }
    {
	std::cout << "  A -= B" << std::endl;
	TMatrix<long> M(3,5);
	randMatrix(M);
	TMatrix<long> N(8,7);
	randMatrix(N);
	TMatrix<long> P(M);
	try
	{
	    P -= N;
	}
	catch (Iex::ArgExc)
	{
	    std::cout << "    (size exc caught OK)" << std::endl;
	    N.resize(M.numRows(), M.numColumns());
	    sameordie(P, N);
	    sameordie(M, N);
	}
	P -= N;

	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == M(i,j) - N(i,j));

	P= M;
	P.setOrder(TMatrix<long>::COLUMN_MAJOR);
	P -= N;
		
	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == M(i,j) - N(i,j));
    }
    {
	std::cout << "  C = A - B " << std::endl;
	TMatrix<long> M(3,5);
	randMatrix(M);
	TMatrix<long> N(8,7);
	randMatrix(N);
	TMatrix<long> P;
	try
	{
	    P = M - N;
	}
	catch (Iex::ArgExc)
	{
	    std::cout << "    (size exc caught OK)" << std::endl;
	    N.resize(M.numRows(), M.numColumns());
	    sameordie(M, N);
	}
	P = M - N;
	sameordie(P, N);

	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == M(i,j) - N(i,j));
    }
    {
	std::cout << "  subtract(A, B, C) " << std::endl;
	TMatrix<long> M(3,5);
	randMatrix(M);
	TMatrix<long> N(8,7);
	randMatrix(N);
	TMatrix<long> P;
	try
	{
	    subtract(M,N,P);
	}
	catch (Iex::ArgExc)
	{
	    std::cout << "    (size exc caught OK)" << std::endl;
	    N.resize(M.numRows(), M.numColumns());
	    sameordie(M, N);
	}
	subtract(M,N,P);
	sameordie(P, N);

	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == M(i,j) - N(i,j));

	N.setOrder(TMatrix<long>::COLUMN_MAJOR);
	subtract(M,N,P);

	for (int i = 0; i<M.numRows(); ++i)
	    for (int j = 0; j<M.numRows(); ++j)
		assert(P(i,j) == M(i,j) - N(i,j));
		
    }
    {
	std::cout << "  C = A * B" << std::endl;
	TMatrix<long> M(3, 7);
	iota(M.begin(), M.end(), 1L);
	TMatrix<long> N(13, 9);
	iota(N.begin(), N.end(), 3L);
	TMatrix<long> P;
	try
	{
	    P = M * N;
	}
	catch (Iex::ArgExc)
	{
	    std::cout << "    (size exc caught OK)" << std::endl;
	    N.resize(7,9);
	}
	P = M * N;
	assert(P.numRows() == M.numRows());
	assert(P.numColumns() == N.numColumns());
	assert(P.order() == M.order());
		
	for (int i = 0; i < P.numRows(); ++i)
	{
	    for (int j = 0; j < P.numColumns(); ++j)
	    {
		double sum = 0;
		for (int k = 0; k < M.numColumns(); ++k) 
		    sum += M(i,k) * N(k,j);
		assert(sum == P(i,j));
	    }
	}
    }
    {
	std::cout << "  mult(A, B, C)" << std::endl;
	TMatrix<long> M(3, 7);
	iota(M.begin(), M.end(), 1L);
	TMatrix<long> N(13, 9);
	iota(N.begin(), N.end(), 3L);
	TMatrix<long> P;
	try
	{
	    mult(M,N,P);
	}
	catch (Iex::ArgExc)
	{
	    std::cout << "    (size exc caught OK)" << std::endl;
	    N.resize(7,9);
	}
	mult(M,N,P);
	assert(P.numRows() == M.numRows());
	assert(P.numColumns() == N.numColumns());
		
	for (int i = 0; i < P.numRows(); ++i) {
	    for (int j = 0; j < P.numColumns(); ++j) {
		double sum = 0;
		for (int k = 0; k < M.numColumns(); ++k) 
		    sum += M(i,k) * N(k,j);
		assert(sum == P(i,j));
	    }
	}

	N.setOrder(TMatrix<long>::COLUMN_MAJOR);
	mult(M,N,P);
	assert(P.numRows() == M.numRows());
	assert(P.numColumns() == N.numColumns());
		
	for (int i = 0; i < P.numRows(); ++i) 
	{
	    for (int j = 0; j < P.numColumns(); ++j) 
	    {
		double sum = 0;
		for (int k = 0; k < M.numColumns(); ++k) 
		    sum += M(i,k) * N(k,j);
		assert(sum == P(i,j));
	    }
	}
		
    }
}


void testTMatrix()
{

    std::cout << "Test TMatrix test" << std::endl;
    try 
    {
	testConstructorsAndAccessors();
	testNull();
	testIdentity();
	testUncheckedAccess();
	testCheckedAccess();
	testConversions();
	testTranspose();
	testSetOrder();
	testResize();
	testAlgebra();
    }
    catch ( Iex::BaseExc & e ) 
    {
	std::cout << "Trapped exception: " << e.what() << std::endl;
	throw;
    }
    std::cout << "ok" << std::endl << std::endl;
}
