#include <Python.h>
#include <boost/python.hpp>
#include <PyImath.h>
#include <PyImathVec.h>
#include <iostream>
#include <boost/format.hpp>
#include <numpy/arrayobject.h>

using namespace boost::python;
using namespace PyImath;

static
object 
arrayToNumpy_float(FloatArray &fa)
{
    if (fa.stride() != 1) {
        throw Iex::LogicExc("Unable to make numpy wrapping of strided arrays");
    }

    npy_intp length = fa.len();
    float *data = &fa[0];
    PyObject *a = PyArray_SimpleNewFromData(1,&length,NPY_FLOAT,data);

    if (!a) {
        throw_error_already_set();
    }

    object retval = object(handle<>(a));
    return retval;
}

static
object 
arrayToNumpy_V3f(V3fArray &va)
{
    if (va.stride() != 1) {
        throw Iex::LogicExc("Unable to make numpy wrapping of strided arrays");
    }

    npy_intp length[2];
    length[0] = va.len();
    length[1] = 3;
    float *data = &va[0].x;
    PyObject *a = PyArray_SimpleNewFromData(2,length,NPY_FLOAT,data);

    if (!a) {
        throw_error_already_set();
    }

    object retval = object(handle<>(a));
    return retval;
}

static
object 
arrayToNumpy_int(IntArray &va)
{
    if (va.stride() != 1) {
        throw Iex::LogicExc("Unable to make numpy wrapping of strided arrays");
    }

    npy_intp length = va.len();
    int *data = &va[0];
    PyObject *a = PyArray_SimpleNewFromData(1,&length,NPY_INT,data);

    if (!a) {
        throw_error_already_set();
    }

    object retval = object(handle<>(a));
    return retval;
}

BOOST_PYTHON_MODULE(imathnumpy)
{
    handle<> imath(PyImport_ImportModule("imath"));
    if (PyErr_Occurred()) throw_error_already_set();
    scope().attr("imath") = imath;

    handle<> numpy(PyImport_ImportModule("numpy"));
    if (PyErr_Occurred()) throw_error_already_set();
    scope().attr("numpy") = numpy;

    import_array();

    scope().attr("__doc__") = "Array wrapping module to overlay imath array data with numpy arrays";

    def("arrayToNumpy",&arrayToNumpy_float,
        "arrayToNumpy(array) - wrap the given FloatArray as a numpy array",
        (arg("array")));
    def("arrayToNumpy",&arrayToNumpy_V3f,
        "arrayToNumpy(array) - wrap the given V3fArray as a numpy array",
        (arg("array")));
    def("arrayToNumpy",&arrayToNumpy_int,
        "arrayToNumpy(array) - wrap the given IntArray as a numpy array",
        (arg("array")));
}
