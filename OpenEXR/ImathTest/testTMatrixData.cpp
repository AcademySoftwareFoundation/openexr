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

#include <vector>
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ImathTMatrix.h>
#include <ImathTMatrixData.h>


using namespace std;
using namespace Imath;

template<typename T>
void randMatrix(TMatrixData<T> & M)
{
    srand(time(0));
    T * p = M.data();
    for (int i = M.size(); i; --i)
	*p++ = T(rand());
}

template<typename T>
static void sameordie(const TMatrixData<T> & M, const TMatrixData<T> & N)
{
    assert(M.numRows() == N.numRows());
    assert(N.numColumns() == M.numColumns());
    assert(M.size() == N.size());
    assert(M.order() == N.order());
    assert(M.rowStride() == N.rowStride());
    assert(M.columnStride() == N.columnStride());
}


static void testConstructorsAndAccessors()
{
    cout << "Testing constructors and accessors" << endl;

    vector<int> D(1024);
    int * data = &(D.front());
    
    {
	cout << "  sized" << endl;
	{
	    TMatrixData<int> M(5,12,data,TMatrixData<int>::ROW_MAJOR);
	    assert(M.order() == TMatrixData<int>::ROW_MAJOR);
	    assert(M.numRows() == 5);
	    assert(M.numColumns() == 12);
	    assert(M.size() == M.numRows() * M.numColumns());
	    assert(! M.empty());
	    assert(M.columnStride() == 1);
	    assert(M.rowStride() == M.numColumns());
	    assert(M.data());
	}
	{
	    TMatrixData<int> M(9,22,data,TMatrixData<int>::COLUMN_MAJOR);
	    assert(M.order() == TMatrixData<int>::COLUMN_MAJOR);
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
	cout << "  sized and initialized" << endl;
	{
	    int val = 19u;
	    TMatrixData<int> M(4, 9, val, data,
			       TMatrixData<int>::ROW_MAJOR);
	    assert(M.order() == TMatrixData<int>::ROW_MAJOR);
	    assert(M.numRows() == 4);
	    assert(M.numColumns() == 9);
	    assert(M.size() == M.numRows() * M.numColumns());
	    assert(! M.empty());
	    assert(M.columnStride() == 1);
	    assert(M.rowStride() == M.numColumns());
	    assert(M.data());
	    int * p = M.data();
	    for (int i = M.size(); i; --i)
		assert(*p++ == val);
	}
	{
	    int val = 19u;
	    TMatrixData<int> M(4, 9, val, data,
			       TMatrixData<int>::COLUMN_MAJOR);
	    assert(M.order() == TMatrixData<int>::COLUMN_MAJOR);
	    assert(M.numRows() == 4);
	    assert(M.numColumns() == 9);
	    assert(M.size() == M.numRows() * M.numColumns());
	    assert(! M.empty());
	    assert(M.columnStride() == M.numRows());
	    assert(M.rowStride() == 1);
	    assert(M.data());
	    int * p = M.data();
	    for (int i = M.size(); i; --i)
		assert(*p++ == val);
	}
    }
    {
	cout << "  copy" << endl;
	{
	    TMatrixData<int> M(4, 9, data, TMatrixData<int>::ROW_MAJOR);
	    TMatrixData<int> N(M);
	    assert(N.order() == M.order());
	    assert(N.numRows() == M.numRows());
	    assert(N.numColumns() == M.numColumns());
	    assert(N.size() == M.size());
	    assert(! N.empty());
	    assert(N.columnStride() == M.columnStride());
	    assert(N.rowStride() == M.rowStride());
	    assert(N.data() && N.data() == M.data());
	}
	{
	    TMatrixData<int> M(4, 9, data, TMatrixData<int>::COLUMN_MAJOR);
	    TMatrixData<int> N(M);
	    assert(N.order() == M.order());
	    assert(N.numRows() == M.numRows());
	    assert(N.numColumns() == M.numColumns());
	    assert(N.size() == M.size());
	    assert(! N.empty());
	    assert(N.columnStride() == M.columnStride());
	    assert(N.rowStride() == M.rowStride());
	    assert(N.data() && N.data() == M.data());
	}
    }
    {
	cout << "  assignment" << endl;
	{
	    TMatrixData<int> M(2, 12, data, TMatrixData<int>::ROW_MAJOR);
	    TMatrixData<int> N(7, 11, data, TMatrixData<int>::COLUMN_MAJOR);
	    N = M;
	    assert(N.order() == M.order());
	    assert(N.numRows() == M.numRows());
	    assert(N.numColumns() == M.numColumns());
	    assert(N.size() == M.size());
	    assert(! N.empty());
	    assert(N.columnStride() == M.columnStride());
	    assert(N.rowStride() == M.rowStride());
	    assert(N.data() && N.data() == M.data());
	}
	{
	    TMatrixData<int> M(2, 12, data, TMatrixData<int>::COLUMN_MAJOR);
	    TMatrixData<int> N(7, 11, data, TMatrixData<int>::ROW_MAJOR);
	    N = M;
	    assert(N.order() == M.order());
	    assert(N.numRows() == M.numRows());
	    assert(N.numColumns() == M.numColumns());
	    assert(N.size() == M.size());
	    assert(! N.empty());
	    assert(N.columnStride() == M.columnStride());
	    assert(N.rowStride() == M.rowStride());
	    assert(N.data() && N.data() == M.data());
	}
    }
}


static void testUncheckedAccess()
{
    cout <<"  Test row-major access via operator []" << std::endl;
    
    int m = 7, n = 17;
    vector<int> D(1024);
    int * data = &(D.front());
    
    {
	TMatrixData<int> M(m,n,data,TMatrixData<int>::ROW_MAJOR);
	randMatrix(M);
	int * p = M.data();
	for (int i = 0; i < m; ++i) 
	    for (int j = 0; j < n; ++j)
		assert(M[i][j] == *p++);
	
	const TMatrixData<int> & N = M;
	const int * q = N.data();
	for (int i = 0; i < m; ++i) 
	    for (int j = 0; j < n; ++j)
		assert(N[i][j] == *q++);		
    }
    {
	TMatrixData<int> M(m,n,data,TMatrixData<int>::COLUMN_MAJOR);
	randMatrix(M);
	int * p = M.data();
	for (int j = 0; j < n; ++j)
	    for (int i = 0; i < m; ++i) 
		assert(M[i][j] == *p++);
	
	const TMatrixData<int> & N = M;
	const int * q = N.data();
	for (int j = 0; j < n; ++j)
	    for (int i = 0; i < m; ++i) 
		assert(N[i][j] == *q++);
    }
}

static void testCheckedAccess()
{
    cout <<"Test access via operator () " << std::endl;
    
    int m = 4, n = 13;
    vector<int> D(1024);
    int * data = &(D.front());
    
    {
	TMatrixData<int> M(m,n,data,TMatrixData<int>::ROW_MAJOR);
	randMatrix(M);
	int * p = M.data();
	for (int i = 0; i < m; ++i) 
	    for (int j = 0; j < n; ++j)
		assert(M(i,j) == *p++);
	
	const TMatrixData<int> & N = M;
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
	    cout <<"  bound check" << endl;
	}
    }
    {
	TMatrixData<int> M(m,n,data,TMatrixData<int>::COLUMN_MAJOR);
	randMatrix(M);
	int * p = M.data();
	for (int j = 0; j < n; ++j)
	    for (int i = 0; i < m; ++i) 
		assert(M(i,j) == *p++);
	
	const TMatrixData<int> & N = M;
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
	    cout <<"  bound check" << endl;
	}
    }
}


static void testIterators()
{
    cout << "Testing iterators" << endl;
    
    int m = 2, n = 5;
    vector<int> D(1024);
    int * data = &(D.front());
    
    {
	cout <<"  iterator" << endl;
	TMatrixData<int> M(m,n,data,TMatrixData<int>::ROW_MAJOR);
	randMatrix(M);
	
	assert(&(*(M.begin())) == M.data());
	assert(std::distance(M.begin(), M.end()) == M.size());

	int * p = M.data();
	TMatrixData<int>::iterator it = M.begin(), ie = M.end();
	while (it != ie)
	    assert(*it++ == *p++);
		
	cout <<"  const_iterator" << endl;
	
	const TMatrixData<int> & N = M;
	
	assert(&(*(N.begin())) == N.data());
	assert(std::distance(N.begin(), N.end()) == N.size());

	const int * q = N.data();
	TMatrixData<int>::const_iterator cit = N.begin(), cie = N.end();
	while (cit != cie)
	    assert(*cit++ == *q++);
    }
    {
	cout <<"  iterator" << endl;
		
	TMatrixData<int> M(m,n,data,TMatrixData<int>::COLUMN_MAJOR);
	randMatrix(M);

	assert(&(*(M.begin())) == M.data());
	assert(std::distance(M.begin(), M.end()) == M.size());

	int * p = M.data();
	TMatrixData<int>::iterator it = M.begin(), ie = M.end();
	while (it != ie)
	    assert(*it++ == *p++);
	
	cout <<"  const_iterator" << endl;
		
	const TMatrixData<int> & N = M;

	assert(&(*(N.begin())) == N.data());
	assert(std::distance(N.begin(), N.end()) == N.size());

	const int * q = N.data();
	TMatrixData<int>::const_iterator cit = N.begin(), cie = N.end();
	while (cit != cie)
	    assert(*cit++ == *q++);
    }
}

static void testTranspose()
{
    std::cout << "Testing transposition" << endl;

    int m = 7, n = 17;
    vector<int> D(1024);
    int * data = &(D.front());
    
    {
	std::cout <<"  in place" << endl;
	
	TMatrixData<int> M(m,n,data,TMatrix<int>::ROW_MAJOR);
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
    }
    {
	std::cout <<"  in place" << endl;
	
	TMatrixData<int> M(m,n,data,TMatrix<int>::COLUMN_MAJOR);
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
    }
}

static void testSetOrder()
{
    std::cout << "Testing set order" << endl;

    int m = 8, n = 3;
    vector<int> D(1024);
    int * data = &(D.front());
    
    {
	TMatrixData<int> M(m,n,data,TMatrix<int>::ROW_MAJOR);
	randMatrix(M);
	
	TMatrix<int> N(M);
	
	std::cout <<"  idempotence" << endl;
		
	M.setOrder(TMatrixData<int>::ROW_MAJOR);
	
	assert(M.numRows() == N.numRows());
	assert(N.numColumns() == M.numColumns());
	assert(M.size() == N.size());
	assert(M.order() == N.order());
	assert(M.rowStride() == N.rowStride());
	assert(M.columnStride() == N.columnStride());
		
	for (int i = M.numRows()-1; i >= 0; --i)
	    for (int j = M.numColumns()-1 ; j >= 0; --j)
		assert(M(i,j) == N(i,j));

	std::cout <<"  order change" << endl;
	
	M.setOrder(TMatrixData<int>::COLUMN_MAJOR);
		
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
	TMatrixData<int> M(m,n,data,TMatrix<int>::COLUMN_MAJOR);
	randMatrix(M);
	
	TMatrix<int> N(M);

	std::cout <<"  idempotence" << endl;
		
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

	std::cout <<"  order change" << endl;

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


void testTMatrixData()
{
    cout << "Testing TMatrixData" << endl;
    try
    {
	testConstructorsAndAccessors();
	testUncheckedAccess();
	testCheckedAccess();
	testTranspose();
	testSetOrder();
    }
    catch ( Iex::BaseExc & e )
    {
	cout << "Trapped exception: " << e.what() << endl;
	throw;
    }
    cout << "ok" << endl << endl;
}
