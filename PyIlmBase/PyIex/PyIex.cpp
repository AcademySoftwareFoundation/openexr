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

//-----------------------------------------------------------------------------
//
//        PyIex -- support for mapping C++ exceptions to Python exceptions
//
//-----------------------------------------------------------------------------


#include <PyIex.h>
#include <IexErrnoExc.h>

using namespace Iex;

namespace PyIex {

PY_DEFINE_EXC(BaseExc,iex,BaseExc)
PY_DEFINE_EXC(ArgExc,iex,ArgExc)
PY_DEFINE_EXC(LogicExc,iex,LogicExc)
PY_DEFINE_EXC(InputExc,iex,InputExc)
PY_DEFINE_EXC(IoExc,iex,IoExc)
PY_DEFINE_EXC(MathExc,iex,MathExc)
PY_DEFINE_EXC(NoImplExc,iex,NoImplExc)
PY_DEFINE_EXC(NullExc,iex,NullExc)
PY_DEFINE_EXC(TypeExc,iex,TypeExc)
PY_DEFINE_EXC(ErrnoExc,iex,ErrnoExc)
PY_DEFINE_EXC(EpermExc,iex,EpermExc)
PY_DEFINE_EXC(EnoentExc,iex,EnoentExc)
PY_DEFINE_EXC(EsrchExc,iex,EsrchExc)
PY_DEFINE_EXC(EintrExc,iex,EintrExc)
PY_DEFINE_EXC(EioExc,iex,EioExc)
PY_DEFINE_EXC(EnxioExc,iex,EnxioExc)
PY_DEFINE_EXC(E2bigExc,iex,E2bigExc)
PY_DEFINE_EXC(EnoexecExc,iex,EnoexecExc)
PY_DEFINE_EXC(EbadfExc,iex,EbadfExc)
PY_DEFINE_EXC(EchildExc,iex,EchildExc)
PY_DEFINE_EXC(EagainExc,iex,EagainExc)
PY_DEFINE_EXC(EnomemExc,iex,EnomemExc)
PY_DEFINE_EXC(EaccesExc,iex,EaccesExc)
PY_DEFINE_EXC(EfaultExc,iex,EfaultExc)
PY_DEFINE_EXC(EnotblkExc,iex,EnotblkExc)
PY_DEFINE_EXC(EbusyExc,iex,EbusyExc)
PY_DEFINE_EXC(EexistExc,iex,EexistExc)
PY_DEFINE_EXC(ExdevExc,iex,ExdevExc)
PY_DEFINE_EXC(EnodevExc,iex,EnodevExc)
PY_DEFINE_EXC(EnotdirExc,iex,EnotdirExc)
PY_DEFINE_EXC(EisdirExc,iex,EisdirExc)
PY_DEFINE_EXC(EinvalExc,iex,EinvalExc)
PY_DEFINE_EXC(EnfileExc,iex,EnfileExc)
PY_DEFINE_EXC(EmfileExc,iex,EmfileExc)
PY_DEFINE_EXC(EnottyExc,iex,EnottyExc)
PY_DEFINE_EXC(EtxtbsyExc,iex,EtxtbsyExc)
PY_DEFINE_EXC(EfbigExc,iex,EfbigExc)
PY_DEFINE_EXC(EnospcExc,iex,EnospcExc)
PY_DEFINE_EXC(EspipeExc,iex,EspipeExc)
PY_DEFINE_EXC(ErofsExc,iex,ErofsExc)
PY_DEFINE_EXC(EmlinkExc,iex,EmlinkExc)
PY_DEFINE_EXC(EpipeExc,iex,EpipeExc)
PY_DEFINE_EXC(EdomExc,iex,EdomExc)
PY_DEFINE_EXC(ErangeExc,iex,ErangeExc)
PY_DEFINE_EXC(EnomsgExc,iex,EnomsgExc)
PY_DEFINE_EXC(EidrmExc,iex,EidrmExc)
PY_DEFINE_EXC(EchrngExc,iex,EchrngExc)
PY_DEFINE_EXC(El2nsyncExc,iex,El2nsyncExc)
PY_DEFINE_EXC(El3hltExc,iex,El3hltExc)
PY_DEFINE_EXC(El3rstExc,iex,El3rstExc)
PY_DEFINE_EXC(ElnrngExc,iex,ElnrngExc)
PY_DEFINE_EXC(EunatchExc,iex,EunatchExc)
PY_DEFINE_EXC(EnocsiExc,iex,EnocsiExc)
PY_DEFINE_EXC(El2hltExc,iex,El2hltExc)
PY_DEFINE_EXC(EdeadlkExc,iex,EdeadlkExc)
PY_DEFINE_EXC(EnolckExc,iex,EnolckExc)
PY_DEFINE_EXC(EbadeExc,iex,EbadeExc)
PY_DEFINE_EXC(EbadrExc,iex,EbadrExc)
PY_DEFINE_EXC(ExfullExc,iex,ExfullExc)
PY_DEFINE_EXC(EnoanoExc,iex,EnoanoExc)
PY_DEFINE_EXC(EbadrqcExc,iex,EbadrqcExc)
PY_DEFINE_EXC(EbadsltExc,iex,EbadsltExc)
PY_DEFINE_EXC(EdeadlockExc,iex,EdeadlockExc)
PY_DEFINE_EXC(EbfontExc,iex,EbfontExc)
PY_DEFINE_EXC(EnostrExc,iex,EnostrExc)
PY_DEFINE_EXC(EnodataExc,iex,EnodataExc)
PY_DEFINE_EXC(EtimeExc,iex,EtimeExc)
PY_DEFINE_EXC(EnosrExc,iex,EnosrExc)
PY_DEFINE_EXC(EnonetExc,iex,EnonetExc)
PY_DEFINE_EXC(EnopkgExc,iex,EnopkgExc)
PY_DEFINE_EXC(EremoteExc,iex,EremoteExc)
PY_DEFINE_EXC(EnolinkExc,iex,EnolinkExc)
PY_DEFINE_EXC(EadvExc,iex,EadvExc)
PY_DEFINE_EXC(EsrmntExc,iex,EsrmntExc)
PY_DEFINE_EXC(EcommExc,iex,EcommExc)
PY_DEFINE_EXC(EprotoExc,iex,EprotoExc)
PY_DEFINE_EXC(EmultihopExc,iex,EmultihopExc)
PY_DEFINE_EXC(EbadmsgExc,iex,EbadmsgExc)
PY_DEFINE_EXC(EnametoolongExc,iex,EnametoolongExc)
PY_DEFINE_EXC(EoverflowExc,iex,EoverflowExc)
PY_DEFINE_EXC(EnotuniqExc,iex,EnotuniqExc)
PY_DEFINE_EXC(EbadfdExc,iex,EbadfdExc)
PY_DEFINE_EXC(EremchgExc,iex,EremchgExc)
PY_DEFINE_EXC(ElibaccExc,iex,ElibaccExc)
PY_DEFINE_EXC(ElibbadExc,iex,ElibbadExc)
PY_DEFINE_EXC(ElibscnExc,iex,ElibscnExc)
PY_DEFINE_EXC(ElibmaxExc,iex,ElibmaxExc)
PY_DEFINE_EXC(ElibexecExc,iex,ElibexecExc)
PY_DEFINE_EXC(EilseqExc,iex,EilseqExc)
PY_DEFINE_EXC(EnosysExc,iex,EnosysExc)
PY_DEFINE_EXC(EloopExc,iex,EloopExc)
PY_DEFINE_EXC(ErestartExc,iex,ErestartExc)
PY_DEFINE_EXC(EstrpipeExc,iex,EstrpipeExc)
PY_DEFINE_EXC(EnotemptyExc,iex,EnotemptyExc)
PY_DEFINE_EXC(EusersExc,iex,EusersExc)
PY_DEFINE_EXC(EnotsockExc,iex,EnotsockExc)
PY_DEFINE_EXC(EdestaddrreqExc,iex,EdestaddrreqExc)
PY_DEFINE_EXC(EmsgsizeExc,iex,EmsgsizeExc)
PY_DEFINE_EXC(EprototypeExc,iex,EprototypeExc)
PY_DEFINE_EXC(EnoprotooptExc,iex,EnoprotooptExc)
PY_DEFINE_EXC(EprotonosupportExc,iex,EprotonosupportExc)
PY_DEFINE_EXC(EsocktnosupportExc,iex,EsocktnosupportExc)
PY_DEFINE_EXC(EopnotsuppExc,iex,EopnotsuppExc)
PY_DEFINE_EXC(EpfnosupportExc,iex,EpfnosupportExc)
PY_DEFINE_EXC(EafnosupportExc,iex,EafnosupportExc)
PY_DEFINE_EXC(EaddrinuseExc,iex,EaddrinuseExc)
PY_DEFINE_EXC(EaddrnotavailExc,iex,EaddrnotavailExc)
PY_DEFINE_EXC(EnetdownExc,iex,EnetdownExc)
PY_DEFINE_EXC(EnetunreachExc,iex,EnetunreachExc)
PY_DEFINE_EXC(EnetresetExc,iex,EnetresetExc)
PY_DEFINE_EXC(EconnabortedExc,iex,EconnabortedExc)
PY_DEFINE_EXC(EconnresetExc,iex,EconnresetExc)
PY_DEFINE_EXC(EnobufsExc,iex,EnobufsExc)
PY_DEFINE_EXC(EisconnExc,iex,EisconnExc)
PY_DEFINE_EXC(EnotconnExc,iex,EnotconnExc)
PY_DEFINE_EXC(EshutdownExc,iex,EshutdownExc)
PY_DEFINE_EXC(EtoomanyrefsExc,iex,EtoomanyrefsExc)
PY_DEFINE_EXC(EtimedoutExc,iex,EtimedoutExc)
PY_DEFINE_EXC(EconnrefusedExc,iex,EconnrefusedExc)
PY_DEFINE_EXC(EhostdownExc,iex,EhostdownExc)
PY_DEFINE_EXC(EhostunreachExc,iex,EhostunreachExc)
PY_DEFINE_EXC(EalreadyExc,iex,EalreadyExc)
PY_DEFINE_EXC(EinprogressExc,iex,EinprogressExc)
PY_DEFINE_EXC(EstaleExc,iex,EstaleExc)
PY_DEFINE_EXC(EioresidExc,iex,EioresidExc)
PY_DEFINE_EXC(EucleanExc,iex,EucleanExc)
PY_DEFINE_EXC(EnotnamExc,iex,EnotnamExc)
PY_DEFINE_EXC(EnavailExc,iex,EnavailExc)
PY_DEFINE_EXC(EisnamExc,iex,EisnamExc)
PY_DEFINE_EXC(EremoteioExc,iex,EremoteioExc)
PY_DEFINE_EXC(EinitExc,iex,EinitExc)
PY_DEFINE_EXC(EremdevExc,iex,EremdevExc)
PY_DEFINE_EXC(EcanceledExc,iex,EcanceledExc)
PY_DEFINE_EXC(EnolimfileExc,iex,EnolimfileExc)
PY_DEFINE_EXC(EproclimExc,iex,EproclimExc)
PY_DEFINE_EXC(EdisjointExc,iex,EdisjointExc)
PY_DEFINE_EXC(EnologinExc,iex,EnologinExc)
PY_DEFINE_EXC(EloginlimExc,iex,EloginlimExc)
PY_DEFINE_EXC(EgrouploopExc,iex,EgrouploopExc)
PY_DEFINE_EXC(EnoattachExc,iex,EnoattachExc)
PY_DEFINE_EXC(EnotsupExc,iex,EnotsupExc)
PY_DEFINE_EXC(EnoattrExc,iex,EnoattrExc)
PY_DEFINE_EXC(EdircorruptedExc,iex,EdircorruptedExc)
PY_DEFINE_EXC(EdquotExc,iex,EdquotExc)
PY_DEFINE_EXC(EnfsremoteExc,iex,EnfsremoteExc)
PY_DEFINE_EXC(EcontrollerExc,iex,EcontrollerExc)
PY_DEFINE_EXC(EnotcontrollerExc,iex,EnotcontrollerExc)
PY_DEFINE_EXC(EenqueuedExc,iex,EenqueuedExc)
PY_DEFINE_EXC(EnotenqueuedExc,iex,EnotenqueuedExc)
PY_DEFINE_EXC(EjoinedExc,iex,EjoinedExc)
PY_DEFINE_EXC(EnotjoinedExc,iex,EnotjoinedExc)
PY_DEFINE_EXC(EnoprocExc,iex,EnoprocExc)
PY_DEFINE_EXC(EmustrunExc,iex,EmustrunExc)
PY_DEFINE_EXC(EnotstoppedExc,iex,EnotstoppedExc)
PY_DEFINE_EXC(EclockcpuExc,iex,EclockcpuExc)
PY_DEFINE_EXC(EinvalstateExc,iex,EinvalstateExc)
PY_DEFINE_EXC(EnoexistExc,iex,EnoexistExc)
PY_DEFINE_EXC(EendofminorExc,iex,EendofminorExc)
PY_DEFINE_EXC(EbufsizeExc,iex,EbufsizeExc)
PY_DEFINE_EXC(EemptyExc,iex,EemptyExc)
PY_DEFINE_EXC(EnointrgroupExc,iex,EnointrgroupExc)
PY_DEFINE_EXC(EinvalmodeExc,iex,EinvalmodeExc)
PY_DEFINE_EXC(EcantextentExc,iex,EcantextentExc)
PY_DEFINE_EXC(EinvaltimeExc,iex,EinvaltimeExc)
PY_DEFINE_EXC(EdestroyedExc,iex,EdestroyedExc)

} // namespace PyIex
