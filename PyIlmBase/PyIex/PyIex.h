///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2001-2011, Industrial Light & Magic, a division of Lucas
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

#ifndef INCLUDED_PY_IEX_H
#define INCLUDED_PY_IEX_H

//-----------------------------------------------------------------------------
//
//	PyIex -- support for mapping C++ exceptions to Python exceptions
//
//-----------------------------------------------------------------------------

#include <Python.h>
#include <boost/python.hpp>
#include <IexMathFloatExc.h>
#include <sstream>
#include <boost/python/errors.hpp>
#include <boost/format.hpp>

#if defined(OPENEXR_DLL) && !defined(ZENO_STATIC)
    #ifdef PYIEX_EXPORTS
        #define PYIEX_EXPORT __declspec(dllexport)
    #else
        #define PYIEX_EXPORT __declspec(dllimport)
    #endif
#else
    #define PYIEX_EXPORT
#endif


namespace PyIex {

//
// Macros to catch C++ exceptions and translate them into Python exceptions
// for use in python C api code:
//
//	PY_TRY
//	PY_CATCH
//
// Usage:
//
//	Insert PY_TRY and PY_CATCH at the beginning and end of every
//	wrapper function to make sure that all possible exceptions
//	are caught and translated to corresponding Python exceptions.
//	Example:
//	
//	PyObject *
//	setSpeed (PyCar *self, PyObject *args)
//	{
//	    PY_TRY
//
//	    float length;
//	    PY_ARG_PARSE ((args, "f", &length));
//
//	    self->data->setSpeed (length);	// may throw
//
//	    PY_RETURN_NONE;
//	    PY_CATCH
//	}
//

#define PY_TRY						\
    try							\
    {							\
	Iex::MathExcOn mathexcon (Iex::IEEE_OVERFLOW |	\
			          Iex::IEEE_DIVZERO |	\
				  Iex::IEEE_INVALID);


#define PY_CATCH					\
    }							\
    catch (boost::python::error_already_set)		\
    {							\
	return 0;					\
    }							\
    catch (...)						\
    {							\
	boost::python::handle_exception();		\
	return 0;					\
    }


#define PY_CATCH_WITH_COMMENT(text)			\
    }							\
    catch (boost::python::error_already_set)		\
    {							\
        /* Can't use text here without messing with */	\
        /* the existing python exception state, so  */	\
        /* ignore                                   */	\
	return 0;					\
    }							\
    catch (...)						\
    {							\
	boost::python::handle_exception();		\
	return 0;					\
    }


// In most case, PY_CATCH should be used.  But in a few cases, the Python
// interpreter treats a return code of 0 as success rather than failure
// (e.g., the tp_print routine in a PyTypeObject struct). 

#define PY_CATCH_RETURN_CODE(CODE)			\
    }							\
    catch (...)						\
    {							\
	boost::python::handle_exception();		\
	return (CODE);					\
    }

//
// The following should be used for registering exceptions
// derived from Iex::BaseExc.  To register a new exception,
// call PY_DEFINE_EXC within the PyIex namespace passing the
// c++ type, python module name and type name for python:
//
// namespace PyIex {
// PY_DEFINE_EXC(MyExcType,iex,MyExcType)
// }
//
//
// Then in the module definition:
//
//     registerExc<MyExcType,MyExcBaseType>();
//

template <class Exc>
struct ExcTranslator
{
    static PyObject *pytype;
    static const char *module;
    static const char *name;

    // to python
    static PyObject *convert(const Exc &exc)
    {
        using namespace boost::python;
        return incref(object(handle<>(borrowed(pytype)))(exc.what()).ptr());
    }

    static PyTypeObject *get_pytype()
    {
        return (PyTypeObject *)pytype;
    }

    // from python
    static void *convertible(PyObject *exc)
    {
        if (!PyType_IsSubtype(Py_TYPE(exc),(PyTypeObject *)pytype)) return 0;
        return exc;
    }

    static void construct(PyObject* raw_exc, boost::python::converter::rvalue_from_python_stage1_data* data)
    {
        using namespace boost::python;
        object exc(handle<>(borrowed(raw_exc)));
        std::string s = extract<std::string>(exc.attr("__str__")());
        void *storage = ((converter::rvalue_from_python_storage<Exc>*)data)->storage.bytes;
        new (storage) Exc(s);
        data->convertible = storage;
    }

    // translate exception
    static void translate(const Exc &exc)
    {
        PyErr_SetObject(pytype,convert(exc));
    }
};


template <class Exc, class Base>
void
registerExc()
{
    using namespace boost::python;

    std::string classname = ExcTranslator<Exc>::name;
    std::string module = ExcTranslator<Exc>::module;
    std::string basename = ExcTranslator<Base>::name;
    std::string basemodule = ExcTranslator<Base>::module;

    dict tmpDict;
    tmpDict["__builtins__"] = handle<>(borrowed(PyEval_GetBuiltins()));

    std::string definition;
    if (basemodule != module)
    {
        definition += (boost::format("import %s\n") % basemodule).str();
        basename = (boost::format("%s.%s") % basemodule % basename).str();
    }
    else
    {
        // bind in the base class type into the tmp dict
        tmpDict[basename] = object(handle<>(borrowed(ExcTranslator<Base>::pytype)));
    }

    definition += (boost::format("class %s (%s):\n"
                                 "  def __init__ (self, v=''):\n"
                                 "    super(%s,self).__init__(v)\n"
                                 "  def __repr__ (self):\n"
                                 "    return \"%s.%s('%%s')\"%%(self.args[0])\n")
                   % classname % basename % classname % module % classname).str();

    handle<> tmp(PyRun_String(definition.c_str(),Py_file_input,tmpDict.ptr(),tmpDict.ptr()));
    object exc_class = tmpDict[classname];
    scope().attr(classname.c_str()) = exc_class;
    ExcTranslator<Exc>::pytype = exc_class.ptr();

    // to python
    to_python_converter<Exc,ExcTranslator<Exc>,true>();

    // from python
    converter::registry::push_back(&ExcTranslator<Exc>::convertible,
                                   &ExcTranslator<Exc>::construct,type_id<Exc>());

    // exception translation
    register_exception_translator<Exc>(&ExcTranslator<Exc>::translate);
}

#define PY_DEFINE_EXC(ExcType,ModuleName,ExcName)                     \
template <> PyObject *ExcTranslator<ExcType>::pytype = 0;             \
template <> const char *ExcTranslator<ExcType>::module = #ModuleName; \
template <> const char *ExcTranslator<ExcType>::name = #ExcName;

} // namespace PyIex

#endif
