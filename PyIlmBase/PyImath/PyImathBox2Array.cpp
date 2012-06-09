///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998-2011, Industrial Light & Magic, a division of Lucas
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


#include <PyImathBoxArrayImpl.h>
#include <PyImathExport.h>

namespace PyImath {
using namespace boost::python;

template PYIMATH_EXPORT class_<FixedArray<Imath::Box2s> > register_BoxArray<Imath::V2s>();
template PYIMATH_EXPORT class_<FixedArray<Imath::Box2i> > register_BoxArray<Imath::V2i>();
template PYIMATH_EXPORT class_<FixedArray<Imath::Box2f> > register_BoxArray<Imath::V2f>();
template PYIMATH_EXPORT class_<FixedArray<Imath::Box2d> > register_BoxArray<Imath::V2d>();

template<> PYIMATH_EXPORT Imath::Box2s PyImath::FixedArrayDefaultValue<Imath::Box2s>::value() { return Imath::Box2s(); }
template<> PYIMATH_EXPORT Imath::Box2i PyImath::FixedArrayDefaultValue<Imath::Box2i>::value() { return Imath::Box2i(); }
template<> PYIMATH_EXPORT Imath::Box2f PyImath::FixedArrayDefaultValue<Imath::Box2f>::value() { return Imath::Box2f(); }
template<> PYIMATH_EXPORT Imath::Box2d PyImath::FixedArrayDefaultValue<Imath::Box2d>::value() { return Imath::Box2d(); }
}
