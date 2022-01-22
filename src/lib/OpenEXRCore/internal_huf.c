/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_huf.h"

#include "internal_memory.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define HUF_ENCBITS 16
#define HUF_DECBITS 14

#define HUF_ENCSIZE ((1 << HUF_ENCBITS) + 1)
#define HUF_DECSIZE (1 << HUF_DECBITS)
#define HUF_DECMASK (HUF_DECSIZE - 1)

typedef struct _HufDec
{
    int32_t   len;
    uint32_t  lit;
    uint32_t* p;
} HufDec;

/**************************************/

static inline int
hufLength (uint64_t code)
{
    return (int) (code & 63);
}

static inline uint64_t
hufCode (uint64_t code)
{
    return code >> 6;
}

static inline void
outputBits (int nBits, uint64_t bits, uint64_t* c, int* lc, uint8_t** outptr)
{
    uint8_t* out = *outptr;
    *c <<= nBits;
    *lc += nBits;
    *c |= bits;

    while (*lc >= 8)
        *out++ = (uint8_t) (*c >> (*lc -= 8));
    *outptr = out;
}

static inline uint64_t
getBits (uint32_t nBits, uint64_t* c, uint32_t* lc, const uint8_t** inptr)
{
    const uint8_t* in = *inptr;
    while (*lc < nBits)
    {
        *c = (*c << 8) | (uint64_t) (*in++);
        *lc += 8;
    }

    *inptr = in;
    *lc -= nBits;
    return (*c >> *lc) & ((1 << nBits) - 1);
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

static void
hufCanonicalCodeTable (uint64_t* hcode)
{
    uint64_t n[59];

    //
    // For each i from 0 through 58, count the
    // number of different codes of length i, and
    // store the count in n[i].
    //

    for (int i = 0; i <= 58; ++i)
        n[i] = 0;

    for (int i = 0; i < HUF_ENCSIZE; ++i)
        n[hcode[i]] += 1;

    //
    // For each i from 58 through 1, compute the
    // numerically lowest code with length i, and
    // store that code in n[i].
    //

    uint64_t c = 0;

    for (int i = 58; i > 0; --i)
    {
        uint64_t nc = ((c + n[i]) >> 1);
        n[i]        = c;
        c           = nc;
    }

    //
    // hcode[i] contains the length, l, of the
    // code for symbol i.  Assign the next available
    // code of length l to the symbol and store both
    // l and the code in hcode[i].
    //

    for (int i = 0; i < HUF_ENCSIZE; ++i)
    {
        uint64_t l = hcode[i];

        if (l > 0) hcode[i] = l | (n[l]++ << 6);
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
// NB: The following code "(*a == *b) && (a > b))" was added to ensure
//     elements in the heap with the same value are sorted by index.
//     This is to ensure, the STL make_heap()/pop_heap()/push_heap() methods
//     produced a resultant sorted heap that is identical across OSes.
//

static inline int
FHeapCompare (uint64_t* a, uint64_t* b)
{
    return ((*a > *b) || ((*a == *b) && (a > b)));
}

static inline void
intern_push_heap (
    uint64_t** first, size_t holeIndex, size_t topIndex, uint64_t* value)
{
    size_t parent = (holeIndex - 1) / 2;
    while (holeIndex > topIndex && FHeapCompare (*(first + parent), value))
    {
        *(first + holeIndex) = *(first + parent);
        holeIndex            = parent;
        parent               = (holeIndex - 1) / 2;
    }
    *(first + holeIndex) = value;
}

static inline void
adjust_heap (uint64_t** first, size_t holeIndex, size_t len, uint64_t* value)
{
    const size_t topIndex    = holeIndex;
    size_t       secondChild = holeIndex;

    while (secondChild < (len - 1) / 2)
    {
        secondChild = 2 * (secondChild + 1);
        if (FHeapCompare (*(first + secondChild), *(first + (secondChild - 1))))
            --secondChild;
        *(first + holeIndex) = *(first + secondChild);
        holeIndex            = secondChild;
    }

    if ((len & 1) == 0 && secondChild == (len - 2) / 2)
    {
        secondChild          = 2 * (secondChild + 1);
        *(first + holeIndex) = *(first + (secondChild - 1));
        holeIndex            = secondChild - 1;
    }

    intern_push_heap (first, holeIndex, topIndex, value);
}

static inline void
push_heap (uint64_t** first, uint64_t** last)
{
    uint64_t* value = *(last - 1);
    intern_push_heap (first, (size_t) (last - first) - 1, 0, value);
}

static inline void
intern_pop_heap (uint64_t** first, uint64_t** last, uint64_t** result)
{
    uint64_t* value = *result;
    *result         = *first;
    adjust_heap (first, 0, (size_t) (last - first), value);
}

static inline void
pop_heap (uint64_t** first, uint64_t** last)
{
    if (last - first > 1)
    {
        --last;
        intern_pop_heap (first, last, last);
    }
}

static void
make_heap (uint64_t** first, uint64_t len)
{
    size_t parent;

    if (len < 2) return;
    parent = (len - 2) / 2;

    while (1)
    {
        uint64_t* value = *(first + parent);
        adjust_heap (first, parent, len, value);
        if (parent == 0) return;
        --parent;
    }
}

static void
hufBuildEncTable (
    uint64_t*  frq,
    uint32_t*  im,
    uint32_t*  iM,
    uint32_t*  hlink,
    uint64_t** fHeap,
    uint64_t*  scode)
{
    //
    // This function assumes that when it is called, array frq
    // indicates the frequency of all possible symbols in the data
    // that are to be Huffman-encoded.  (frq[i] contains the number
    // of occurrences of symbol i in the data.)
    //
    // The loop below does three things:
    //
    // 1) Finds the minimum and maximum indices that point
    //    to non-zero entries in frq:
    //
    //     frq[im] != 0, and frq[i] == 0 for all i < im
    //     frq[iM] != 0, and frq[i] == 0 for all i > iM
    //
    // 2) Fills array fHeap with pointers to all non-zero
    //    entries in frq.
    //
    // 3) Initializes array hlink such that hlink[i] == i
    //    for all array entries.
    //

    *im = 0;

    while (!frq[*im])
        (*im)++;

    uint32_t nf = 0;

    for (uint32_t i = *im; i < HUF_ENCSIZE; i++)
    {
        hlink[i] = i;

        if (frq[i])
        {
            fHeap[nf] = &frq[i];
            ++nf;
            *iM = i;
        }
    }

    //
    // Add a pseudo-symbol, with a frequency count of 1, to frq;
    // adjust the fHeap and hlink array accordingly.  Function
    // hufEncode() uses the pseudo-symbol for run-length encoding.
    //

    (*iM)++;
    frq[*iM]  = 1;
    fHeap[nf] = &frq[*iM];
    ++nf;

    //
    // Build an array, scode, such that scode[i] contains the number
    // of bits assigned to symbol i.  Conceptually this is done by
    // constructing a tree whose leaves are the symbols with non-zero
    // frequency:
    //
    //     Make a heap that contains all symbols with a non-zero frequency,
    //     with the least frequent symbol on top.
    //
    //     Repeat until only one symbol is left on the heap:
    //
    //         Take the two least frequent symbols off the top of the heap.
    //         Create a new node that has first two nodes as children, and
    //         whose frequency is the sum of the frequencies of the first
    //         two nodes.  Put the new node back into the heap.
    //
    // The last node left on the heap is the root of the tree.  For each
    // leaf node, the distance between the root and the leaf is the length
    // of the code for the corresponding symbol.
    //
    // The loop below doesn't actually build the tree; instead we compute
    // the distances of the leaves from the root on the fly.  When a new
    // node is added to the heap, then that node's descendants are linked
    // into a single linear list that starts at the new node, and the code
    // lengths of the descendants (that is, their distance from the root
    // of the tree) are incremented by one.
    //

    make_heap (fHeap, nf);

    memset (scode, 0, sizeof (uint64_t) * HUF_ENCSIZE);

    while (nf > 1)
    {
        //
        // Find the indices, mm and m, of the two smallest non-zero frq
        // values in fHeap, add the smallest frq to the second-smallest
        // frq, and remove the smallest frq value from fHeap.
        //

        uint32_t mm = (uint32_t) (fHeap[0] - frq);
        pop_heap (&fHeap[0], &fHeap[nf]);
        --nf;

        uint32_t m = (uint32_t) (fHeap[0] - frq);
        pop_heap (&fHeap[0], &fHeap[nf]);

        frq[m] += frq[mm];
        push_heap (&fHeap[0], &fHeap[nf]);

        //
        // The entries in scode are linked into lists with the
        // entries in hlink serving as "next" pointers and with
        // the end of a list marked by hlink[j] == j.
        //
        // Traverse the lists that start at scode[m] and scode[mm].
        // For each element visited, increment the length of the
        // corresponding code by one bit. (If we visit scode[j]
        // during the traversal, then the code for symbol j becomes
        // one bit longer.)
        //
        // Merge the lists that start at scode[m] and scode[mm]
        // into a single list that starts at scode[m].
        //

        //
        // Add a bit to all codes in the first list.
        //

        for (uint32_t j = m;; j = hlink[j])
        {
            scode[j]++;

            if (hlink[j] == j)
            {
                //
                // Merge the two lists.
                //

                hlink[j] = mm;
                break;
            }
        }

        //
        // Add a bit to all codes in the second list
        //

        for (uint32_t j = mm;; j = hlink[j])
        {
            scode[j]++;

            if (hlink[j] == j) break;
        }
    }

    //
    // Build a canonical Huffman code table, replacing the code
    // lengths in scode with (code, code length) pairs.  Copy the
    // code table from scode into frq.
    //

    hufCanonicalCodeTable (scode);
    memcpy (frq, scode, sizeof (uint64_t) * HUF_ENCSIZE);
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

#define SHORT_ZEROCODE_RUN 59
#define LONG_ZEROCODE_RUN 63
#define SHORTEST_LONG_RUN (2 + LONG_ZEROCODE_RUN - SHORT_ZEROCODE_RUN)
#define LONGEST_LONG_RUN (255 + SHORTEST_LONG_RUN)

static void
hufPackEncTable (
    const uint64_t* hcode, // i : encoding table [HUF_ENCSIZE]
    uint32_t        im,    // i : min hcode index
    uint32_t        iM,    // i : max hcode index
    uint8_t**       pcode)       //  o: ptr to packed table (updated)
{
    uint8_t* out = *pcode;
    uint64_t c   = 0;
    int      lc  = 0;

    for (; im <= iM; im++)
    {
        int l = hufLength (hcode[im]);

        if (l == 0)
        {
            uint64_t zerun = 1;

            while ((im < iM) && (zerun < LONGEST_LONG_RUN))
            {
                if (hufLength (hcode[im + 1]) > 0) break;
                im++;
                zerun++;
            }

            if (zerun >= 2)
            {
                if (zerun >= SHORTEST_LONG_RUN)
                {
                    outputBits (6, LONG_ZEROCODE_RUN, &c, &lc, &out);
                    outputBits (8, zerun - SHORTEST_LONG_RUN, &c, &lc, &out);
                }
                else
                {
                    outputBits (
                        6, SHORT_ZEROCODE_RUN + zerun - 2, &c, &lc, &out);
                }
                continue;
            }
        }

        outputBits (6, (uint64_t) l, &c, &lc, &out);
    }

    if (lc > 0) *out++ = (uint8_t) (c << (8 - lc));

    *pcode = out;
}

//
// Unpack an encoding table packed by hufPackEncTable():
//

static exr_result_t
hufUnpackEncTable (
    const uint8_t** pcode, // io: ptr to packed table (updated)
    uint64_t*       nLeft, // io: input size (in bytes), bytes left
    uint32_t        im,    // i : min hcode index
    uint32_t        iM,    // i : max hcode index
    uint64_t*       hcode) // o : encoding table [HUF_ENCSIZE]
{
    memset (hcode, 0, sizeof (uint64_t) * HUF_ENCSIZE);

    const uint8_t* p  = *pcode;
    uint64_t       c  = 0;
    uint64_t       ni = *nLeft;
    uint64_t       nr;
    uint32_t       lc = 0;

    for (; im <= iM; im++)
    {
        nr = (((uintptr_t) p) - ((uintptr_t) *pcode));
        if (lc < 6 && nr >= ni) return EXR_ERR_OUT_OF_MEMORY;

        uint64_t l = hcode[im] = getBits (6, &c, &lc, &p); // code length

        if (l == (uint64_t) LONG_ZEROCODE_RUN)
        {
            nr = (((uintptr_t) p) - ((uintptr_t) *pcode));
            if (lc < 8 && nr >= ni) return EXR_ERR_OUT_OF_MEMORY;

            uint64_t zerun = getBits (8, &c, &lc, &p) + SHORTEST_LONG_RUN;

            if (im + zerun > iM + 1) return EXR_ERR_CORRUPT_CHUNK;

            while (zerun--)
                hcode[im++] = 0;

            im--;
        }
        else if (l >= (uint64_t) SHORT_ZEROCODE_RUN)
        {
            uint64_t zerun = l - SHORT_ZEROCODE_RUN + 2;

            if (im + zerun > iM + 1) return EXR_ERR_CORRUPT_CHUNK;

            while (zerun--)
                hcode[im++] = 0;

            im--;
        }
    }

    nr = (((uintptr_t) p) - ((uintptr_t) *pcode));
    *nLeft -= nr;
    *pcode = p;

    hufCanonicalCodeTable (hcode);
    return EXR_ERR_SUCCESS;
}

//
// DECODING TABLE BUILDING
//

//
// Clear a newly allocated decoding table so that it contains only zeroes.
//

static void
hufClearDecTable (HufDec* hdecod)
{
    memset (hdecod, 0, sizeof (HufDec) * HUF_DECSIZE);
}

//
// Build a decoding hash table based on the encoding table hcode:
//	- short codes (<= HUF_DECBITS) are resolved with a single table access;
//	- long code entry allocations are not optimized, because long codes are
//	  unfrequent;
//	- decoding tables are used by hufDecode();
//

static exr_result_t
hufBuildDecTable (
    const uint64_t* hcode, uint32_t im, uint32_t iM, HufDec* hdecod)
{
    //
    // Init hashtable & loop on all codes.
    // Assumes that hufClearDecTable(hdecod) has already been called.
    //

    for (; im <= iM; im++)
    {
        uint64_t c = hufCode (hcode[im]);
        int      l = hufLength (hcode[im]);

        if (c >> l)
        {
            //
            // Error: c is supposed to be an l-bit code,
            // but c contains a value that is greater
            // than the largest l-bit number.
            //

            return EXR_ERR_CORRUPT_CHUNK;
        }

        if (l > HUF_DECBITS)
        {
            //
            // Long code: add a secondary entry
            //

            HufDec* pl = hdecod + (c >> (l - HUF_DECBITS));

            if (pl->len)
            {
                //
                // Error: a short code has already
                // been stored in table entry *pl.
                //

                return EXR_ERR_CORRUPT_CHUNK;
            }

            pl->lit++;

            if (pl->p)
            {
                uint32_t* p = pl->p;
                pl->p       = (uint32_t*) internal_exr_alloc (
                    sizeof (uint32_t) * pl->lit);

                if (pl->p)
                {
                    for (uint32_t i = 0; i < pl->lit - 1; ++i)
                        pl->p[i] = p[i];
                }

                internal_exr_free (p);
            }
            else
            {
                pl->p = (uint32_t*) internal_exr_alloc (sizeof (uint32_t));
            }

            if (!pl->p) return EXR_ERR_OUT_OF_MEMORY;

            pl->p[pl->lit - 1] = im;
        }
        else if (l)
        {
            //
            // Short code: init all primary entries
            //

            HufDec* pl = hdecod + (c << (HUF_DECBITS - l));

            for (uint64_t i = 1 << (HUF_DECBITS - l); i > 0; i--, pl++)
            {
                if (pl->len || pl->p)
                {
                    //
                    // Error: a short code or a long code has
                    // already been stored in table entry *pl.
                    //

                    return EXR_ERR_CORRUPT_CHUNK;
                }

                pl->len = (int32_t) l;
                pl->lit = im;
            }
        }
    }
    return EXR_ERR_SUCCESS;
}

//
// Free the long code entries of a decoding table built by hufBuildDecTable()
//

static void
hufFreeDecTable (HufDec* hdecod) // io: Decoding table
{
    for (int i = 0; i < HUF_DECSIZE; i++)
    {
        if (hdecod[i].p)
        {
            internal_exr_free (hdecod[i].p);
            hdecod[i].p = NULL;
        }
    }
}

//
// ENCODING
//

static inline void
outputCode (uint64_t code, uint64_t* c, int* lc, uint8_t** out)
{
    outputBits (hufLength (code), hufCode (code), c, lc, out);
}

static inline void
sendCode (
    uint64_t  sCode,
    int       runCount,
    uint64_t  runCode,
    uint64_t* c,
    int*      lc,
    uint8_t** out)
{
    if (hufLength (sCode) + hufLength (runCode) + 8 <
        hufLength (sCode) * runCount)
    {
        outputCode (sCode, c, lc, out);
        outputCode (runCode, c, lc, out);
        outputBits (8, (uint64_t) runCount, c, lc, out);
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

static inline uint64_t
hufEncode (
    const uint64_t* hcode,
    const uint16_t* in,
    const uint64_t  ni,
    uint32_t        rlc,
    uint8_t*        out)
{
    uint8_t* outStart = out;
    uint64_t c        = 0; // bits not yet written to out
    int      lc       = 0; // number of valid bits in c (LSB)
    uint16_t s        = in[0];
    int      cs       = 0;

    //
    // Loop on input values
    //

    for (uint64_t i = 1; i < ni; i++)
    {
        //
        // Count same values or send code
        //

        if (s == in[i] && cs < 255) { cs++; }
        else
        {
            sendCode (hcode[s], cs, hcode[rlc], &c, &lc, &out);
            cs = 0;
        }

        s = in[i];
    }

    //
    // Send remaining code
    //

    sendCode (hcode[s], cs, hcode[rlc], &c, &lc, &out);

    if (lc) *out = (c << (8 - lc)) & 0xff;

    return (((uintptr_t) out) - ((uintptr_t) outStart)) * 8 + (uint64_t) (lc);
}

//
// DECODING
//

//
// In order to force the compiler to inline them,
// getChar() and getCode() are implemented as macros
// instead of "inline" functions.
//

#define getChar(c, lc, in)                                                     \
    c = (c << 8) | (uint64_t) (*in++);                                         \
    lc += 8

#define getCode(po, rlc, c, lc, in, ie, out, ob, oe)                           \
    do                                                                         \
    {                                                                          \
        if (po == rlc)                                                         \
        {                                                                      \
            if (lc < 8)                                                        \
            {                                                                  \
                if (in >= ie) return EXR_ERR_OUT_OF_MEMORY;                    \
                getChar (c, lc, in);                                           \
            }                                                                  \
                                                                               \
            lc -= 8;                                                           \
                                                                               \
            uint8_t cs = (uint8_t) (c >> lc);                                  \
                                                                               \
            if (out + cs > oe)                                                 \
                return EXR_ERR_CORRUPT_CHUNK;                                  \
            else if (out - 1 < ob)                                             \
                return EXR_ERR_OUT_OF_MEMORY;                                  \
                                                                               \
            uint16_t s = out[-1];                                              \
                                                                               \
            while (cs-- > 0)                                                   \
                *out++ = s;                                                    \
        }                                                                      \
        else if (out < oe)                                                     \
        {                                                                      \
            *out++ = (uint16_t) po;                                            \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return EXR_ERR_CORRUPT_CHUNK;                                      \
        }                                                                      \
    } while (0)

//
// Decode (uncompress) ni bits based on encoding & decoding tables:
//

static exr_result_t
hufDecode (
    const uint64_t* hcode,  // i : encoding table
    const HufDec*   hdecod, // i : decoding table
    const uint8_t*  in,     // i : compressed input buffer
    uint64_t        ni,     // i : input size (in bits)
    uint32_t        rlc,    // i : run-length code
    uint64_t        no,     // i : expected output size (count of uint16 items)
    uint16_t*       out)
{
    uint64_t       c    = 0;
    int            lc   = 0;
    uint16_t*      outb = out;
    uint16_t*      oe   = out + no;
    const uint8_t* ie   = in + (ni + 7) / 8; // input byte size

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
            uint64_t      decoffset = (c >> (lc - HUF_DECBITS)) & HUF_DECMASK;
            const HufDec* pl        = hdecod + decoffset;

            if (pl->len)
            {
                //
                // Get short code
                //

                if (pl->len > lc) return EXR_ERR_CORRUPT_CHUNK;

                lc -= pl->len;
                getCode (pl->lit, rlc, c, lc, in, ie, out, outb, oe);
            }
            else
            {
                uint32_t        j;
                const uint32_t* decbuf = pl->p;
                if (!pl->p) return EXR_ERR_CORRUPT_CHUNK; // wrong code

                //
                // Search long code
                //

                for (j = 0; j < pl->lit; j++)
                {
                    int l = hufLength (hcode[decbuf[j]]);

                    while (lc < l && in < ie) // get more bits
                    {
                        getChar (c, lc, in);
                    }

                    if (lc >= l)
                    {
                        if (hufCode (hcode[decbuf[j]]) ==
                            ((c >> (lc - l)) & (((uint64_t) (1) << l) - 1)))
                        {
                            //
                            // Found : get long code
                            //

                            lc -= l;
                            getCode (
                                decbuf[j], rlc, c, lc, in, ie, out, outb, oe);
                            break;
                        }
                    }
                }

                if (j == pl->lit) return EXR_ERR_CORRUPT_CHUNK;
            }
        }
    }

    //
    // Get remaining (short) codes
    //

    uint64_t i = (8 - ni) & 7;
    c >>= i;
    lc -= i;

    while (lc > 0)
    {
        uint64_t      decoffset = (c << (HUF_DECBITS - lc)) & HUF_DECMASK;
        const HufDec* pl        = hdecod + decoffset;

        if (pl->len)
        {
            if (pl->len > lc) return EXR_ERR_CORRUPT_CHUNK;
            lc -= pl->len;
            getCode (pl->lit, rlc, c, lc, in, ie, out, outb, oe);
        }
        else
            return EXR_ERR_CORRUPT_CHUNK;
    }

    if (out != oe) return EXR_ERR_OUT_OF_MEMORY;
    return EXR_ERR_SUCCESS;
}

static inline void
countFrequencies (uint64_t* freq, const uint16_t* data, uint64_t n)
{
    memset (freq, 0, HUF_ENCSIZE * sizeof (uint64_t));
    for (uint64_t i = 0; i < n; ++i)
        ++freq[data[i]];
}

static inline void
writeUInt (uint8_t* b, uint32_t i)
{
    b[0] = (uint8_t) (i);
    b[1] = (uint8_t) (i >> 8);
    b[2] = (uint8_t) (i >> 16);
    b[3] = (uint8_t) (i >> 24);
}

static inline uint32_t
readUInt (const uint8_t* b)
{
    return (
        ((uint32_t) b[0]) | (((uint32_t) b[1]) << 8u) |
        (((uint32_t) b[2]) << 16u) | (((uint32_t) b[3]) << 24u));
}

/**************************************/

uint64_t
internal_exr_huf_compress_spare_bytes (void)
{
    uint64_t ret = 0;
    ret += HUF_ENCSIZE * sizeof (uint64_t);  // freq
    ret += HUF_ENCSIZE * sizeof (int);       // hlink
    ret += HUF_ENCSIZE * sizeof (uint64_t*); // fheap
    ret += HUF_ENCSIZE * sizeof (uint64_t);  // scode
    return ret;
}

uint64_t
internal_exr_huf_decompress_spare_bytes (void)
{
    uint64_t ret = 0;
    ret += HUF_ENCSIZE * sizeof (uint64_t); // freq
    ret += HUF_DECSIZE * sizeof (HufDec);   // hdec
    //    ret += HUF_ENCSIZE * sizeof (uint64_t*); // fheap
    //    ret += HUF_ENCSIZE * sizeof (uint64_t);  // scode
    return ret;
}

exr_result_t
internal_huf_compress (
    uint64_t*       encbytes,
    void*           out,
    uint64_t        outsz,
    const uint16_t* raw,
    uint64_t        nRaw,
    void*           spare,
    uint64_t        sparebytes)
{
    uint64_t*  freq;
    uint32_t*  hlink;
    uint64_t** fHeap;
    uint64_t*  scode;
    uint32_t   im = 0;
    uint32_t   iM = 0;
    uint32_t   tableLength, nBits, dataLength;
    uint8_t*   dataStart;
    uint8_t*   compressed = (uint8_t*) out;
    uint8_t*   tableStart = compressed + 20;
    uint8_t*   tableEnd   = tableStart;

    if (nRaw == 0)
    {
        *encbytes = 0;
        return EXR_ERR_SUCCESS;
    }

    (void) outsz;
    if (sparebytes != internal_exr_huf_compress_spare_bytes ())
        return EXR_ERR_INVALID_ARGUMENT;

    freq  = (uint64_t*) spare;
    scode = freq + HUF_ENCSIZE;
    fHeap = (uint64_t**) (scode + HUF_ENCSIZE);
    hlink = (uint32_t*) (fHeap + HUF_ENCSIZE);

    countFrequencies (freq, raw, nRaw);

    hufBuildEncTable (freq, &im, &iM, hlink, fHeap, scode);

    hufPackEncTable (freq, im, iM, &tableEnd);

    tableLength =
        (uint32_t) (((uintptr_t) tableEnd) - ((uintptr_t) tableStart));
    dataStart = tableEnd;

    nBits      = (uint32_t) hufEncode (freq, raw, nRaw, iM, dataStart);
    dataLength = (nBits + 7) / 8;

    writeUInt (compressed, im);
    writeUInt (compressed + 4, iM);
    writeUInt (compressed + 8, tableLength);
    writeUInt (compressed + 12, nBits);
    writeUInt (compressed + 16, 0); // room for future extensions

    *encbytes =
        (((uintptr_t) dataStart) + ((uintptr_t) dataLength) -
         ((uintptr_t) compressed));
    return EXR_ERR_SUCCESS;
}

exr_result_t
internal_huf_decompress (
    const uint8_t* compressed,
    uint64_t       nCompressed,
    uint16_t*      raw,
    uint64_t       nRaw,
    void*          spare,
    uint64_t       sparebytes)
{
    uint32_t       im, iM, nBits;
    uint64_t       nBytes;
    const uint8_t* ptr;
    exr_result_t   rv;

    //
    // need at least 20 bytes for header
    //
    if (nCompressed < 20)
    {
        if (nRaw != 0) return EXR_ERR_INVALID_ARGUMENT;
        return EXR_ERR_SUCCESS;
    }

    if (sparebytes != internal_exr_huf_decompress_spare_bytes ())
        return EXR_ERR_INVALID_ARGUMENT;

    im = readUInt (compressed);
    iM = readUInt (compressed + 4);
    // uint32_t tableLength = readUInt (compressed + 8);
    nBits = readUInt (compressed + 12);
    // uint32_t future = readUInt (compressed + 16);

    if (im >= HUF_ENCSIZE || iM >= HUF_ENCSIZE) return EXR_ERR_CORRUPT_CHUNK;

    ptr = compressed + 20;

    nBytes = (((uint64_t) (nBits) + 7)) / 8;
    if (ptr + nBytes > compressed + nCompressed) return EXR_ERR_OUT_OF_MEMORY;

    //
    // Fast decoder needs at least 2x64-bits of compressed data, and
    // needs to be run-able on this platform. Otherwise, fall back
    // to the original decoder
    //
#if 0
    if (FastHufDecoder::enabled () && nBits > 128)
    {
        FastHufDecoder fhd (ptr, nCompressed - (ptr - compressed), im, iM, iM);

        // must be nBytes remaining in buffer
        if (ptr - compressed + nBytes > static_cast<uint64_t> (nCompressed))
        {
            notEnoughData ();
            return;
        }

        rv = fhd.decode (ptr, nBits, raw, nRaw);
    }
    else
#endif
    {
        uint64_t* freq     = (uint64_t*) spare;
        HufDec*   hdec     = (HufDec*) (freq + HUF_ENCSIZE);
        uint64_t  nLeft    = nCompressed - 20;
        uint64_t  nTableSz = 0;

        hufClearDecTable (hdec);
        hufUnpackEncTable (&ptr, &nLeft, im, iM, freq);

        if (nBits > 8 * nLeft) return EXR_ERR_CORRUPT_CHUNK;

        rv = hufBuildDecTable (freq, im, iM, hdec);
        if (rv == EXR_ERR_SUCCESS)
            rv = hufDecode (freq, hdec, ptr, nBits, iM, nRaw, raw);

        hufFreeDecTable (hdec);
    }
    return rv;
}
