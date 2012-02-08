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

#include <Python.h>
#include <boost/python.hpp>
#include <boost/format.hpp>
#include <Iex.h>
#include <PyIex.h>
#include <IexErrnoExc.h>
#include <iostream>

using namespace boost::python;
using namespace Iex;

namespace PyIex {

namespace {

void
testCxxExceptions (int i)
{
    //
    // This function is only for testing.
    // It exercises the PY_TRY / PY_CATCH macros
    // and the C++ to Python exception translation.
    //


    switch (i)
    {
      case 1:
	throw int (1);

      case 2:
	throw std::invalid_argument ("2");

      case 3:
	throw Iex::BaseExc ("3");

      case 4:
	throw Iex::ArgExc ("4");

      default:
	;
    }
}

std::string
testBaseExcString(const BaseExc &exc)
{
    return exc.what();
}

std::string
testArgExcString(const ArgExc &exc)
{
    return exc.what();
}

BaseExc
testMakeBaseExc(const std::string &s)
{
    return BaseExc(s);
}

ArgExc
testMakeArgExc(const std::string &s)
{
    return ArgExc(s);
}

} // namespace

// not registered, but define RuntimeError so that BaseExc can have the appropriate
// python base type
struct RuntimeError {};
template<> PyObject *ExcTranslator<RuntimeError>::pytype = PyExc_RuntimeError;
template<> const char *ExcTranslator<RuntimeError>::name = "RuntimeError";
template<> const char *ExcTranslator<RuntimeError>::module = "__builtin__";

} // namespace PyIex

using namespace PyIex;

BOOST_PYTHON_MODULE(iex)
{
    using namespace Iex;

    def("testCxxExceptions", &testCxxExceptions);
    def("testBaseExcString", &testBaseExcString);
    def("testArgExcString", &testArgExcString);
    def("testMakeBaseExc", &testMakeBaseExc);
    def("testMakeArgExc", &testMakeArgExc);

    registerExc<BaseExc,RuntimeError>();
        registerExc<ArgExc,BaseExc>();
        registerExc<LogicExc,BaseExc>();
        registerExc<InputExc,BaseExc>();
        registerExc<IoExc,BaseExc>();
        registerExc<MathExc,BaseExc>();
        registerExc<NoImplExc,BaseExc>();
        registerExc<NullExc,BaseExc>();
        registerExc<TypeExc,BaseExc>();
        registerExc<ErrnoExc,BaseExc>();
            registerExc<EpermExc,ErrnoExc>();
            registerExc<EnoentExc,ErrnoExc>();
            registerExc<EsrchExc,ErrnoExc>();
            registerExc<EintrExc,ErrnoExc>();
            registerExc<EioExc,ErrnoExc>();
            registerExc<EnxioExc,ErrnoExc>();
            registerExc<E2bigExc,ErrnoExc>();
            registerExc<EnoexecExc,ErrnoExc>();
            registerExc<EbadfExc,ErrnoExc>();
            registerExc<EchildExc,ErrnoExc>();
            registerExc<EagainExc,ErrnoExc>();
            registerExc<EnomemExc,ErrnoExc>();
            registerExc<EaccesExc,ErrnoExc>();
            registerExc<EfaultExc,ErrnoExc>();
            registerExc<EnotblkExc,ErrnoExc>();
            registerExc<EbusyExc,ErrnoExc>();
            registerExc<EexistExc,ErrnoExc>();
            registerExc<ExdevExc,ErrnoExc>();
            registerExc<EnodevExc,ErrnoExc>();
            registerExc<EnotdirExc,ErrnoExc>();
            registerExc<EisdirExc,ErrnoExc>();
            registerExc<EinvalExc,ErrnoExc>();
            registerExc<EnfileExc,ErrnoExc>();
            registerExc<EmfileExc,ErrnoExc>();
            registerExc<EnottyExc,ErrnoExc>();
            registerExc<EtxtbsyExc,ErrnoExc>();
            registerExc<EfbigExc,ErrnoExc>();
            registerExc<EnospcExc,ErrnoExc>();
            registerExc<EspipeExc,ErrnoExc>();
            registerExc<ErofsExc,ErrnoExc>();
            registerExc<EmlinkExc,ErrnoExc>();
            registerExc<EpipeExc,ErrnoExc>();
            registerExc<EdomExc,ErrnoExc>();
            registerExc<ErangeExc,ErrnoExc>();
            registerExc<EnomsgExc,ErrnoExc>();
            registerExc<EidrmExc,ErrnoExc>();
            registerExc<EchrngExc,ErrnoExc>();
            registerExc<El2nsyncExc,ErrnoExc>();
            registerExc<El3hltExc,ErrnoExc>();
            registerExc<El3rstExc,ErrnoExc>();
            registerExc<ElnrngExc,ErrnoExc>();
            registerExc<EunatchExc,ErrnoExc>();
            registerExc<EnocsiExc,ErrnoExc>();
            registerExc<El2hltExc,ErrnoExc>();
            registerExc<EdeadlkExc,ErrnoExc>();
            registerExc<EnolckExc,ErrnoExc>();
            registerExc<EbadeExc,ErrnoExc>();
            registerExc<EbadrExc,ErrnoExc>();
            registerExc<ExfullExc,ErrnoExc>();
            registerExc<EnoanoExc,ErrnoExc>();
            registerExc<EbadrqcExc,ErrnoExc>();
            registerExc<EbadsltExc,ErrnoExc>();
            registerExc<EdeadlockExc,ErrnoExc>();
            registerExc<EbfontExc,ErrnoExc>();
            registerExc<EnostrExc,ErrnoExc>();
            registerExc<EnodataExc,ErrnoExc>();
            registerExc<EtimeExc,ErrnoExc>();
            registerExc<EnosrExc,ErrnoExc>();
            registerExc<EnonetExc,ErrnoExc>();
            registerExc<EnopkgExc,ErrnoExc>();
            registerExc<EremoteExc,ErrnoExc>();
            registerExc<EnolinkExc,ErrnoExc>();
            registerExc<EadvExc,ErrnoExc>();
            registerExc<EsrmntExc,ErrnoExc>();
            registerExc<EcommExc,ErrnoExc>();
            registerExc<EprotoExc,ErrnoExc>();
            registerExc<EmultihopExc,ErrnoExc>();
            registerExc<EbadmsgExc,ErrnoExc>();
            registerExc<EnametoolongExc,ErrnoExc>();
            registerExc<EoverflowExc,ErrnoExc>();
            registerExc<EnotuniqExc,ErrnoExc>();
            registerExc<EbadfdExc,ErrnoExc>();
            registerExc<EremchgExc,ErrnoExc>();
            registerExc<ElibaccExc,ErrnoExc>();
            registerExc<ElibbadExc,ErrnoExc>();
            registerExc<ElibscnExc,ErrnoExc>();
            registerExc<ElibmaxExc,ErrnoExc>();
            registerExc<ElibexecExc,ErrnoExc>();
            registerExc<EilseqExc,ErrnoExc>();
            registerExc<EnosysExc,ErrnoExc>();
            registerExc<EloopExc,ErrnoExc>();
            registerExc<ErestartExc,ErrnoExc>();
            registerExc<EstrpipeExc,ErrnoExc>();
            registerExc<EnotemptyExc,ErrnoExc>();
            registerExc<EusersExc,ErrnoExc>();
            registerExc<EnotsockExc,ErrnoExc>();
            registerExc<EdestaddrreqExc,ErrnoExc>();
            registerExc<EmsgsizeExc,ErrnoExc>();
            registerExc<EprototypeExc,ErrnoExc>();
            registerExc<EnoprotooptExc,ErrnoExc>();
            registerExc<EprotonosupportExc,ErrnoExc>();
            registerExc<EsocktnosupportExc,ErrnoExc>();
            registerExc<EopnotsuppExc,ErrnoExc>();
            registerExc<EpfnosupportExc,ErrnoExc>();
            registerExc<EafnosupportExc,ErrnoExc>();
            registerExc<EaddrinuseExc,ErrnoExc>();
            registerExc<EaddrnotavailExc,ErrnoExc>();
            registerExc<EnetdownExc,ErrnoExc>();
            registerExc<EnetunreachExc,ErrnoExc>();
            registerExc<EnetresetExc,ErrnoExc>();
            registerExc<EconnabortedExc,ErrnoExc>();
            registerExc<EconnresetExc,ErrnoExc>();
            registerExc<EnobufsExc,ErrnoExc>();
            registerExc<EisconnExc,ErrnoExc>();
            registerExc<EnotconnExc,ErrnoExc>();
            registerExc<EshutdownExc,ErrnoExc>();
            registerExc<EtoomanyrefsExc,ErrnoExc>();
            registerExc<EtimedoutExc,ErrnoExc>();
            registerExc<EconnrefusedExc,ErrnoExc>();
            registerExc<EhostdownExc,ErrnoExc>();
            registerExc<EhostunreachExc,ErrnoExc>();
            registerExc<EalreadyExc,ErrnoExc>();
            registerExc<EinprogressExc,ErrnoExc>();
            registerExc<EstaleExc,ErrnoExc>();
            registerExc<EioresidExc,ErrnoExc>();
            registerExc<EucleanExc,ErrnoExc>();
            registerExc<EnotnamExc,ErrnoExc>();
            registerExc<EnavailExc,ErrnoExc>();
            registerExc<EisnamExc,ErrnoExc>();
            registerExc<EremoteioExc,ErrnoExc>();
            registerExc<EinitExc,ErrnoExc>();
            registerExc<EremdevExc,ErrnoExc>();
            registerExc<EcanceledExc,ErrnoExc>();
            registerExc<EnolimfileExc,ErrnoExc>();
            registerExc<EproclimExc,ErrnoExc>();
            registerExc<EdisjointExc,ErrnoExc>();
            registerExc<EnologinExc,ErrnoExc>();
            registerExc<EloginlimExc,ErrnoExc>();
            registerExc<EgrouploopExc,ErrnoExc>();
            registerExc<EnoattachExc,ErrnoExc>();
            registerExc<EnotsupExc,ErrnoExc>();
            registerExc<EnoattrExc,ErrnoExc>();
            registerExc<EdircorruptedExc,ErrnoExc>();
            registerExc<EdquotExc,ErrnoExc>();
            registerExc<EnfsremoteExc,ErrnoExc>();
            registerExc<EcontrollerExc,ErrnoExc>();
            registerExc<EnotcontrollerExc,ErrnoExc>();
            registerExc<EenqueuedExc,ErrnoExc>();
            registerExc<EnotenqueuedExc,ErrnoExc>();
            registerExc<EjoinedExc,ErrnoExc>();
            registerExc<EnotjoinedExc,ErrnoExc>();
            registerExc<EnoprocExc,ErrnoExc>();
            registerExc<EmustrunExc,ErrnoExc>();
            registerExc<EnotstoppedExc,ErrnoExc>();
            registerExc<EclockcpuExc,ErrnoExc>();
            registerExc<EinvalstateExc,ErrnoExc>();
            registerExc<EnoexistExc,ErrnoExc>();
            registerExc<EendofminorExc,ErrnoExc>();
            registerExc<EbufsizeExc,ErrnoExc>();
            registerExc<EemptyExc,ErrnoExc>();
            registerExc<EnointrgroupExc,ErrnoExc>();
            registerExc<EinvalmodeExc,ErrnoExc>();
            registerExc<EcantextentExc,ErrnoExc>();
            registerExc<EinvaltimeExc,ErrnoExc>();
            registerExc<EdestroyedExc,ErrnoExc>();
}
