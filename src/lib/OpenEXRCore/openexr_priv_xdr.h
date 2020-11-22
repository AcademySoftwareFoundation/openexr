/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_XDR_H
#define OPENEXR_PRIVATE_XDR_H

#include <endian.h>

static inline uint64_t one_to_native64( uint64_t v )
{
    return le64toh( v );
}

static inline uint64_t one_from_native64( uint64_t v )
{
    return htole64( v );
}

static inline void priv_to_native64( void *ptr, int n )
{
#if __BYTE_ORDER != __LITTLE_ENDIAN
    uint64_t *vals = (uint64_t *)ptr;
    for ( int i = 0; i < n; ++i )
        vals[i] = le64toh( vals[i] );
#endif
}

static inline void priv_from_native64( void *ptr, int n )
{
#if __BYTE_ORDER != __LITTLE_ENDIAN
    uint64_t *vals = (uint64_t *)ptr;
    for ( int i = 0; i < n; ++i )
        vals[i] = htole64( vals[i] );
#endif
}

/**************************************/

static inline uint32_t one_to_native32( uint32_t v )
{
    return le32toh( v );
}

static inline uint32_t one_from_native32( uint32_t v )
{
    return htole32( v );
}

static inline void priv_to_native32( void *ptr, int n )
{
#if __BYTE_ORDER != __LITTLE_ENDIAN
    uint32_t *vals = (uint32_t *)ptr;
    for ( int i = 0; i < n; ++i )
        vals[i] = le32toh( vals[i] );
#endif
}

static inline void priv_from_native32( void *ptr, int n )
{
#if __BYTE_ORDER != __LITTLE_ENDIAN
    uint32_t *vals = (uint32_t *)ptr;
    for ( int i = 0; i < n; ++i )
        vals[i] = htole32( vals[i] );
#endif
}

/**************************************/

static inline uint16_t one_to_native16( uint16_t v )
{
    return le16toh( v );
}

static inline uint16_t one_from_native16( uint16_t v )
{
    return htole16( v );
}

static inline void priv_to_native16( void *ptr, int n )
{
#if __BYTE_ORDER != __LITTLE_ENDIAN
    uint16_t *vals = (uint16_t *)ptr;
    for ( int i = 0; i < n; ++i )
        vals[i] = le16toh( vals[i] );
#endif
}

static inline void priv_from_native16( void *ptr, int n )
{
#if __BYTE_ORDER != __LITTLE_ENDIAN
    uint16_t *vals = (uint16_t *)ptr;
    for ( int i = 0; i < n; ++i )
        vals[i] = htole16( vals[i] );
#endif
}

/**************************************/

static inline void priv_to_native( void *ptr, int n, size_t eltsize )
{
    if ( eltsize == 8 )
        priv_to_native64( ptr, n );
    else if ( eltsize == 4 )
        priv_to_native32( ptr, n );
    else if ( eltsize == 2 )
        priv_to_native16( ptr, n );
}

static inline void priv_from_native( void *ptr, int n, size_t eltsize )
{
    if ( eltsize == 8 )
        priv_from_native64( ptr, n );
    else if ( eltsize == 4 )
        priv_from_native32( ptr, n );
    else if ( eltsize == 2 )
        priv_from_native16( ptr, n );
}

#endif /* OPENEXR_PRIVATE_XDR_H */
