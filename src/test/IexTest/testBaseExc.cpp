//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <Iex.h>
#include <IexErrnoExc.h>
#include <assert.h>
#include <iostream>
#include <stdexcept>
#include <typeinfo>
#include <errno.h>
#include <testBaseExc.h>

namespace
{

void
throwArgExc ()
{
    throw IEX_INTERNAL_NAMESPACE::ArgExc ("ArgExc");
}

void
throwLogicError ()
{
    throw std::logic_error ("logic_error");
}

void
throwInt ()
{
    throw 3;
}

void
throwNested ()
{
    try
    {
        throwArgExc ();
    }
    catch (const IEX_INTERNAL_NAMESPACE::ArgExc&)
    {
        bool caught = false;
        try
        {
            throwInt ();
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
        throwArgExc ();
    }
    catch (const IEX_INTERNAL_NAMESPACE::ArgExc&)
    {
        return;
    }
    catch (std::exception&)
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
        throwLogicError ();
    }
    catch (const IEX_INTERNAL_NAMESPACE::ArgExc&)
    {
        assert (false);
    }
    catch (std::exception&)
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
        throwArgExc ();
    }
    catch (std::exception&)
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
        throwInt ();
    }
    catch (const IEX_INTERNAL_NAMESPACE::ArgExc&)
    {
        assert (false);
    }
    catch (std::exception&)
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
test5 ()
{
    std::cout << "5" << std::endl;

    try
    {
        throwNested ();
    }
    catch (const IEX_INTERNAL_NAMESPACE::ArgExc& e)
    {
        assert (std::string (e.what ()) == "ArgExc");
    }
}

std::string
getStackTrace ()
{
    return "???";
}

template <class T>
void
test6 ()
{
    std::cout << "6" << std::endl;

    IEX_INTERNAL_NAMESPACE::setStackTracer (getStackTrace);
    assert (IEX_INTERNAL_NAMESPACE::stackTracer () == getStackTrace);

    //
    // Test the constructors that take char* and stringstream,
    // and the += and assign functions.
    //

    T e0;

    T e1 ("arg");

    e1 += "X";
    std::stringstream ss;
    ss << "Y";
    e1 += ss;

    T e2 (e1);

    assert (e1.message () == "argXY");
    assert (e1.stackTrace () == getStackTrace ());

    assert (e2.message () == e1.message ());
    assert (e2.stackTrace () == e1.stackTrace ());

    e2.assign ("Z");
    assert (e2.message () == "Z");
    e2.assign (ss);
    assert (e2.message () == "Y");

    T e3 (ss);
    assert (e3.message () == ss.str ());

    std::string str4 ("e4");
    T           e4 (str4);
    assert (e4.message () == str4);

    const std::string str5 ("e5");
    T                 e5 (str5);
    assert (e5.message () == str5);

    {
        const T e6 ("e6");
        T       e7 (e6);
        assert (e7.message () == e6.message ());
    }

    {
        e5 = e4;
        assert (e5.message () == e4.message ());
    }

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
        assert (e.message () == e1.message ());
    }
    catch (...)
    {
        assert (false);
    }

    assert (caught);
}

template <class T>
void
testThrowErrnoExc (int err)
{
    bool caught = false;

    try
    {
        IEX_INTERNAL_NAMESPACE::throwErrnoExc ("err", err);
    }
    catch (const T& e)
    {
        caught = true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "testThrowErrnoExc: caught " << typeid (e).name ()
                  << ", expected " << typeid (T).name () << std::endl;
    }

    assert (caught);
}

void
test7 ()
{
#if defined(EPERM)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EpermExc> (EPERM);
#endif
#if defined(ENOENT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnoentExc> (ENOENT);
#endif
#if defined(ESRCH)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EsrchExc> (ESRCH);
#endif
#if defined(EINTR)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EintrExc> (EINTR);
#endif
#if defined(EIO)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EioExc> (EIO);
#endif
#if defined(ENXIO)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnxioExc> (ENXIO);
#endif
#if defined(E2BIG)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::E2bigExc> (E2BIG);
#endif
#if defined(ENOEXEC)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnoexecExc> (ENOEXEC);
#endif
#if defined(EBADF)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EbadfExc> (EBADF);
#endif
#if defined(ECHILD)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EchildExc> (ECHILD);
#endif
#if defined(EAGAIN)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EagainExc> (EAGAIN);
#endif
#if defined(ENOMEM)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnomemExc> (ENOMEM);
#endif
#if defined(EACCES)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EaccesExc> (EACCES);
#endif
#if defined(EFAULT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EfaultExc> (EFAULT);
#endif
#if defined(ENOTBLK)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotblkExc> (ENOTBLK);
#endif
#if defined(EBUSY)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EbusyExc> (EBUSY);
#endif
#if defined(EEXIST)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EexistExc> (EEXIST);
#endif
#if defined(EXDEV)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ExdevExc> (EXDEV);
#endif
#if defined(ENODEV)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnodevExc> (ENODEV);
#endif
#if defined(ENOTDIR)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotdirExc> (ENOTDIR);
#endif
#if defined(EISDIR)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EisdirExc> (EISDIR);
#endif
#if defined(EINVAL)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EinvalExc> (EINVAL);
#endif
#if defined(ENFILE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnfileExc> (ENFILE);
#endif
#if defined(EMFILE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EmfileExc> (EMFILE);
#endif
#if defined(ENOTTY)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnottyExc> (ENOTTY);
#endif
#if defined(ETXTBSY)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EtxtbsyExc> (ETXTBSY);
#endif
#if defined(EFBIG)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EfbigExc> (EFBIG);
#endif
#if defined(ENOSPC)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnospcExc> (ENOSPC);
#endif
#if defined(ESPIPE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EspipeExc> (ESPIPE);
#endif
#if defined(EROFS)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ErofsExc> (EROFS);
#endif
#if defined(EMLINK)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EmlinkExc> (EMLINK);
#endif
#if defined(EPIPE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EpipeExc> (EPIPE);
#endif
#if defined(EDOM)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EdomExc> (EDOM);
#endif
#if defined(ERANGE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ErangeExc> (ERANGE);
#endif
#if defined(ENOMSG)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnomsgExc> (ENOMSG);
#endif
#if defined(EIDRM)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EidrmExc> (EIDRM);
#endif
#if defined(ECHRNG)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EchrngExc> (ECHRNG);
#endif
#if defined(EL2NSYNC)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::El2nsyncExc> (EL2NSYNC);
#endif
#if defined(EL3HLT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::El3hltExc> (EL3HLT);
#endif
#if defined(EL3RST)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::El3rstExc> (EL3RST);
#endif
#if defined(ELNRNG)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ElnrngExc> (ELNRNG);
#endif
#if defined(EUNATCH)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EunatchExc> (EUNATCH);
#endif
#if defined(ENOCSI)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnocsiExc> (ENOCSI);
#endif
#if defined(EL2HLT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::El2hltExc> (EL2HLT);
#endif
#if defined(EDEADLK)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EdeadlkExc> (EDEADLK);
#endif
#if defined(ENOLCK)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnolckExc> (ENOLCK);
#endif
#if defined(EBADE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EbadeExc> (EBADE);
#endif
#if defined(EBADR)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EbadrExc> (EBADR);
#endif
#if defined(EXFULL)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ExfullExc> (EXFULL);
#endif
#if defined(ENOANO)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnoanoExc> (ENOANO);
#endif
#if defined(EBADRQC)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EbadrqcExc> (EBADRQC);
#endif
#if defined(EBADSLT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EbadsltExc> (EBADSLT);
#endif
#if defined(EDEADLOCK) && defined(EDEADLK)
#    if EDEADLOCK != EDEADLK
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EdeadlockExc> (EDEADLOCK);
#    endif
#elif defined(EDEADLOCK)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EdeadlockExc> (EDEADLOCK);
#endif
#if defined(EBFONT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EbfontExc> (EBFONT);
#endif
#if defined(ENOSTR)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnostrExc> (ENOSTR);
#endif
#if defined(ENODATA)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnodataExc> (ENODATA);
#endif
#if defined(ETIME)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EtimeExc> (ETIME);
#endif
#if defined(ENOSR)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnosrExc> (ENOSR);
#endif
#if defined(ENONET)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnonetExc> (ENONET);
#endif
#if defined(ENOPKG)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnopkgExc> (ENOPKG);
#endif
#if defined(EREMOTE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EremoteExc> (EREMOTE);
#endif
#if defined(ENOLINK)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnolinkExc> (ENOLINK);
#endif
#if defined(EADV)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EadvExc> (EADV);
#endif
#if defined(ESRMNT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EsrmntExc> (ESRMNT);
#endif
#if defined(ECOMM)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EcommExc> (ECOMM);
#endif
#if defined(EPROTO)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EprotoExc> (EPROTO);
#endif
#if defined(EMULTIHOP)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EmultihopExc> (EMULTIHOP);
#endif
#if defined(EBADMSG)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EbadmsgExc> (EBADMSG);
#endif
#if defined(ENAMETOOLONG)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnametoolongExc> (ENAMETOOLONG);
#endif
#if defined(EOVERFLOW)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EoverflowExc> (EOVERFLOW);
#endif
#if defined(ENOTUNIQ)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotuniqExc> (ENOTUNIQ);
#endif
#if defined(EBADFD)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EbadfdExc> (EBADFD);
#endif
#if defined(EREMCHG)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EremchgExc> (EREMCHG);
#endif
#if defined(ELIBACC)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ElibaccExc> (ELIBACC);
#endif
#if defined(ELIBBAD)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ElibbadExc> (ELIBBAD);
#endif
#if defined(ELIBSCN)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ElibscnExc> (ELIBSCN);
#endif
#if defined(ELIBMAX)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ElibmaxExc> (ELIBMAX);
#endif
#if defined(ELIBEXEC)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ElibexecExc> (ELIBEXEC);
#endif
#if defined(EILSEQ)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EilseqExc> (EILSEQ);
#endif
#if defined(ENOSYS)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnosysExc> (ENOSYS);
#endif
#if defined(ELOOP)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EloopExc> (ELOOP);
#endif
#if defined(ERESTART)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::ErestartExc> (ERESTART);
#endif
#if defined(ESTRPIPE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EstrpipeExc> (ESTRPIPE);
#endif
#if defined(ENOTEMPTY)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotemptyExc> (ENOTEMPTY);
#endif
#if defined(EUSERS)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EusersExc> (EUSERS);
#endif
#if defined(ENOTSOCK)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotsockExc> (ENOTSOCK);
#endif
#if defined(EDESTADDRREQ)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EdestaddrreqExc> (EDESTADDRREQ);
#endif
#if defined(EMSGSIZE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EmsgsizeExc> (EMSGSIZE);
#endif
#if defined(EPROTOTYPE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EprototypeExc> (EPROTOTYPE);
#endif
#if defined(ENOPROTOOPT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnoprotooptExc> (ENOPROTOOPT);
#endif
#if defined(EPROTONOSUPPORT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EprotonosupportExc> (
        EPROTONOSUPPORT);
#endif
#if defined(ESOCKTNOSUPPORT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EsocktnosupportExc> (
        ESOCKTNOSUPPORT);
#endif
#if defined(EOPNOTSUPP)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EopnotsuppExc> (EOPNOTSUPP);
#endif
#if defined(EPFNOSUPPORT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EpfnosupportExc> (EPFNOSUPPORT);
#endif
#if defined(EAFNOSUPPORT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EafnosupportExc> (EAFNOSUPPORT);
#endif
#if defined(EADDRINUSE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EaddrinuseExc> (EADDRINUSE);
#endif
#if defined(EADDRNOTAVAIL)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EaddrnotavailExc> (EADDRNOTAVAIL);
#endif
#if defined(ENETDOWN)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnetdownExc> (ENETDOWN);
#endif
#if defined(ENETUNREACH)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnetunreachExc> (ENETUNREACH);
#endif
#if defined(ENETRESET)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnetresetExc> (ENETRESET);
#endif
#if defined(ECONNABORTED)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EconnabortedExc> (ECONNABORTED);
#endif
#if defined(ECONNRESET)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EconnresetExc> (ECONNRESET);
#endif
#if defined(ENOBUFS)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnobufsExc> (ENOBUFS);
#endif
#if defined(EISCONN)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EisconnExc> (EISCONN);
#endif
#if defined(ENOTCONN)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotconnExc> (ENOTCONN);
#endif
#if defined(ESHUTDOWN)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EshutdownExc> (ESHUTDOWN);
#endif
#if defined(ETOOMANYREFS)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EtoomanyrefsExc> (ETOOMANYREFS);
#endif
#if defined(ETIMEDOUT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EtimedoutExc> (ETIMEDOUT);
#endif
#if defined(ECONNREFUSED)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EconnrefusedExc> (ECONNREFUSED);
#endif
#if defined(EHOSTDOWN)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EhostdownExc> (EHOSTDOWN);
#endif
#if defined(EHOSTUNREACH)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EhostunreachExc> (EHOSTUNREACH);
#endif
#if defined(EALREADY)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EalreadyExc> (EALREADY);
#endif
#if defined(EINPROGRESS)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EinprogressExc> (EINPROGRESS);
#endif
#if defined(ESTALE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EstaleExc> (ESTALE);
#endif
#if defined(EIORESID)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EioresidExc> (EIORESID);
#endif
#if defined(EUCLEAN)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EucleanExc> (EUCLEAN);
#endif
#if defined(ENOTNAM)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotnamExc> (ENOTNAM);
#endif
#if defined(ENAVAIL)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnavailExc> (ENAVAIL);
#endif
#if defined(EISNAM)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EisnamExc> (EISNAM);
#endif
#if defined(EREMOTEIO)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EremoteioExc> (EREMOTEIO);
#endif
#if defined(EINIT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EinitExc> (EINIT);
#endif
#if defined(EREMDEV)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EremdevExc> (EREMDEV);
#endif
#if defined(ECANCELED)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EcanceledExc> (ECANCELED);
#endif
#if defined(ENOLIMFILE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnolimfileExc> (ENOLIMFILE);
#endif
#if defined(EPROCLIM)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EproclimExc> (EPROCLIM);
#endif
#if defined(EDISJOINT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EdisjointExc> (EDISJOINT);
#endif
#if defined(ENOLOGIN)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnologinExc> (ENOLOGIN);
#endif
#if defined(ELOGINLIM)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EloginlimExc> (ELOGINLIM);
#endif
#if defined(EGROUPLOOP)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EgrouploopExc> (EGROUPLOOP);
#endif
#if defined(ENOATTACH)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnoattachExc> (ENOATTACH);
#endif
#if defined(ENOTSUP) && defined(EOPNOTSUPP)
#    if ENOTSUP != EOPNOTSUPP
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotsupExc> (ENOTSUP);
#    endif
#elif defined(ENOTSUP)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotsupExc> (ENOTSUP);
#endif
#if defined(ENOATTR)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnoattrExc> (ENOATTR);
#endif
#if defined(EDIRCORRUPTED)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EdircorruptedExc> (EDIRCORRUPTED);
#endif
#if defined(EDQUOT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EdquotExc> (EDQUOT);
#endif
#if defined(ENFSREMOTE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnfsremoteExc> (ENFSREMOTE);
#endif
#if defined(ECONTROLLER)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EcontrollerExc> (ECONTROLLER);
#endif
#if defined(ENOTCONTROLLER)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotcontrollerExc> (
        ENOTCONTROLLER);
#endif
#if defined(EENQUEUED)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EenqueuedExc> (EENQUEUED);
#endif
#if defined(ENOTENQUEUED)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotenqueuedExc> (ENOTENQUEUED);
#endif
#if defined(EJOINED)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EjoinedExc> (EJOINED);
#endif
#if defined(ENOTJOINED)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotjoinedExc> (ENOTJOINED);
#endif
#if defined(ENOPROC)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnoprocExc> (ENOPROC);
#endif
#if defined(EMUSTRUN)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EmustrunExc> (EMUSTRUN);
#endif
#if defined(ENOTSTOPPED)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnotstoppedExc> (ENOTSTOPPED);
#endif
#if defined(ECLOCKCPU)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EclockcpuExc> (ECLOCKCPU);
#endif
#if defined(EINVALSTATE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EinvalstateExc> (EINVALSTATE);
#endif
#if defined(ENOEXIST)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnoexistExc> (ENOEXIST);
#endif
#if defined(EENDOFMINOR)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EendofminorExc> (EENDOFMINOR);
#endif
#if defined(EBUFSIZE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EbufsizeExc> (EBUFSIZE);
#endif
#if defined(EEMPTY)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EemptyExc> (EEMPTY);
#endif
#if defined(ENOINTRGROUP)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EnointrgroupExc> (ENOINTRGROUP);
#endif
#if defined(EINVALMODE)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EinvalmodeExc> (EINVALMODE);
#endif
#if defined(ECANTEXTENT)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EcantextentExc> (ECANTEXTENT);
#endif
#if defined(EINVALTIME)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EinvaltimeExc> (EINVALTIME);
#endif
#if defined(EDESTROYED)
    testThrowErrnoExc<IEX_INTERNAL_NAMESPACE::EdestroyedExc> (EDESTROYED);
#endif
}

} // namespace

void
testBaseExc ()
{
    std::cout << "See if throw and catch work:" << std::endl;

    test1 ();
    test2 ();
    test3 ();
    test4 ();
    test5 ();

    test6<IEX_INTERNAL_NAMESPACE::ArgExc> ();
    test6<IEX_INTERNAL_NAMESPACE::LogicExc> ();
    test6<IEX_INTERNAL_NAMESPACE::InputExc> ();
    test6<IEX_INTERNAL_NAMESPACE::IoExc> ();
    test6<IEX_INTERNAL_NAMESPACE::MathExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ErrnoExc> ();
    test6<IEX_INTERNAL_NAMESPACE::NoImplExc> ();
    test6<IEX_INTERNAL_NAMESPACE::NullExc> ();
    test6<IEX_INTERNAL_NAMESPACE::TypeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EpermExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnoentExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EsrchExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EintrExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EioExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnxioExc> ();
    test6<IEX_INTERNAL_NAMESPACE::E2bigExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnoexecExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EbadfExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EchildExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EagainExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnomemExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EaccesExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EfaultExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotblkExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EbusyExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EexistExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ExdevExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnodevExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotdirExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EisdirExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EinvalExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnfileExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EmfileExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnottyExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EtxtbsyExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EfbigExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnospcExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EspipeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ErofsExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EmlinkExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EpipeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EdomExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ErangeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnomsgExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EidrmExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EchrngExc> ();
    test6<IEX_INTERNAL_NAMESPACE::El2nsyncExc> ();
    test6<IEX_INTERNAL_NAMESPACE::El3hltExc> ();
    test6<IEX_INTERNAL_NAMESPACE::El3rstExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ElnrngExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EunatchExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnocsiExc> ();
    test6<IEX_INTERNAL_NAMESPACE::El2hltExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EdeadlkExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnolckExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EbadeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EbadrExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ExfullExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnoanoExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EbadrqcExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EbadsltExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EdeadlockExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EbfontExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnostrExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnodataExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EtimeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnosrExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnonetExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnopkgExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EremoteExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnolinkExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EadvExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EsrmntExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EcommExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EprotoExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EmultihopExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EbadmsgExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnametoolongExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EoverflowExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotuniqExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EbadfdExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EremchgExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ElibaccExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ElibbadExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ElibscnExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ElibmaxExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ElibexecExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EilseqExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnosysExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EloopExc> ();
    test6<IEX_INTERNAL_NAMESPACE::ErestartExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EstrpipeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotemptyExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EusersExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotsockExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EdestaddrreqExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EmsgsizeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EprototypeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnoprotooptExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EprotonosupportExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EsocktnosupportExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EopnotsuppExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EpfnosupportExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EafnosupportExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EaddrinuseExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EaddrnotavailExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnetdownExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnetunreachExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnetresetExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EconnabortedExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EconnresetExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnobufsExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EisconnExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotconnExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EshutdownExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EtoomanyrefsExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EtimedoutExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EconnrefusedExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EhostdownExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EhostunreachExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EalreadyExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EinprogressExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EstaleExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EioresidExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EucleanExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotnamExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnavailExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EisnamExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EremoteioExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EinitExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EremdevExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EcanceledExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnolimfileExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EproclimExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EdisjointExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnologinExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EloginlimExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EgrouploopExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnoattachExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotsupExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnoattrExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EdircorruptedExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EdquotExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnfsremoteExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EcontrollerExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotcontrollerExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EenqueuedExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotenqueuedExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EjoinedExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotjoinedExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnoprocExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EmustrunExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnotstoppedExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EclockcpuExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EinvalstateExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnoexistExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EendofminorExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EbufsizeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EemptyExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EnointrgroupExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EinvalmodeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EcantextentExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EinvaltimeExc> ();
    test6<IEX_INTERNAL_NAMESPACE::EdestroyedExc> ();
    test6<IEX_INTERNAL_NAMESPACE::OverflowExc> ();
    test6<IEX_INTERNAL_NAMESPACE::UnderflowExc> ();
    test6<IEX_INTERNAL_NAMESPACE::DivzeroExc> ();
    test6<IEX_INTERNAL_NAMESPACE::InexactExc> ();
    test6<IEX_INTERNAL_NAMESPACE::InvalidFpOpExc> ();

    test7 ();

    std::cout << "ok\n" << std::endl;
}
