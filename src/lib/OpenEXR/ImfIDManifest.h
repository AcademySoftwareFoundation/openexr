// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#ifndef INCLUDED_IMF_ID_MANIFEST_H
#define INCLUDED_IMF_ID_MANIFEST_H

//-----------------------------------------------------------------------------
//
//        class IDManifest, to store a table mapping ID numbers to text
//
//-----------------------------------------------------------------------------
#include "ImfNamespace.h"
#include "ImfInt64.h"
#include <map>
#include <vector>
#include <set>
#include <string>
#include "ImfForward.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class IDManifest
{
public:
           
    // indication of how long a mapping between an ID number and the text holds for
    typedef enum 
    {
        LIFETIME_FRAME, // The mapping may change every frame:
        LIFETIME_SHOT,  // The mapping is consistent for every frame of a shot
        LIFETIME_STABLE // The mapping is consistent for all time.
    } IdLifetime;

    //
    // hashing scheme is stored as a string rather than an enum, to allow
    // proprietary schemes to be encoded with less danger of collision
    // proprietary schemes should be encoded in a reverse-URL syntax
    //

    
    static const std::string UNKNOWN;        // = "unknown" : default value for encoding scheme and hash scheme - should be changed
    static const std::string NOTHASHED;      // = "none" : no relationship between text and ID
    static const std::string CUSTOMHASH;     // = "custom" : text is hashed using defined scheme
    static const std::string MURMURHASH3_32; // = "MurmurHash3_32" : MurmurHash3 32 bit is used
    static const std::string MURMURHASH3_64; // = "MurmurHash3_64" : bottom 8 bytes of MurmarHash3_128 (64 bit architecture version) is used


    static const std::string ID_SCHEME;      // ="id" : 32 bit ID stored directly in a UINT channel
    static const std::string ID2_SCHEME;     // ="id2" : 64 bit ID stored in two channels, specified by the ChannelGroup
    
    
    
    IDManifest();
    
    friend class CompressedIDManifest;
    
    //
    // decompress a compressed IDManifest into IDManifest for reading
    //
    IDManifest(const CompressedIDManifest&);
    
    //
    // construct manifest from serialized representation stored at 'data'
    //
    IDManifest(const char* data, const char* end);
    

private :
    // internal helper function called by constructors
    void init(const char* data,const char* end);
public :
    
    //
    // Description of the information represented by a single group of channels
    //
    class ChannelGroupManifest
    {
    private:
        std::set<std::string> _channels; // group of channels this manifest represents
        std::vector<std::string> _components; // ordered list of components represented by this channel group
        IdLifetime _lifeTime;
        std::string _hashScheme; //one of above strings or custom value e.g "nz.co.wetafx.cleverhash2"
        std::string _encodingScheme; //string identifying scheme to encode ID numbers within the image

        typedef std::map<Int64, std::vector<std::string> > IDTable;
        IDTable _table;

        // used for << operator to work: tracks the last item inserted into the Manifest 
        IDTable::iterator _insertionIterator; 
        bool _insertingEntry; // true if << has been called but not enough strings yet set

    public:
        
        ChannelGroupManifest();
        
        const std::set<std::string>& getChannels() const;
        std::set<std::string>& getChannels();
        
        void setChannels(const std::set<std::string>& channels);
        void setChannel(const std::string& channel);
        
         // get list of components for this channel group
         const std::vector<std::string>& getComponents() const;
         
         // set components: throws an exception if there are already entries in the table
         // and the component length changes
         void setComponents(const std::vector<std::string>& components);
         
         // set name of single component: throws an exception if there are already entries in the table
         // unless there was previously one component
         void setComponent(const std::string& component);
         
         
         IdLifetime getLifetime() const { return _lifeTime;}
         void setLifetime(const IdLifetime& lifeTime)      { _lifeTime = lifeTime;}
         
         const std::string& getHashScheme() const { return _hashScheme;}
         void setHashScheme(const std::string& hashScheme)             { _hashScheme = hashScheme;}
         
         const std::string& getEncodingScheme() const { return _encodingScheme;}
         void setEncodingScheme(const std::string& encodingScheme)             { _encodingScheme = encodingScheme;}
         
         
         class Iterator;          // iterator which allows modification of the text
         class ConstIterator;     // iterator which does not allow modification
         
         
         
         Iterator begin();
         ConstIterator begin() const;
         Iterator end();
         ConstIterator end() const;
         
         // return number of entries in manifest - could be 0
         size_t size() const ;
         
         // insert a new entry - text must contain same number of items as getComponents
         Iterator insert(Int64 idValue,  const std::vector<std::string>& text);
         
         // insert a new entry - getComponents must be a single entry
         Iterator insert(Int64 idValue,  const std::string& text);
         
         
         // compute hash of given entry, insert into manifest, and return 
         // the computed hash. Exception will be thrown if hash scheme isn't recognised
         Int64 insert(const std::vector<std::string>& text);
         Int64 insert(const std::string& text);
         
         Iterator find(Int64 idValue);
         ConstIterator find(Int64 idValue) const;
         void erase(Int64 idValue); 
         
         // return reference to idName for given idValue. Adds the mapping to the vector if it doesn't exist
         std::vector<std::string>& operator[](Int64 idValue); 
         
         // add a new entry to the manifest as an insertion operator: <<
         // the component strings must also be inserted using <<
         // throws an exception if the previous insert operation didn't insert the correct number of string components
         ChannelGroupManifest& operator<<(Int64 idValue);
         
         // insert a string as the next component of a previously inserted attribute
         ChannelGroupManifest& operator<<(const std::string& text);
         
         bool operator==(const ChannelGroupManifest& other) const;
         bool operator!=(const ChannelGroupManifest& other) const { return !(*this==other);}
         
         friend class IDManifest;
         
    };
    
   
    
private:
    std::vector<ChannelGroupManifest> _manifest;

public:

    // add a new channel group definition to the table, presumably populated with mappings
    // 'table' will be copied to the internal manifest; to further modify use the return value
    ChannelGroupManifest& add(const ChannelGroupManifest& table);
    
    
    //insert an empty table definition for the given channel / group of channels
    ChannelGroupManifest& add(const std::set<std::string>& group);
    ChannelGroupManifest& add(const std::string& channel);
    
 
    // return number of items in manifest
    size_t size() const;
    
    // find the first manifest ChannelGroupManifest that defines the given channel
    // if channel not find, returns a value equal to size()
    size_t find(const std::string& channel) const;

    const ChannelGroupManifest& operator[](size_t index) const;    
    ChannelGroupManifest& operator[](size_t index);
    
    //
    // serialize manifest into data array. Array will be resized to the required size
    //
    void serialize(std::vector<char>& data) const;
    
    bool operator==(const IDManifest& other) const;
    bool operator!=(const IDManifest& other) const;
    
    
    //
    // add entries from 'other' into this manifest if possible
    // * all ChannelGroupsManifests for different ChannelGroups
    //   will be appended.
    // * Where 'other' contains a manifest for the same
    //   ChannelGroup:
    //     * If _components differs, the entire ChannelGroupManifest is skipped
    //     * Otherwise, entries not present in 'this' will be inserted
    //     * _hashScheme, _lifeTime and _encodingScheme will be unchanged
    // 
    // returns 'false' if the same ChannelGroupManifest appears in both 'other' and 'this',
    // but with different _components, _hashScheme, _lifeTime or _encodingScheme
    // or if any idValue maps to different strings in 'other' and 'this'
    //
    bool merge(const IDManifest& other);
    
    
    //
    // static has generation functions
    //
    static unsigned int MurmurHash32(const std::string& idString);    
    static unsigned int MurmurHash32(const std::vector<std::string>& idString);
    
    static Int64 MurmurHash64(const std::string& idString);
    static Int64 MurmurHash64(const std::vector<std::string>& idString);
    

};


//
// zlip compressed version of IDManifest - the IDManifestAttribute encodes this format
// This should be transparent to the user, since there is implicit casting between the two types
//
class CompressedIDManifest
{
public:
    CompressedIDManifest();
    CompressedIDManifest(const CompressedIDManifest& other);
    
    CompressedIDManifest& operator=(const CompressedIDManifest& other);
    
    //
    // construct a compressed version of the given manifest - to decompress it cast to an IDManifest
    //
    CompressedIDManifest(const IDManifest& manifest);
    
   ~CompressedIDManifest();

    int _compressedDataSize;
    size_t _uncompressedDataSize;
    unsigned char* _data;

    
};


//
// Read/Write Iterator object to access individual entries within a manifest
//

class IDManifest::ChannelGroupManifest::Iterator
{
public:
    Iterator ();
    explicit Iterator (const IDManifest::ChannelGroupManifest::IDTable::iterator &i);
    
    friend class IDManifest::ChannelGroupManifest::ConstIterator;
    Iterator &                         operator ++ ();

    Int64                              id() const;
    std::vector<std::string>&          text();
    
private:
    std::map< Int64 , std::vector<std::string> >::iterator _i;
    
};

//
// Read-only Iterator object to access individual entries within a manifest
//


class IDManifest::ChannelGroupManifest::ConstIterator
{
public:
    ConstIterator ();

    // explicit cast from internal map operator (for internal use only)
    explicit ConstIterator (const IDManifest::ChannelGroupManifest::IDTable::const_iterator &i);
    // cast from non-const to const iterator
    ConstIterator (const IDManifest::ChannelGroupManifest::Iterator &other);
    ConstIterator &                         operator ++ ();

    Int64                              id() const;
    const std::vector<std::string>&          text() const;
    
    private:
        
    std::map< Int64 , std::vector<std::string> >::const_iterator _i;
    friend bool operator == (const ConstIterator &, const ConstIterator &);
    friend bool operator != (const ConstIterator &, const ConstIterator &);
};


//
// ChannelGroupManifest::Iterator implementation: all inline
//

inline IDManifest::ChannelGroupManifest::Iterator::Iterator() {}
inline IDManifest::ChannelGroupManifest::Iterator::Iterator(const IDManifest::ChannelGroupManifest::IDTable::iterator &i) :_i(i) {}


inline Int64
IDManifest::ChannelGroupManifest::Iterator::id() const { return _i->first;}

inline  std::vector<std::string>&
IDManifest::ChannelGroupManifest::Iterator::text() { return _i->second;}

inline IDManifest::ChannelGroupManifest::Iterator&
IDManifest::ChannelGroupManifest::Iterator::operator++()
{
    ++_i;
    return *this;
}

//
// ChannelGroupManifest::ConstIterator implementation: all inline
//

inline IDManifest::ChannelGroupManifest::ConstIterator::ConstIterator() {}
inline IDManifest::ChannelGroupManifest::ConstIterator::ConstIterator(const IDManifest::ChannelGroupManifest::Iterator &other) : _i(other._i) {}
inline IDManifest::ChannelGroupManifest::ConstIterator::ConstIterator(const IDManifest::ChannelGroupManifest::IDTable::const_iterator &i) :_i(i) {}

inline Int64
IDManifest::ChannelGroupManifest::ConstIterator::id() const { return _i->first;}

inline const  std::vector<std::string>&
IDManifest::ChannelGroupManifest::ConstIterator::text() const { return _i->second;}

inline IDManifest::ChannelGroupManifest::ConstIterator &
IDManifest::ChannelGroupManifest::ConstIterator::operator++()
{
    ++_i;
    return *this;
}

inline bool
operator==(const IDManifest::ChannelGroupManifest::ConstIterator& a, const IDManifest::ChannelGroupManifest::ConstIterator& b)
{
   return a._i ==b._i;
}

inline bool
operator!=(const IDManifest::ChannelGroupManifest::ConstIterator& a, const IDManifest::ChannelGroupManifest::ConstIterator& b)
{
   return a._i !=b._i;
}







OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT
#endif
