///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
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

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <testBaseExc.h>
#include <Iex.h>
#include <IexErrnoExc.h>
#include <iostream>
#include <stdexcept>
#include <assert.h>

namespace {

void
throwArgExc ()
{
    throw IEX_INTERNAL_NAMESPACE::ArgExc ("ArgExc");
}

void
throwLogicError ()
{
    throw std::logic_error("logic_error");
}

void
throwInt ()
{
    throw 3;
}

void
throwNested()
{
    try
    {
	throwArgExc();
    }
    catch (const IEX_INTERNAL_NAMESPACE::ArgExc &)
    {
        bool caught = false;
	try
	{
	    throwInt();
	}
	catch (...)
	{
            caught = true;
	}

        assert (caught);

	throw;
    }
}

void
test1 ()
{
    std::cout << "1" << std::endl;

    try
    {
	throwArgExc();
    }
    catch (const IEX_INTERNAL_NAMESPACE::ArgExc &)
    {
	return;
    }
    catch (std::exception &)
    {
	assert (false);
    }
    catch (...)
    {
	assert (false);
    }

    assert (false);
}

void
test2 ()
{
    std::cout << "2" << std::endl;

    try
    {
	throwLogicError();
    }
    catch (const IEX_INTERNAL_NAMESPACE::ArgExc &)
    {
	assert (false);
    }
    catch (std::exception &)
    {
	return;
    }
    catch (...)
    {
	assert (false);
    }

    assert (false);
}

void
test3 ()
{
    std::cout << "3" << std::endl;

    try
    {
	throwArgExc();
    }
    catch (std::exception &)
    {
	return;
    }
    catch (...)
    {
	assert (false);
    }

    assert (false);
}

void
test4 ()
{
    std::cout << "4" << std::endl;

    try
    {
	throwInt();
    }
    catch (const IEX_INTERNAL_NAMESPACE::ArgExc &)
    {
	assert (false);
    }
    catch (std::exception &)
    {
	assert (false);
    }
    catch (...)
    {
	return;
    }

    assert (false);
}

void
test5()
{
    std::cout << "5" << std::endl;

    try
    {
	throwNested();
    }
    catch (const IEX_INTERNAL_NAMESPACE::ArgExc &e)
    {
	assert (std::string(e.what()) == "ArgExc");
    }
}

std::string
getStackTrace()
{
    return "???";
}
    

template <class T>
void
test6()
{
    std::cout << "6" << std::endl;

    IEX_INTERNAL_NAMESPACE::setStackTracer (getStackTrace);
    assert (IEX_INTERNAL_NAMESPACE::stackTracer() == getStackTrace);
    
    //
    // Test the constructors that take char* and stringstream,
    // and the += and assign functions.
    //
    
    T e1 ("arg");

    e1 += "X";
    std::stringstream s;
    s << "Y";
    e1 += s;
    
    T e2 (e1);
    
    assert (e1.message() == "argXY");
    assert (e1.stackTrace() == getStackTrace());

    assert (e2.message() == e1.message());
    assert (e2.stackTrace() == e1.stackTrace());

    e2.assign ("Z");
    assert (e2.message() == "Z");
    e2.assign (s);
    assert (e2.message() == "Y");

    T e3 (s);
    assert (e3.message() == s.str());

    //
    // Confirm the throw/catch
    //
    
    bool caught = false;
    
    try
    {
        throw e1;
    }
    catch (T& e)
    {
        caught = true;
        assert (e.message() == e1.message());
    }
    catch (...)
    {
        assert (false);
    }

    assert (caught);
}
    

} // namespace


void
testBaseExc()
{
    std::cout << "See if throw and catch work:" << std::endl;

    test1();
    test2();
    test3();
    test4();
    test5();
    
    test6<IEX_INTERNAL_NAMESPACE::ArgExc>();
    test6<IEX_INTERNAL_NAMESPACE::LogicExc>();
    test6<IEX_INTERNAL_NAMESPACE::InputExc>();
    test6<IEX_INTERNAL_NAMESPACE::IoExc>();
    test6<IEX_INTERNAL_NAMESPACE::MathExc>();
    test6<IEX_INTERNAL_NAMESPACE::ErrnoExc>();
    test6<IEX_INTERNAL_NAMESPACE::NoImplExc>();
    test6<IEX_INTERNAL_NAMESPACE::NullExc>();
    test6<IEX_INTERNAL_NAMESPACE::TypeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EpermExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnoentExc>();
    test6<IEX_INTERNAL_NAMESPACE::EsrchExc>();
    test6<IEX_INTERNAL_NAMESPACE::EintrExc>();
    test6<IEX_INTERNAL_NAMESPACE::EioExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnxioExc>();
    test6<IEX_INTERNAL_NAMESPACE::E2bigExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnoexecExc>();
    test6<IEX_INTERNAL_NAMESPACE::EbadfExc>();
    test6<IEX_INTERNAL_NAMESPACE::EchildExc>();
    test6<IEX_INTERNAL_NAMESPACE::EagainExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnomemExc>();
    test6<IEX_INTERNAL_NAMESPACE::EaccesExc>();
    test6<IEX_INTERNAL_NAMESPACE::EfaultExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotblkExc>();
    test6<IEX_INTERNAL_NAMESPACE::EbusyExc>();
    test6<IEX_INTERNAL_NAMESPACE::EexistExc>();
    test6<IEX_INTERNAL_NAMESPACE::ExdevExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnodevExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotdirExc>();
    test6<IEX_INTERNAL_NAMESPACE::EisdirExc>();
    test6<IEX_INTERNAL_NAMESPACE::EinvalExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnfileExc>();
    test6<IEX_INTERNAL_NAMESPACE::EmfileExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnottyExc>();
    test6<IEX_INTERNAL_NAMESPACE::EtxtbsyExc>();
    test6<IEX_INTERNAL_NAMESPACE::EfbigExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnospcExc>();
    test6<IEX_INTERNAL_NAMESPACE::EspipeExc>();
    test6<IEX_INTERNAL_NAMESPACE::ErofsExc>();
    test6<IEX_INTERNAL_NAMESPACE::EmlinkExc>();
    test6<IEX_INTERNAL_NAMESPACE::EpipeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EdomExc>();
    test6<IEX_INTERNAL_NAMESPACE::ErangeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnomsgExc>();
    test6<IEX_INTERNAL_NAMESPACE::EidrmExc>();
    test6<IEX_INTERNAL_NAMESPACE::EchrngExc>();
    test6<IEX_INTERNAL_NAMESPACE::El2nsyncExc>();
    test6<IEX_INTERNAL_NAMESPACE::El3hltExc>();
    test6<IEX_INTERNAL_NAMESPACE::El3rstExc>();
    test6<IEX_INTERNAL_NAMESPACE::ElnrngExc>();
    test6<IEX_INTERNAL_NAMESPACE::EunatchExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnocsiExc>();
    test6<IEX_INTERNAL_NAMESPACE::El2hltExc>();
    test6<IEX_INTERNAL_NAMESPACE::EdeadlkExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnolckExc>();
    test6<IEX_INTERNAL_NAMESPACE::EbadeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EbadrExc>();
    test6<IEX_INTERNAL_NAMESPACE::ExfullExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnoanoExc>();
    test6<IEX_INTERNAL_NAMESPACE::EbadrqcExc>();
    test6<IEX_INTERNAL_NAMESPACE::EbadsltExc>();
    test6<IEX_INTERNAL_NAMESPACE::EdeadlockExc>();
    test6<IEX_INTERNAL_NAMESPACE::EbfontExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnostrExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnodataExc>();
    test6<IEX_INTERNAL_NAMESPACE::EtimeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnosrExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnonetExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnopkgExc>();
    test6<IEX_INTERNAL_NAMESPACE::EremoteExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnolinkExc>();
    test6<IEX_INTERNAL_NAMESPACE::EadvExc>();
    test6<IEX_INTERNAL_NAMESPACE::EsrmntExc>();
    test6<IEX_INTERNAL_NAMESPACE::EcommExc>();
    test6<IEX_INTERNAL_NAMESPACE::EprotoExc>();
    test6<IEX_INTERNAL_NAMESPACE::EmultihopExc>();
    test6<IEX_INTERNAL_NAMESPACE::EbadmsgExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnametoolongExc>();
    test6<IEX_INTERNAL_NAMESPACE::EoverflowExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotuniqExc>();
    test6<IEX_INTERNAL_NAMESPACE::EbadfdExc>();
    test6<IEX_INTERNAL_NAMESPACE::EremchgExc>();
    test6<IEX_INTERNAL_NAMESPACE::ElibaccExc>();
    test6<IEX_INTERNAL_NAMESPACE::ElibbadExc>();
    test6<IEX_INTERNAL_NAMESPACE::ElibscnExc>();
    test6<IEX_INTERNAL_NAMESPACE::ElibmaxExc>();
    test6<IEX_INTERNAL_NAMESPACE::ElibexecExc>();
    test6<IEX_INTERNAL_NAMESPACE::EilseqExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnosysExc>();
    test6<IEX_INTERNAL_NAMESPACE::EloopExc>();
    test6<IEX_INTERNAL_NAMESPACE::ErestartExc>();
    test6<IEX_INTERNAL_NAMESPACE::EstrpipeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotemptyExc>();
    test6<IEX_INTERNAL_NAMESPACE::EusersExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotsockExc>();
    test6<IEX_INTERNAL_NAMESPACE::EdestaddrreqExc>();
    test6<IEX_INTERNAL_NAMESPACE::EmsgsizeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EprototypeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnoprotooptExc>();
    test6<IEX_INTERNAL_NAMESPACE::EprotonosupportExc>();
    test6<IEX_INTERNAL_NAMESPACE::EsocktnosupportExc>();
    test6<IEX_INTERNAL_NAMESPACE::EopnotsuppExc>();
    test6<IEX_INTERNAL_NAMESPACE::EpfnosupportExc>();
    test6<IEX_INTERNAL_NAMESPACE::EafnosupportExc>();
    test6<IEX_INTERNAL_NAMESPACE::EaddrinuseExc>();
    test6<IEX_INTERNAL_NAMESPACE::EaddrnotavailExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnetdownExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnetunreachExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnetresetExc>();
    test6<IEX_INTERNAL_NAMESPACE::EconnabortedExc>();
    test6<IEX_INTERNAL_NAMESPACE::EconnresetExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnobufsExc>();
    test6<IEX_INTERNAL_NAMESPACE::EisconnExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotconnExc>();
    test6<IEX_INTERNAL_NAMESPACE::EshutdownExc>();
    test6<IEX_INTERNAL_NAMESPACE::EtoomanyrefsExc>();
    test6<IEX_INTERNAL_NAMESPACE::EtimedoutExc>();
    test6<IEX_INTERNAL_NAMESPACE::EconnrefusedExc>();
    test6<IEX_INTERNAL_NAMESPACE::EhostdownExc>();
    test6<IEX_INTERNAL_NAMESPACE::EhostunreachExc>();
    test6<IEX_INTERNAL_NAMESPACE::EalreadyExc>();
    test6<IEX_INTERNAL_NAMESPACE::EinprogressExc>();
    test6<IEX_INTERNAL_NAMESPACE::EstaleExc>();
    test6<IEX_INTERNAL_NAMESPACE::EioresidExc>();
    test6<IEX_INTERNAL_NAMESPACE::EucleanExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotnamExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnavailExc>();
    test6<IEX_INTERNAL_NAMESPACE::EisnamExc>();
    test6<IEX_INTERNAL_NAMESPACE::EremoteioExc>();
    test6<IEX_INTERNAL_NAMESPACE::EinitExc>();
    test6<IEX_INTERNAL_NAMESPACE::EremdevExc>();
    test6<IEX_INTERNAL_NAMESPACE::EcanceledExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnolimfileExc>();
    test6<IEX_INTERNAL_NAMESPACE::EproclimExc>();
    test6<IEX_INTERNAL_NAMESPACE::EdisjointExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnologinExc>();
    test6<IEX_INTERNAL_NAMESPACE::EloginlimExc>();
    test6<IEX_INTERNAL_NAMESPACE::EgrouploopExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnoattachExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotsupExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnoattrExc>();
    test6<IEX_INTERNAL_NAMESPACE::EdircorruptedExc>();
    test6<IEX_INTERNAL_NAMESPACE::EdquotExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnfsremoteExc>();
    test6<IEX_INTERNAL_NAMESPACE::EcontrollerExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotcontrollerExc>();
    test6<IEX_INTERNAL_NAMESPACE::EenqueuedExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotenqueuedExc>();
    test6<IEX_INTERNAL_NAMESPACE::EjoinedExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotjoinedExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnoprocExc>();
    test6<IEX_INTERNAL_NAMESPACE::EmustrunExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnotstoppedExc>();
    test6<IEX_INTERNAL_NAMESPACE::EclockcpuExc>();
    test6<IEX_INTERNAL_NAMESPACE::EinvalstateExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnoexistExc>();
    test6<IEX_INTERNAL_NAMESPACE::EendofminorExc>();
    test6<IEX_INTERNAL_NAMESPACE::EbufsizeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EemptyExc>();
    test6<IEX_INTERNAL_NAMESPACE::EnointrgroupExc>();
    test6<IEX_INTERNAL_NAMESPACE::EinvalmodeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EcantextentExc>();
    test6<IEX_INTERNAL_NAMESPACE::EinvaltimeExc>();
    test6<IEX_INTERNAL_NAMESPACE::EdestroyedExc>();
    test6<IEX_INTERNAL_NAMESPACE::OverflowExc>();
    test6<IEX_INTERNAL_NAMESPACE::UnderflowExc>();
    test6<IEX_INTERNAL_NAMESPACE::DivzeroExc>();
    test6<IEX_INTERNAL_NAMESPACE::InexactExc>();
    test6<IEX_INTERNAL_NAMESPACE::InvalidFpOpExc>();
    
    std::cout << "ok\n" << std::endl;
}
