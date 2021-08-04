//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <tmpDir.h>

#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfArray.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfVersion.h>
#include <half.h>

#include <ImfBoxAttribute.h>
#include <ImfChannelListAttribute.h>
#include <ImfCompressionAttribute.h>
#include <ImfChromaticitiesAttribute.h>
#include <ImfFloatAttribute.h>
#include <ImfFloatVectorAttribute.h>
#include <ImfEnvmapAttribute.h>
#include <ImfDeepImageStateAttribute.h>
#include <ImfDoubleAttribute.h>
#include <ImfIntAttribute.h>
#include <ImfLineOrderAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfOpaqueAttribute.h>
#include <ImfStringAttribute.h>
#include <ImfStringVectorAttribute.h>
#include <ImfVecAttribute.h>

#include <stdio.h>
#ifdef NDEBUG
#    undef NDEBUG
#endif
#include <assert.h>



namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;

using namespace std;
using namespace IMATH_NAMESPACE;


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class TestOpaque 
{
   public:
    TestOpaque () : magic (666) {}
    TestOpaque (int m) : magic (m) {}
    int  magic;
};

bool
operator == (const TestOpaque& a, const TestOpaque& b)
{
    return a.magic == b.magic;
}

typedef TypedAttribute<TestOpaque> TestOpaqueAttribute;

template <>
const char *
TestOpaqueAttribute::staticTypeName ()
{
    return "testOpaque";
}
    
template <>
void
TestOpaqueAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream& os, int version) const
{
    Xdr::write <StreamIO> (os, _value.magic);
}
                              
template <>
void
TestOpaqueAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is, int size, int version)
{
    Xdr::read <StreamIO> (is, _value.magic);
}

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

namespace {

void
fillPixels (Array2D<float> &pf, int width, int height)
{
    for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x)
	    pf[y][x] = x % 10 + 10 * (y % 17);
}


void
writeReadAttr (const Array2D<float> &pf1,
	       const char fileName[],
	       int width,
	       int height)
{
    TestOpaqueAttribute::registerAttributeType();
    
    //
    // We don't test ChannelList, LineOrder, Compression and opaque
    // attributes here; those types are covered by other tests.
    //

    Box2i  a1  (V2i (1, 2), V2i (3, 4));
    Box2f  a2  (V2f (1.5, 2.5), V2f (3.5, 4.5));
    float  a3  (3.14159);
    int    a4  (17);
    M33f   a5  (11, 12, 13, 14, 15, 16, 17, 18, 19);
    M44f   a6  (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    string a7  ("extensive rebuilding by Nebuchadrezzar has left");
    V2i    a8  (27, 28);
    V2f    a9  (27.5, 28.5);
    V3i    a10 (37, 38, 39);
    V3f    a11 (37.5, 38.5, 39.5);
    double a12 (7.12342341419);
    Chromaticities a13 (V2f (1, 2), V2f (3, 4), V2f (5, 6), V2f (7, 8));
    Envmap a14 (ENVMAP_CUBE);

    StringVector a15;
    a15.push_back ("who can spin");
    a15.push_back ("");
    a15.push_back ("straw into");
    a15.push_back ("gold");
    M33d   a16 (12, 13, 14, 15, 16, 17, 18, 19, 20);
    M44d   a17 (2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17);
    V2d    a18 (27.51, 28.51);
    V3d    a19 (37.51, 38.51, 39.51);
    
    StringVector a20;

    DeepImageState a21 (DIS_TIDY);

    FloatVector a22;
    FloatVector a23;
    a23.push_back (1.5f);
    a23.push_back (-1.5f);
    a23.push_back (15.0f);
    a23.push_back (150.0f);

    TestOpaque  a24 (42);

    //
    // Write an image file with extra attributes in the header
    //

    {
	Header hdr (width, height);

	hdr.insert ("a1",  Box2iAttribute          (a1));
	hdr.insert ("a2",  Box2fAttribute          (a2));
	hdr.insert ("a3",  FloatAttribute          (a3));
	hdr.insert ("a4",  IntAttribute            (a4));
	hdr.insert ("a5",  M33fAttribute           (a5));
	hdr.insert ("a6",  M44fAttribute           (a6));
	hdr.insert ("a7",  StringAttribute         (a7));
	hdr.insert ("a8",  V2iAttribute            (a8));
	hdr.insert ("a9",  V2fAttribute            (a9));
	hdr.insert ("a10", V3iAttribute            (a10));
	hdr.insert ("a11", V3fAttribute            (a11));
	hdr.insert ("a12", DoubleAttribute         (a12));
	hdr.insert ("a13", ChromaticitiesAttribute (a13));
	hdr.insert ("a14", EnvmapAttribute         (a14));
	hdr.insert ("a15", StringVectorAttribute   (a15));
	hdr.insert ("a16", M33dAttribute           (a16));
	hdr.insert ("a17", M44dAttribute           (a17));
	hdr.insert ("a18", V2dAttribute            (a18));
	hdr.insert ("a19", V3dAttribute            (a19));
	hdr.insert ("a20", StringVectorAttribute   (a20));
	hdr.insert ("a21", DeepImageStateAttribute (a21));
	hdr.insert ("a22", FloatVectorAttribute    (a22));
	hdr.insert ("a23", FloatVectorAttribute    (a23));
        hdr.insert ("a24", TestOpaqueAttribute          (a24));
        
	hdr.channels().insert ("F",			// name
			       Channel (IMF::FLOAT,	// type
					1,		// xSampling
					1)		// ySampling
			      );

	FrameBuffer fb; 

	fb.insert ("F",					// name
		   Slice (IMF::FLOAT,			// type
			  (char *) &pf1[0][0],		// base
			  sizeof (pf1[0][0]), 		// xStride
			  sizeof (pf1[0][0]) * width,	// yStride
			  1,				// xSampling
			  1)				// ySampling
		  );

	cout << "writing" << flush;

	remove (fileName);
	OutputFile out (fileName, hdr);
	out.setFrameBuffer (fb);
	out.writePixels (height);
    }

    //
    // Test the handling of "opaque" attributes that preserve
    // attributes whose type is not recognized; remove the definition
    // of the "testOpaque" type now that the file has been written.
    //

    TestOpaqueAttribute::unRegisterAttributeType();
    
    //
    // Read the header back from the file, and see if the
    // values of the extra attributes come back correctly.
    //

    {
	cout << " reading" << flush;

	InputFile in (fileName);

	cout << " (version " << in.version() << ")" << flush;

	const Header &hdr = in.header();

	assert (hdr.typedAttribute <Box2iAttribute>  ("a1").value()  == a1);
	assert (hdr.typedAttribute <Box2fAttribute>  ("a2").value()  == a2);
	assert (hdr.typedAttribute <FloatAttribute>  ("a3").value()  == a3);
	assert (hdr.typedAttribute <IntAttribute>    ("a4").value()  == a4);
	assert (hdr.typedAttribute <M33fAttribute>   ("a5").value()  == a5);
	assert (hdr.typedAttribute <M44fAttribute>   ("a6").value()  == a6);
	assert (hdr.typedAttribute <StringAttribute> ("a7").value()  == a7);
	assert (hdr.typedAttribute <V2iAttribute>    ("a8").value()  == a8);
	assert (hdr.typedAttribute <V2fAttribute>    ("a9").value()  == a9);
	assert (hdr.typedAttribute <V3iAttribute>    ("a10").value() == a10);
	assert (hdr.typedAttribute <V3fAttribute>    ("a11").value() == a11);
	assert (hdr.typedAttribute <DoubleAttribute> ("a12").value() == a12);

	assert (hdr.typedAttribute <ChromaticitiesAttribute>
					("a13").value().red == a13.red);

	assert (hdr.typedAttribute <ChromaticitiesAttribute>
					("a13").value().green == a13.green);

	assert (hdr.typedAttribute <ChromaticitiesAttribute>
					("a13").value().blue == a13.blue);

	assert (hdr.typedAttribute <ChromaticitiesAttribute>
					("a13").value().white == a13.white);

	assert (hdr.typedAttribute <EnvmapAttribute> ("a14").value() == a14);

	assert (hdr.typedAttribute <StringVectorAttribute>
					("a15").value().size() == 4);

	assert (hdr.typedAttribute <StringVectorAttribute>
					("a15").value()[0] == "who can spin");

	assert (hdr.typedAttribute <StringVectorAttribute>
					("a15").value()[1] == "");

	assert (hdr.typedAttribute <StringVectorAttribute>
					("a15").value()[2] == "straw into");

	assert (hdr.typedAttribute <StringVectorAttribute>
					("a15").value()[3] == "gold");

	assert (hdr.typedAttribute <M33dAttribute>   ("a16").value()  == a16);
	assert (hdr.typedAttribute <M44dAttribute>   ("a17").value()  == a17);
	assert (hdr.typedAttribute <V2dAttribute>    ("a18").value()  == a18);
	assert (hdr.typedAttribute <V3dAttribute>    ("a19").value()  == a19);

        assert (hdr.typedAttribute <StringVectorAttribute>
                                        ("a20").value() == a20);

	assert (hdr.typedAttribute <DeepImageStateAttribute>
					("a21").value() == a21);

        assert (hdr.typedAttribute <FloatVectorAttribute>
                                        ("a22").value() == a22);

        assert (hdr.typedAttribute <FloatVectorAttribute>
                                        ("a23").value() == a23);

        //
        // a24 should be an opaque attribute with type "testOpaque".
        //

        const Attribute& a = hdr["a24"];
        const OpaqueAttribute* oa = dynamic_cast <const OpaqueAttribute *> (&a);
        assert (oa);
        assert (!strcmp (a.typeName(), "testOpaque"));

        // test the copy constructor
        OpaqueAttribute b (*oa);
        assert (!strcmp (b.typeName(), a.typeName()));
        assert (b.dataSize() == oa->dataSize());
        for (int i=0; i<b.dataSize(); i++)
            assert (b.data()[i] == oa->data()[i]);
    }

    remove (fileName);
    cout << endl;
}


void
channelList ()
{
    cout << "channel list" << endl;

    {
	// test channelsWithPrefix()

	ChannelList channels;

	channels.insert ("b0", Channel (IMF::HALF, 1, 1));
	channels.insert ("b1", Channel (IMF::HALF, 1, 1));
	channels.insert ("b2", Channel (IMF::HALF, 1, 1));
	channels.insert ("d3", Channel (IMF::HALF, 1, 1));
	channels.insert ("e4", Channel (IMF::HALF, 1, 1));

	ChannelList::Iterator first;
	ChannelList::Iterator last;

	channels.channelsWithPrefix ("a", first, last);
	assert (first != channels.end());
	assert (first == last);

	channels.channelsWithPrefix ("b", first, last);
	assert (first != channels.end());
	assert (first != last);
	assert (first++.name() == Name ("b0"));
	assert (first++.name() == Name ("b1"));
	assert (first++.name() == Name ("b2"));
	assert (first == last);

	channels.channelsWithPrefix ("b1", first, last);
	assert (first != channels.end());
	assert (first != last);
	assert (first++.name() == Name ("b1"));
	assert (first == last);

	channels.channelsWithPrefix ("b11", first, last);
	assert (first != channels.end());
	assert (first == last);

	channels.channelsWithPrefix ("c", first, last);
	assert (first != channels.end());
	assert (first == last);

	channels.channelsWithPrefix ("d", first, last);
	assert (first != channels.end());
	assert (first != last);
	assert (first++.name() == Name ("d3"));
	assert (first == last);

	channels.channelsWithPrefix ("e", first, last);
	assert (first != channels.end());
	assert (first != last);
	assert (first++.name() == Name ("e4"));
	assert (first == last);

	channels.channelsWithPrefix ("f", first, last);
	assert (first == channels.end());
	assert (first == last);
    }

    {
	// Test support for layers
	
	ChannelList channels;

	channels.insert ("a",   Channel (IMF::HALF, 1, 1));
	channels.insert (".a",  Channel (IMF::HALF, 1, 1));
	channels.insert ("a.",  Channel (IMF::HALF, 1, 1));

	channels.insert ("layer1.R", Channel (IMF::HALF, 1, 1));
	channels.insert ("layer1.G", Channel (IMF::HALF, 1, 1));
	channels.insert ("layer1.B", Channel (IMF::HALF, 1, 1));

	channels.insert ("layer1.sublayer1.AA", Channel (IMF::HALF, 1, 1));
	channels.insert ("layer1.sublayer1.R", Channel  (IMF::HALF, 1, 1));
	channels.insert ("layer1.sublayer1.G", Channel  (IMF::HALF, 1, 1));
	channels.insert ("layer1.sublayer1.B", Channel  (IMF::HALF, 1, 1));

	channels.insert ("layer1.sublayer2.R", Channel (IMF::HALF, 1, 1));

	channels.insert ("layer2.R", Channel (IMF::HALF, 1, 1));
	channels.insert ("layer2.G", Channel (IMF::HALF, 1, 1));
	channels.insert ("layer2.B", Channel (IMF::HALF, 1, 1));

	set <string> layerNames;
	channels.layers (layerNames);

	set<string>::iterator i = layerNames.begin();
	assert (*i++ == "layer1");
	assert (*i++ == "layer1.sublayer1");
	assert (*i++ == "layer1.sublayer2");
	assert (*i++ == "layer2");
	assert (i == layerNames.end());

	ChannelList::ConstIterator first, last;

	channels.channelsInLayer ("layer1.sublayer1", first, last);
	assert (first != channels.end());
	assert (first != last);
	assert (first++.name() == Name ("layer1.sublayer1.AA"));
	assert (first++.name() == Name ("layer1.sublayer1.B"));
	assert (first++.name() == Name ("layer1.sublayer1.G"));
	assert (first++.name() == Name ("layer1.sublayer1.R"));
	assert (first == last);

	channels.channelsInLayer ("layer2", first, last);
	assert (first != channels.end());
	assert (first != last);
	assert (first++.name() == Name ("layer2.B"));
	assert (first++.name() == Name ("layer2.G"));
	assert (first++.name() == Name ("layer2.R"));
	assert (first == last);
    }
}


void
longNames (const Array2D<float> &pf1,
           const char fileName[],
           int width,
           int height)
{
    //
    // Verify that long attibute or channel names in the header
    // set the LONG_NAMES_FLAG in the file version number.
    //

    FrameBuffer fb; 

    fb.insert ("F",					// name
               Slice (IMF::FLOAT,			// type
                      (char *) &pf1[0][0],		// base
                      sizeof (pf1[0][0]), 		// xStride
                      sizeof (pf1[0][0]) * width,	// yStride
                      1,				// xSampling
                      1)				// ySampling
              );

    cout << "only short names" << endl;

    {
	Header hdr (width, height);

	hdr.channels().insert ("F",			// name
			       Channel (IMF::FLOAT,	// type
					1,		// xSampling
					1)		// ySampling
			      );


	cout << "writing" << flush;

	remove (fileName);
	OutputFile out (fileName, hdr);
	out.setFrameBuffer (fb);
	out.writePixels (height);
    }

    {
	cout << " reading" << endl;

	InputFile in (fileName);
        assert (!(in.version() & LONG_NAMES_FLAG));
    }

    static const char longName[] = "x2345678901234567890123456789012";

    cout << "long attribute name" << endl;

    {
	Header hdr (width, height);
	hdr.insert (longName, StringAttribute ("y"));

	hdr.channels().insert ("F",			// name
			       Channel (IMF::FLOAT,	// type
					1,		// xSampling
					1)		// ySampling
			      );

	cout << "writing" << flush;

	remove (fileName);
	OutputFile out (fileName, hdr);
	out.setFrameBuffer (fb);
	out.writePixels (height);
    }

    {
	cout << " reading" << endl;

	InputFile in (fileName);
        assert (in.version() & LONG_NAMES_FLAG);

	const Header &hdr = in.header();
	assert (hdr.typedAttribute <StringAttribute> (longName).value() == "y");
    }

    cout << "long channel name" << endl;

    {
	Header hdr (width, height);

	hdr.channels().insert (longName,		// name
			       Channel (IMF::FLOAT,	// type
					1,		// xSampling
					1)		// ySampling
			      );

	cout << "writing" << flush;

	remove (fileName);
	OutputFile out (fileName, hdr);
	out.setFrameBuffer (fb);
	out.writePixels (height);
    }

    {
	cout << " reading" << endl;

	InputFile in (fileName);
        assert (in.version() & LONG_NAMES_FLAG);

	const Header &hdr = in.header();
	assert (hdr.channels().findChannel (longName));
    }

    remove (fileName);
    cout << endl;
}


} // namespace


template<class T> void 
print_type(const OPENEXR_IMF_NAMESPACE::TypedAttribute<T> & object)
{
    cout << object.typeName() << endl;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

//
// Test to confirm that the copy/move constructors are implemented
// properly.
//

static int default_constructor;
static int destructor;
static int copy_constructor;
static int assignment_operator;
static int move_constructor;
static int move_assignment_operator;

struct TestType 
{
    TestType() 
    {
        default_constructor++;
    }

    ~TestType() 
    {
        destructor++;
    }
    
    TestType (const TestType& other) 
        : _f (other._f)
    {
        copy_constructor++;
    }

    TestType& operator = (const TestType& other)
    {
        assignment_operator++;
        _f = other._f;
        return *this;
    }

    TestType (TestType&& other) 
        : _f (std::move (other._f))
    {
        move_constructor++;
    }

    TestType& operator = (TestType&& other)
    {
        move_assignment_operator++;
        _f = std::move (other._f);
        return *this;
    }

    static TestType func()
    {
        TestType t;
        return t;
    }
    
    static TestType arg (TestType a)
    {
        return a;
    }
    
    int _f;
    
    static void assert_count (int dc, int d, int cc, int ao, int mc, int mao)
    {
        assert (dc == default_constructor);
        assert (d == destructor);
        assert (cc == copy_constructor);
        assert (ao == assignment_operator);
        assert (mc == move_constructor);
        assert (mao == move_assignment_operator);
    }
        
    static void reset()
    {
        default_constructor = 0;
        destructor = 0;
        copy_constructor = 0;
        assignment_operator = 0;
        move_constructor = 0;
        move_assignment_operator = 0;
    }

    static std::string str()
    {
        std::stringstream s;
        if (default_constructor)
            s << "default_constructor=" << default_constructor << std::endl;        
        if (destructor)
            s << "destructor=" << destructor << std::endl;        

        if (copy_constructor)
            s << "copy_constructor=" << copy_constructor << std::endl;        

        if (assignment_operator)
            s << "assignment_operator=" << assignment_operator << std::endl;        
        if (move_constructor)
            s << "move_constructor=" << move_constructor << std::endl;        

        if (move_assignment_operator)
            s << "move_assignment_operator=" << move_assignment_operator << std::endl;        
        return s.str();
    }
};

typedef TypedAttribute<TestType> TestTypedAttribute;

template <>
const char *
TestTypedAttribute::staticTypeName ()
{
    return "test";
}

template <>
void
TestTypedAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
}

template <>
void
TestTypedAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
}

TestType
testTypeFunc()
{
    return TestType();
}

TestTypedAttribute
testFunc()
{
    TestTypedAttribute a;
    return a;
}

TestTypedAttribute
testArg (TestTypedAttribute a)
{
    return a;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


void
testTypedAttribute()
{
    std::cout << "running testTypedAttribute()\n";
    
    using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;
    
    //
    // Validate the test type
    //
    
    TestType A;
    TestType::assert_count (1, 0, 0, 0, 0, 0);
    TestType::reset();

    TestType B (A);
    TestType::assert_count (0, 0, 1, 0, 0, 0);
    TestType::reset();

    B = A;
    TestType::assert_count (0, 0, 0, 1, 0, 0);
    TestType::reset();

    A = std::move (B);
    TestType::assert_count (0, 0, 0, 0, 0, 1);
    TestType::reset();

    A = TestType::func();
    TestType::assert_count (1, 1, 0, 0, 0, 1);
    TestType::reset();

    A = TestType::arg(B);
    TestType::assert_count (0, 2, 1, 0, 1, 1);
    TestType::reset();

    //
    // Test the attribute type
    //
    
    TestTypedAttribute a;
    TestType::assert_count (1, 0, 0, 0, 0, 0);
    TestType::reset();

    {
        TestType x;
    }
    TestType::assert_count (1, 1, 0, 0, 0, 0);
    TestType::reset();
    
    TestTypedAttribute b(a);
    TestType::assert_count (0, 0, 1, 0, 0, 0);
    TestType::reset();

    a = b;
    TestType::assert_count (0, 0, 0, 1, 0, 0);
    TestType::reset();

    a = std::move (b);
    TestType::assert_count (0, 0, 0, 0, 0, 1);
    TestType::reset();

    a = testFunc();
    TestType::assert_count (1, 1, 0, 0, 0, 1);
    TestType::reset();

    a = testArg(b);
    TestType::assert_count (0, 2, 1, 0, 1, 1);
    TestType::reset();

    std::cout << "ok." << std::endl;
}

void
testAttributes (const std::string &tempDir)
{
    try
    {
	cout << "Testing built-in attributes" << endl;

        testTypedAttribute();

	const int W = 217;
	const int H = 197;

	Array2D<float> pf (H, W);
	fillPixels (pf, W, H);

	std::string filename = tempDir + "imf_test_attr.exr";

	writeReadAttr (pf, filename.c_str(), W, H);
	channelList();
        longNames(pf, filename.c_str(), W, H);

        print_type(OPENEXR_IMF_NAMESPACE::TypedAttribute<int>());

	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
