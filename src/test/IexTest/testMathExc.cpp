//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <testMathExc.h>
#include <mathFuncs.h>
#include <IexMathFloatExc.h>
#include <IexMathExc.h>
#include <iostream>
#include <assert.h>
#include <string.h>

namespace {

void
print (float f)
{
    std::cout << f << std::endl;
}


void
test1 ()
{
    //
    // Turn math exception handling off, and verify that no C++ exceptions
    // are thrown even though "exceptional" floating-point operations are
    // performed.
    //

    std::cout << "invalid operations / exception handling off" << std::endl;

    IEX_INTERNAL_NAMESPACE::mathExcOn (0);

    for (int i = 0; i < 3; ++i)
    {
	try
	{
	    print (divide (1, 0));	// division by zero
	    print (root (-1));		// invalid operation
	    print (grow (1000, 100));	// overflow
	}
	catch (...)
	{
	    assert (false);
	}
    }
}


void
test2a ()
{
    try
    {
	print (divide (1, 0));		// division by zero
	assert (false); // note, this happens when running under valgrind
    }
    catch (const IEX_INTERNAL_NAMESPACE::DivzeroExc &e)
    {
	std::cout << "caught exception: " << e.what() << std::endl;
    }
}


void
test2b ()
{
    try
    {
	print (root (-1));		// invalid operation
	assert (false); // note, this happens when running under valgrind
    }
    catch (const IEX_INTERNAL_NAMESPACE::InvalidFpOpExc &e)
    {
	std::cout << "caught exception: " << e.what() << std::endl;
    }
}


void
test2c ()
{
    try
    {
	print (grow (1000, 100));	// overflow
	assert (false); // note, this happens when running under valgrind
    }
    catch (const IEX_INTERNAL_NAMESPACE::OverflowExc &e)
    {
	std::cout << "caught exception: " << e.what() << std::endl;
    }
}


void
test2 ()
{
    //
    // Turn math exception handling on, and verify that C++ exceptions
    // are thrown when "exceptional" floating-point operations are
    // performed.
    //

    std::cout << "invalid operations / exception handling on" << std::endl;

    IEX_INTERNAL_NAMESPACE::mathExcOn (IEX_INTERNAL_NAMESPACE::IEEE_OVERFLOW | IEX_INTERNAL_NAMESPACE::IEEE_DIVZERO | IEX_INTERNAL_NAMESPACE::IEEE_INVALID);

    for (int i = 0; i < 3; ++i)
    {
#if defined(BUILD) && defined(__VERSION__)
    // magic voodoo so that QUOTE(BUILD)
    // turns into quoted "LINUX_AMD64_OPT_DEBUG"
    #define QUOTE0(x) #x
    #define QUOTE(x) QUOTE0(x)
        if (strcmp(QUOTE(BUILD), "LINUX_AMD64_OPT_DEBUG") == 0
         && strcmp(__VERSION__, "4.1.2 20080704 (Red Hat 4.1.2-44)") == 0)
            std::cout << "WARNING: skipping 1/0 test since it's known to fail"
                      << " in optimized builds using compiler version"
                      << " \""<<__VERSION__<<"\""
                      << " (see SQ42578)"
                      << std::endl;
        else
#endif
            test2a();

	test2b();
	test2c();
    }
}


void
test3 ()
{
    //
    // Verify that getMathExcOn() returns the value that
    // was most recently set with setMathExcOn().
    //

    std::cout << "getMathExc()" << std::endl;

    int when = 0;

    IEX_INTERNAL_NAMESPACE::mathExcOn (when);
    assert (IEX_INTERNAL_NAMESPACE::getMathExcOn() == when);

    when = IEX_INTERNAL_NAMESPACE::IEEE_OVERFLOW;

    IEX_INTERNAL_NAMESPACE::mathExcOn (when);
    assert (IEX_INTERNAL_NAMESPACE::getMathExcOn() == when);

    when = IEX_INTERNAL_NAMESPACE::IEEE_DIVZERO;

    IEX_INTERNAL_NAMESPACE::mathExcOn (when);
    assert (IEX_INTERNAL_NAMESPACE::getMathExcOn() == when);

    when = IEX_INTERNAL_NAMESPACE::IEEE_INVALID;

    IEX_INTERNAL_NAMESPACE::mathExcOn (when);
    assert (IEX_INTERNAL_NAMESPACE::getMathExcOn() == when);

    when = IEX_INTERNAL_NAMESPACE::IEEE_OVERFLOW | IEX_INTERNAL_NAMESPACE::IEEE_DIVZERO | IEX_INTERNAL_NAMESPACE::IEEE_INVALID;

    IEX_INTERNAL_NAMESPACE::mathExcOn (when);
    assert (IEX_INTERNAL_NAMESPACE::getMathExcOn() == when);
}

} // namespace


void
testMathExc()
{
    std::cout << "See if floating-point exceptions work:" << std::endl;

#if defined (GCC_VERSION_295) || defined (GCC_VERSION_296) || defined (ICC_VERSION_71)

         std::cout << "\nWARNING: test skipped for GCC295/296/ICC Linux / IA32.\n"
                      "Floating-point exceptions do not work yet." << std::endl;

#else
    test1();
    test2();
    test3();

    std::cout << " ok" << std::endl;
#endif

}
