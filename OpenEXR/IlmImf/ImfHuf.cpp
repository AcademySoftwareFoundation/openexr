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




//-----------------------------------------------------------------------------
//
//	16-bit Huffman compression and decompression.
//
//	The source code in this file is derived from the 8-bit
//	Huffman compression and decompression routines written
//	by Christian Rouet for his PIZ image file format.
//
//-----------------------------------------------------------------------------

#include <ImfHuf.h>
#include <ImfInt64.h>
#include <ImfAutoArray.h>
#include <Iex.h>
#include <string.h>
#include <assert.h>
#include <algorithm>


using namespace std;
using namespace Iex;

namespace Imf {
namespace {


const int HUF_ENCBITS = 16;			// literal (value) bit length
const int HUF_DECBITS = 14;			// decoding bit size (>= 8)

const int HUF_ENCSIZE = (1 << HUF_ENCBITS) + 1;	// encoding table size
const int HUF_DECSIZE =  1 << HUF_DECBITS;	// decoding table size
const int HUF_DECMASK = HUF_DECSIZE - 1;


struct HufDec
{				// short code		long code
				//-------------------------------
    int		len:8;		// code length		0	 
    int		lit:24;		// lit			p size	 
    int	*	p;		// 0			lits	 
};


void
tooMuchData ()
{
    throw InputExc ("Error in Huffman-encoded data "
		    "(decoded data are longer than expected).");
}


void
notEnoughData ()
{
    throw InputExc ("Error in Huffman-encoded data "
		    "(decoded data are shorter than expected).");
}


void
invalidCode ()
{
    throw InputExc ("Error in Huffman-encoded data (invalid code).");
}


void
invalidTableSize ()
{
    throw InputExc ("Error in Huffman-encoded data (invalid code table size).");
}


inline Int64
hufLength (Int64 code)
{
    return code & 63;
}


inline Int64
hufCode (Int64 code)
{
    return code >> 6;
}


inline void
outputBits (int nBits, Int64 bits, Int64 &c, int &lc, char *&out)
{
    c <<= nBits;
    lc += nBits;

    c |= bits;

    while (lc >= 8)
	*out++ = (c >> (lc -= 8));
}


inline Int64
getBits (int nBits, Int64 &c, int &lc, const char *&in)
{
    while (lc < nBits)
    {
	c = (c << 8) | *(unsigned char *)(in++);
	lc += 8;
    }

    lc -= nBits;
    return (c >> lc) & ((1 << nBits) - 1);
}


//
// ENCODING TABLE BUILDING & (UN)PACKING
//

//
// Build a "canonical" Huffman code table:
//	- for each (uncompressed) symbol, hcode contains the length
//	  of the corresponding code (in the compressed data)
//	- canonical codes are computed and stored in hcode
//	- the rules for constructing canonical codes are as follows:
//	  * shorter codes (if filled with zeroes to the right)
//	    have a numerically higher value than longer codes
//	  * for codes with the same length, numerical values
//	    increase with numerical symbol values
//	- because the canonical code table can be constructed from
//	  symbol lengths alone, the code table can be transmitted
//	  without sending the actual code values
//	- see http://www.compressconsult.com/huffman/
//

void
hufCanonicalCodeTable (Int64 hcode[HUF_ENCSIZE])
{
    Int64 n[58];

    for (int i = 0; i < 58; ++i)
	n[i] = 0;

    for (int i = 0; i < HUF_ENCSIZE; ++i)
	n[hcode[i]] += 1;

    Int64 c = 0;

    for (int i = 57; i > 0; --i)
    {
	Int64 nc = ((c + n[i]) >> 1);
	n[i] = c;
	c = nc;
    }

    for (int i = 0; i < HUF_ENCSIZE; ++i)
    {
	int l = hcode[i];

	if (l > 0)
	    hcode[i] = l | (n[l]++ << 6);
    }
}


//
// Compute Huffman codes (based on frq input) and store them in frq:
//	- code structure is : [63:lsb - 6:msb] | [5-0: bit length];
//	- max code length is 58 bits;
//	- codes outside the range [im-iM] have a null length (unused values);
//	- original frequencies are destroyed;
//	- encoding tables are used by hufEncode() and hufBuildDecTable();
//


struct FHeapCompare
{
    bool operator () (Int64 *a, Int64 *b) {return *a > *b;}
};


void
hufBuildEncTable
    (Int64*	frq,	// io: input frequencies [HUF_ENCSIZE], output table
     int*	im,	//  o: min frq index
     int*	iM)	//  o: max frq index
{
    //
    // Find min/max indices in frq, mark leaves
    //

    static const int LEAF  = 0x80000;
    static const int INDEX = 0x7ffff;

    // Temporary table dependencies,
    // and heap for quickly finding min frq value

    AutoArray <int, HUF_ENCSIZE> hlink;
    AutoArray <Int64 *, HUF_ENCSIZE> fHeap;

    *im= 0;

    while (!frq[*im])
	(*im)++;

    int nf = 0;

    for (int i = *im; i < HUF_ENCSIZE; i++)
    {
	hlink[i]= LEAF | i;

	if (frq[i])
	{
	    fHeap[nf] = &frq[i];
	    nf++;
	    *iM= i;
	}
    }

    (*iM)++;	// add entry for run-length code
    frq[*iM]= 1;
    fHeap[nf] = &frq[*iM];
    nf++;

    make_heap (&fHeap[0], &fHeap[nf], FHeapCompare());

    //
    // Init scode & loop on all values
    //

    //
    // Temporary encoding table
    //

    AutoArray <Int64, HUF_ENCSIZE> scode;

    memset (scode, 0, sizeof (Int64) * HUF_ENCSIZE);

    while (nf > 1)
    {
	//
	// Find the indices, mm and m, of the two smallest non-zero frq
	// values in fHeap, add the smallest frq to the second-smallest
	// frq, and remove the smallest frq value from fHeap.
	//

	int mm = fHeap[0] - frq;
	pop_heap (&fHeap[0], &fHeap[nf], FHeapCompare());
	--nf;

	int m = fHeap[0] - frq;
	pop_heap (&fHeap[0], &fHeap[nf], FHeapCompare());

	frq[m ] += frq[mm];
	push_heap (&fHeap[0], &fHeap[nf], FHeapCompare());

	hlink[mm] &= ~LEAF;	// unmark min value

	//
	// Add 1 bit to all lower list codes
	//

	for (int j = m; true; j = hlink[j] & INDEX)
	{
	    scode[j]++;

	    assert (scode[j] <= 58);

	    if ((hlink[j] & INDEX) == j)
	    {
		//
		// Merge upper & lower lists
		//

		hlink[j] &= ~INDEX;
		hlink[j] |= mm;
		break;
	    }
	}

	//
	// Add 1 bit to all lower list codes
	//

	for (int j = mm; true; j = hlink[j] & INDEX)
	{
	    scode[j]++;

	    assert (scode[j] <= 58);

	    if ((hlink[j] & INDEX) == j)
		break;
	}
    }

    hufCanonicalCodeTable (scode);

    memcpy (frq, scode, sizeof (Int64) * HUF_ENCSIZE);	// overwrite frq
}


//
// Pack an encoding table:
//	- only code lengths, not actual codes, are stored
//	- runs of zeroes are compressed as follows:
//
//	  unpacked		packed
//	  --------------------------------
//	  1 zero		0	(6 bits)
//	  2 zeroes		59
//	  3 zeroes		60
//	  4 zeroes		61
//	  5 zeroes		62
//	  n zeroes (6 or more)	63 n-6	(6 + 8 bits)
//

const int SHORT_ZEROCODE_RUN = 59;
const int LONG_ZEROCODE_RUN  = 63;
const int SHORTEST_LONG_RUN  = 2 + LONG_ZEROCODE_RUN - SHORT_ZEROCODE_RUN;
const int LONGEST_LONG_RUN   = 255 + SHORTEST_LONG_RUN;


void
hufPackEncTable
    (const Int64*	hcode,		// i : encoding table [HUF_ENCSIZE]
     int		im,		// i : min hcode index
     int		iM,		// i : max hcode index
     char**		pcode)		//  o: ptr to packed table (updated)
{
    char *p = *pcode;
    Int64 c = 0;
    int lc = 0;

    for (; im <= iM; im++)
    {
	int l = hufLength (hcode[im]);

	if (l == 0)
	{
	    int zerun = 1;

	    while ((im < iM) && (zerun < LONGEST_LONG_RUN))
	    {
		if (hufLength (hcode[im+1]) > 0 )	 
		    break;
		im++;
		zerun++;
	    }

	    if (zerun >= 2)
	    {
		if (zerun >= SHORTEST_LONG_RUN)
		{
		    outputBits (6, LONG_ZEROCODE_RUN, c, lc, p);
		    outputBits (8, zerun - SHORTEST_LONG_RUN, c, lc, p);
		}
		else
		{
		    outputBits (6, SHORT_ZEROCODE_RUN + zerun - 2, c, lc, p);
		}
		continue;
	    }
	}

	outputBits (6, l, c, lc, p);
    }

    if (lc > 0)
	*p++ = (unsigned char) (c << (8 - lc));

    *pcode = p;
}


//
// Unpack an encoding table packed by hufPackEncTable():
//

void
hufUnpackEncTable
    (const char**	pcode,		// io: ptr to packed table (updated)
     int		im,		// i : min hcode index
     int		iM,		// i : max hcode index
     Int64*		hcode)		//  o: encoding table [HUF_ENCSIZE]
{
    memset (hcode, 0, sizeof (Int64) * HUF_ENCSIZE);

    const char *p = *pcode;
    Int64 c = 0;
    int lc = 0;

    for (; im <= iM; im++)
    {
	Int64 l = hcode[im] = getBits (6, c, lc, p); // code length

	if (l == (Int64) LONG_ZEROCODE_RUN)
	{
	    int zerun = getBits (8, c, lc, p) + SHORTEST_LONG_RUN;

	    while (zerun--)
		hcode[im++] = 0;

	    im--;
	}
	else if (l >= (Int64) SHORT_ZEROCODE_RUN)
	{
	    int zerun = l - SHORT_ZEROCODE_RUN + 2;

	    while (zerun--)
		hcode[im++] = 0;

	    im--;
	}
    }

    *pcode = (char *) p;

    hufCanonicalCodeTable (hcode);
}


//
// DECODING TABLE BUILDING
//

//
// Build a decoding hash table based on the encoding table hcode:
//	- short codes (<= HUF_DECBITS) are resolved with a single table access;
//	- long code entry allocations are not optimized, because long codes are
//	  unfrequent;
//	- decoding tables are used by hufDecode();
//

void
hufBuildDecTable
    (const Int64*	hcode,		// i : encoding table
     int		im,		// i : min index in hcode
     int		iM,		// i : max index in hcode
     HufDec *		hdecod)		//  o: (allocated by caller)
     					//     decoding table [HUF_DECSIZE]
{
    //
    // Init hashtable & loop on all codes
    //

    memset (hdecod, 0, sizeof (HufDec) * HUF_DECSIZE);

    for (; im <= iM; im++)
    {
	Int64 c = hufCode (hcode[im]);
	int l = hufLength (hcode[im]);

	if (l > HUF_DECBITS)
	{
	    //
	    // Long code: add a secondary entry
	    //

	    HufDec *pl = hdecod + (c >> (l - HUF_DECBITS));
	    pl->lit++;

	    if (pl->p)
	    {
		int *p = pl->p;
		pl->p = new int [pl->lit];

		for (int i = 0; i < pl->lit - 1; ++i)
		    pl->p[i] = p[i];

		delete [] p;
	    }
	    else
	    {
		pl->p = new int [1];
	    }

	    pl->p[pl->lit - 1]= im;
	}
	else if (l)
	{
	    //
	    // Short code: init all primary entries
	    //

	    HufDec *pl = hdecod + (c << (HUF_DECBITS - l));

	    for (Int64 i = 1 << (HUF_DECBITS - l); i > 0; i--, pl++)
	    {
		pl->len = l;
		pl->lit = im;
	    }
	}
    }
}


//
// Free the long code entries of a decoding table built by hufBuildDecTable()
//

void
hufFreeDecTable (HufDec *hdecod)	// io: Decoding table
{
    for (int i = 0; i < HUF_DECSIZE; i++)
    {
	if (hdecod[i].p)
	{
	    delete [] hdecod[i].p;
	    hdecod[i].p = 0;
	}
    }
}


//
// ENCODING
//

inline void
outputCode (Int64 code, Int64 &c, int &lc, char *&out)
{
    outputBits (hufLength (code), hufCode (code), c, lc, out);
}


inline void
sendCode (Int64 sCode, int runCount, Int64 runCode,
	  Int64 &c, int &lc, char *&out)
{
    static const int RLMIN = 32; // min count to activate run-length coding

    if (runCount > RLMIN)
    {
	outputCode (sCode, c, lc, out);
	outputCode (runCode, c, lc, out);
	outputBits (8, runCount, c, lc, out);
    }
    else
    {
	while (runCount-- >= 0)
	    outputCode (sCode, c, lc, out);
    }
}


//
// Encode (compress) ni values based on the Huffman encoding table hcode:
//

int
hufEncode				// return: output size (in bits)
    (const Int64*  	    hcode,	// i : encoding table
     const unsigned short*  in,		// i : uncompressed input buffer
     const int     	    ni,		// i : input buffer size (in bytes)
     int           	    rlc,	// i : rl code
     char*         	    out)	//  o: compressed output buffer
{
    char *outStart = out;
    Int64 c = 0;	// bits not yet written to out
    int lc = 0;		// number of valid bits in c (LSB)
    int s = in[0];
    int cs = 0;

    //
    // Loop on input values
    //

    for (int i = 1; i < ni; i++)
    {
	//
	// Count same values or send code
	//

	if (s == in[i] && cs < 255)
	{
	    cs++;
	}
	else
	{
	    sendCode (hcode[s], cs, hcode[rlc], c, lc, out);
	    cs=0;
	}

	s = in[i];
    }

    //
    // Send remaining code
    //

    sendCode (hcode[s], cs, hcode[rlc], c, lc, out);

    if (lc)
	*out = (c << (8 - lc)) & 0xff;

    return (out - outStart) * 8 + lc;
}


//
// DECODING
//

//
// In order to force the compiler to inline them,
// getChar() and getCode() are implemented as macros
// instead of "inline" functions.
//

#define getChar(c, lc, in)			\
{						\
    c = (c << 8) | *(unsigned char *)(in++);	\
    lc += 8;					\
}


#define getCode(po, rlc, c, lc, in, out, oe)	\
{						\
    if (po == rlc)				\
    {						\
	if (lc < 8)				\
	    getChar(c, lc, in);			\
						\
	lc -= 8;				\
						\
	unsigned char cs = (c >> lc);		\
						\
	if (out + cs > oe)			\
	    tooMuchData();			\
						\
	unsigned short s = out[-1];		\
						\
	while (cs-- > 0)			\
	    *out++ = s;				\
    }						\
    else if (out < oe)				\
    {						\
	*out++ = po;				\
    }						\
    else					\
    {						\
	tooMuchData();				\
    }						\
}


//
// Decode (uncompress) ni bits based on encoding & decoding tables:
//

void
hufDecode
    (const Int64 * 	hcode,	// i : encoding table
     const HufDec * 	hdecod,	// i : decoding table
     const char* 	in,	// i : compressed input buffer
     int		ni,	// i : input size (in bits)
     int		rlc,	// i : run-length code
     int		no,	// i : expected output size (in bytes)
     unsigned short*	out)	//  o: uncompressed output buffer
{
    Int64 c = 0;
    int lc = 0;
    unsigned short * outb = out;
    unsigned short * oe = out + no;
    const char * ie = in + (ni + 7) / 8; // input byte size

    //
    // Loop on input bytes
    //

    while (in < ie)
    {
	getChar (c, lc, in);

	//
	// Access decoding table
	//

	while (lc >= HUF_DECBITS)
	{
	    const HufDec pl = hdecod[(c >> (lc-HUF_DECBITS)) & HUF_DECMASK];

	    if (pl.len)
	    {
		//
		// Get short code
		//

		lc -= pl.len;
		getCode (pl.lit, rlc, c, lc, in, out, oe);
	    }
	    else
	    {
		if (!pl.p)
		    invalidCode(); // wrong code

		//
		// Search long code
		//

		int j;

		for (j = 0; j < pl.lit; j++)
		{
		    int	l = hufLength (hcode[pl.p[j]]);

		    while (lc < l && in < ie)	// get more bits
			getChar (c, lc, in);

		    if (lc >= l)
		    {
			if (hufCode (hcode[pl.p[j]]) ==
				((c >> (lc - l)) & ((Int64(1) << l) - 1)))
			{
			    //
			    // Found : get long code
			    //

			    lc -= l;
			    getCode (pl.p[j], rlc, c, lc, in, out, oe);
			    break;
			}
		    }
		}

		if (j == pl.lit)
		    invalidCode(); // Not found
	    }
	}
    }

    //
    // Get remaining (short) codes
    //

    int i = (8 - ni) & 7;
    c >>= i;
    lc -= i;

    while (lc > 0)
    {
	const HufDec pl = hdecod[(c << (HUF_DECBITS - lc)) & HUF_DECMASK];

	if (pl.len)
	{
	    lc -= pl.len;
	    getCode (pl.lit, rlc, c, lc, in, out, oe);
	}
	else
	{
	    invalidCode(); // wrong (long) code
	}
    }

    if (out - outb != no)
	notEnoughData ();
}


void
countFrequencies (Int64 freq[HUF_ENCSIZE],
		  const unsigned short data[/*n*/],
		  int n)
{
    for (int i = 0; i < HUF_ENCSIZE; ++i)
	freq[i] = 0;

    for (int i = 0; i < n; ++i)
	++freq[data[i]];
}


void
writeUInt (char buf[4], unsigned int i)
{
    unsigned char *b = (unsigned char *) buf;

    b[0] = i;
    b[1] = i >> 8;
    b[2] = i >> 16;
    b[3] = i >> 24;
}


unsigned int
readUInt (const char buf[4])
{
    const unsigned char *b = (const unsigned char *) buf;

    return ( b[0]        & 0x000000ff) |
	   ((b[1] <<  8) & 0x0000ff00) |
	   ((b[2] << 16) & 0x00ff0000) |
	   ((b[3] << 24) & 0xff000000);
}

} // namespace


//
// EXTERNAL INTERFACE
//


int
hufCompress (const unsigned short raw[],
	     int nRaw,
	     char compressed[])
{
    if (nRaw == 0)
	return 0;

    AutoArray <Int64, HUF_ENCSIZE> freq;

    countFrequencies (freq, raw, nRaw);

    int im, iM;
    hufBuildEncTable (freq, &im, &iM);

    char *tableStart = compressed + 20;
    char *tableEnd   = tableStart;
    hufPackEncTable (freq, im, iM, &tableEnd);
    int tableLength = tableEnd - tableStart;

    char *dataStart = tableEnd;
    int nBits = hufEncode (freq, raw, nRaw, iM, dataStart);
    int dataLength = (nBits + 7) / 8;

    writeUInt (compressed,      im);
    writeUInt (compressed +  4, iM);
    writeUInt (compressed +  8, tableLength);
    writeUInt (compressed + 12, nBits);
    writeUInt (compressed + 16, 0);	// room for future extensions

    return dataStart + dataLength - compressed;
}


void
hufUncompress (const char compressed[],
	       int nCompressed,
	       unsigned short raw[],
	       int nRaw)
{
    if (nCompressed == 0)
    {
	if (nRaw != 0)
	    notEnoughData();

	return;
    }

    int im = readUInt (compressed);
    int iM = readUInt (compressed + 4);
    // int tableLength = readUInt (compressed + 8);
    int nBits = readUInt (compressed + 12);

    if (im < 0 || im >= HUF_ENCSIZE || iM < 0 || iM >= HUF_ENCSIZE)
	invalidTableSize();

    compressed += 20;

    AutoArray <Int64, HUF_ENCSIZE> freq;
    AutoArray <HufDec, HUF_DECSIZE> hdec;

    hufUnpackEncTable (&compressed, im, iM, freq);

    try
    {
	hufBuildDecTable (freq, im, iM, hdec);
	hufDecode (freq, hdec, compressed, nBits, iM, nRaw, raw);
    }
    catch (...)
    {
	hufFreeDecTable (hdec);
	throw;
    }

    hufFreeDecTable (hdec);
}


} // namespace Imf
