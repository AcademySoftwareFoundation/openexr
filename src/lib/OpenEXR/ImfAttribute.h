//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_ATTRIBUTE_H
#define INCLUDED_IMF_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class Attribute
//
//-----------------------------------------------------------------------------

#include <IexBaseExc.h>
#include "ImfIO.h"
#include "ImfXdr.h"
#include "ImfForward.h"
#include "ImfExport.h"
#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class IMF_EXPORT_VAGUELINKAGE Attribute
{
  public:

    //---------------------------
    // Constructor and destructor
    //---------------------------

    IMF_EXPORT Attribute ();
    IMF_EXPORT virtual ~Attribute ();


    //-------------------------------
    // Get this attribute's type name
    //-------------------------------

    virtual const char *	typeName () const = 0;


    //------------------------------
    // Make a copy of this attribute
    //------------------------------

    virtual Attribute *		copy () const = 0;


    //----------------------------------------
    // Type-specific attribute I/O and copying
    //----------------------------------------

    virtual void		writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os,
					      int version) const = 0;

    virtual void		readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is,
					       int size,
					       int version) = 0;

    virtual void		copyValueFrom (const Attribute &other) = 0;


    //------------------
    // Attribute factory
    //------------------

    IMF_EXPORT static Attribute *		newAttribute (const char typeName[]);


    //-----------------------------------------------------------
    // Test if a given attribute type has already been registered
    //-----------------------------------------------------------

    IMF_EXPORT static bool			knownType (const char typeName[]);

  protected:

    //--------------------------------------------------
    // Register an attribute type so that newAttribute()
    // knows how to make objects of this type.
    //--------------------------------------------------
    IMF_EXPORT
    static void		registerAttributeType (const char typeName[],
					       Attribute *(*newAttribute)());

    //------------------------------------------------------
    // Un-register an attribute type so that newAttribute()
    // no longer knows how to make objects of this type (for
    // debugging only).
    //------------------------------------------------------
    IMF_EXPORT
    static void		unRegisterAttributeType (const char typeName[]);
};

/// \defgroup TypeConversion type conversion helpers
///
/// We define these here for systems such as libc++ where the typeinfo
/// may not be the same between what is compiled into the library, and
/// the one used in code put into applications, especially with hidden
/// visibility enabled by default. The internal type checks as one
/// sets attributes into headers would then fail. As a result, we use
/// these where the types are more loosely defined to only include the
/// hash and name. With our custom, versioned namespace, this should
/// be safe, and allows us to have hidden visibility on symbols and
/// make a tidy shared object.
///
/// @{

template <typename U>
static U *dynamic_cast_attr (Attribute *a)
{
    if (!a)
        return nullptr;
    const auto &aid = typeid(*a);
    const auto &uid = typeid(U);
    // check the fast tests first before comparing names...
    if (aid == uid ||
        (aid.hash_code() == uid.hash_code() &&
         aid.name() == uid.name()))
    {
        return static_cast<U *>( a );
    }
    return nullptr;
}
template <typename U>
static const U *dynamic_cast_attr (const Attribute *a)
{
    return dynamic_cast_attr <U> ( const_cast <Attribute *> ( a ) );
}
template<class U>
static U &dynamic_cast_attr (Attribute &a)
{
    U *ret = dynamic_cast_attr <U> (&a);
    if ( ! ret )
        throw IEX_NAMESPACE::TypeExc ("Mismatched attribute type.");
    return *ret;
}
template<class U>
static const U &dynamic_cast_attr (const Attribute &a)
{
    const U *ret = dynamic_cast_attr <U> (&a);
    if ( ! ret )
        throw IEX_NAMESPACE::TypeExc ("Mismatched attribute type.");
    return *ret;
}

/// @}

//-------------------------------------------------
// Class template for attributes of a specific type
//-------------------------------------------------
    
template <class T>
class  TypedAttribute: public Attribute
{
  public:

    //------------------------------------------------------------
    // Constructors and destructor: default behavior. This assumes
    // that the type T is copyable/assignable/moveable.
    //------------------------------------------------------------

    TypedAttribute () = default;
    TypedAttribute (const T &value);
    TypedAttribute (const TypedAttribute<T> &other) = default;
    TypedAttribute (TypedAttribute<T> &&other) = default;

    virtual ~TypedAttribute () = default;

    TypedAttribute& operator = (const TypedAttribute<T>& other) = default;
    TypedAttribute& operator = (TypedAttribute<T>&& other) = default;
    
    //--------------------------------
    // Access to the attribute's value
    //--------------------------------

    T &					value ();
    const T &				value () const;


    //--------------------------------
    // Get this attribute's type name.
    //--------------------------------

    virtual const char *		typeName () const;
    

    //---------------------------------------------------------
    // Static version of typeName()
    // This function must be specialized for each value type T.
    //---------------------------------------------------------

    static const char *			staticTypeName ();
    

    //---------------------
    // Make a new attribute
    //---------------------

    static Attribute *			makeNewAttribute ();


    //------------------------------
    // Make a copy of this attribute
    //------------------------------

    virtual Attribute *			copy () const;


    //-----------------------------------------------------------------
    // Type-specific attribute I/O and copying.
    // Depending on type T, these functions may have to be specialized.
    //-----------------------------------------------------------------

    virtual void		writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os,
					      int version) const;

    virtual void		readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is,
					       int size,
					       int version);

    virtual void		copyValueFrom (const Attribute &other);


    //------------------------------------------------------------
    // Dynamic casts that throw exceptions instead of returning 0.
    //------------------------------------------------------------

    static TypedAttribute *		cast (Attribute *attribute);
    static const TypedAttribute *	cast (const Attribute *attribute);
    static TypedAttribute &		cast (Attribute &attribute);
    static const TypedAttribute &	cast (const Attribute &attribute);


    //---------------------------------------------------------------
    // Register this attribute type so that Attribute::newAttribute()
    // knows how to make objects of this type.
    //
    // Note that this function is not thread-safe because it modifies
    // a global variable in the IlmIlm library.  A thread in a multi-
    // threaded program may call registerAttributeType() only when no
    // other thread is accessing any functions or classes in the
    // OpenEXR library.
    //
    //---------------------------------------------------------------

    static void				registerAttributeType ();


    //-----------------------------------------------------
    // Un-register this attribute type (for debugging only)
    //-----------------------------------------------------

    static void				 unRegisterAttributeType ();


  private:

    T					_value;
};

//------------------------------------
// Implementation of TypedAttribute<T>
//------------------------------------

template <class T>
TypedAttribute<T>::TypedAttribute (const T & value):
    Attribute (),
    _value (value)
{
    // empty
}

template <class T>
inline T &
TypedAttribute<T>::value ()
{
    return _value;
}


template <class T>
inline const T &
TypedAttribute<T>::value () const
{
    return _value;
}


template <class T>
const char *	
TypedAttribute<T>::typeName () const
{
    return staticTypeName();
}


template <class T>
Attribute *
TypedAttribute<T>::makeNewAttribute ()
{
    return new TypedAttribute<T>();
}


template <class T>
Attribute *
TypedAttribute<T>::copy () const
{
    Attribute * attribute = new TypedAttribute<T>();
    attribute->copyValueFrom (*this);
    return attribute;
}


template <class T>
void		
TypedAttribute<T>::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os,
                                    int version) const
{
    OPENEXR_IMF_INTERNAL_NAMESPACE::Xdr::write <OPENEXR_IMF_INTERNAL_NAMESPACE::StreamIO> (os, _value);
}


template <class T>
void		
TypedAttribute<T>::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is,
                                     int size,
                                     int version)
{
    OPENEXR_IMF_INTERNAL_NAMESPACE::Xdr::read <OPENEXR_IMF_INTERNAL_NAMESPACE::StreamIO> (is, _value);
}


template <class T>
void		
TypedAttribute<T>::copyValueFrom (const Attribute &other)
{
    _value = cast(other)._value;
}


template <class T>
TypedAttribute<T> *
TypedAttribute<T>::cast (Attribute *attribute)
{
    TypedAttribute<T> *t =
	dynamic_cast_attr <TypedAttribute<T>> (attribute);

    if (t == 0)
	throw IEX_NAMESPACE::TypeExc ("Unexpected attribute type.");

    return t;
}


template <class T>
const TypedAttribute<T> *
TypedAttribute<T>::cast (const Attribute *attribute)
{
    const TypedAttribute<T> *t =
	dynamic_cast_attr <TypedAttribute<T>> (attribute);

    if (t == 0)
	throw IEX_NAMESPACE::TypeExc ("Unexpected attribute type.");

    return t;
}


template <class T>
inline TypedAttribute<T> &
TypedAttribute<T>::cast (Attribute &attribute)
{
    return *cast (&attribute);
}


template <class T>
inline const TypedAttribute<T> &
TypedAttribute<T>::cast (const Attribute &attribute)
{
    return *cast (&attribute);
}


template <class T>
inline void
TypedAttribute<T>::registerAttributeType ()
{
    Attribute::registerAttributeType (staticTypeName(), makeNewAttribute);
}


template <class T>
inline void
TypedAttribute<T>::unRegisterAttributeType ()
{
    Attribute::unRegisterAttributeType (staticTypeName());
}


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


#endif
