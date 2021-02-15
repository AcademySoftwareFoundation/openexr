//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IEXEXPORT_H
#define IEXEXPORT_H

/// \defgroup ExportMacros Macros to manage symbol visibility
///
/// In order to produce tidy symbol tables in shared objects, one must
/// manage symbol visibility. This is required under Windows for DLLs,
/// and has been well documented in terms of export / import
/// swapping. However, under Unixen or other similar platforms, there
/// is a different mechanism of specifying the visibility. So to
/// achieve nearly the same results, without requiring every single
/// function be tagged, one can tell the compiler to mark all symbols
/// as hidden by default, then only export the specific symbols in
/// question.
///
/// However, this is not so easy for C++. There are what are called
/// 'vague' linkage objects, which are often the typeinfo / vtable
/// type objects, although templates and a few other items fall into
/// this category. In order to enable dynamic_cast and similar
/// behavior, we must export the typeinfo and such. This is done at
/// the class level, but if we were to do this under Windows, then the
/// class member data types also end up being exported, which leaks
/// STL details. So, to differentiate this, we use
/// IMF_EXPORT_VAGUELINKAGE at the class level, and continue to use
/// IMF_EXPORT on the public function symbols. Unfortunately, by
/// putting the export at the class level, that means that private
/// functions are then exported as well. Hence, we must force any of
/// these back to local using IMF_EXPORT_LOCAL. To avoid pollution of
/// the code, we should only apply this to classes which are used for
/// dynamic_cast, or otherwise require it.
///
/// There may be more needed than have been tagged so far, if you
/// start receiving link errors about typeinfo, please look if it is
/// the symbol exports, and whether additional tagging is
/// needed. Having a goal to hide symbols should increase symbol
/// loading / resolution performance in aggregate, so is a desired end
/// goal.
///
/// @{
#if defined(OPENEXR_DLL)
#    if defined(IEX_EXPORTS)
#        define IEX_EXPORT __declspec(dllexport)
#    else
#        define IEX_EXPORT __declspec(dllimport)
#    endif
#    define IEX_EXPORT_VAGUELINKAGE
#    define IEX_EXPORT_LOCAL
#else
#    ifndef _MSC_VER
#        define IEX_EXPORT __attribute__ ((visibility ("default")))
#        define IEX_EXPORT_VAGUELINKAGE __attribute__ ((visibility ("default")))
#        define IEX_EXPORT_LOCAL __attribute__ ((visibility ("hidden")))
#    else
#        define IEX_EXPORT
#        define IEX_EXPORT_VAGUELINKAGE
#        define IEX_EXPORT_LOCAL
#    endif
#endif

#ifndef IEX_EXPORT_TYPE
#    define IEX_EXPORT_TYPE
#endif
#ifndef IEX_EXPORT_ENUM
#    define IEX_EXPORT_ENUM
#endif

/// @}

#endif // #ifndef IEXEXPORT_H

