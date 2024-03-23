//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//      exrstdattr -- a program that can set the values of most
//      standard attributes in an OpenEXR image file's header.
//
//-----------------------------------------------------------------------------

#include <ImathNamespace.h>
#include <ImfDeepScanLineInputPart.h>
#include <ImfDeepScanLineOutputPart.h>
#include <ImfDeepTiledInputPart.h>
#include <ImfDeepTiledOutputPart.h>
#include <ImfInputFile.h>
#include <ImfInputPart.h>
#include <ImfIntAttribute.h>
#include <ImfMultiPartInputFile.h>
#include <ImfMultiPartOutputFile.h>
#include <ImfNamespace.h>
#include <ImfOutputFile.h>
#include <ImfOutputPart.h>
#include <ImfPartType.h>
#include <ImfStandardAttributes.h>
#include <ImfTiledInputPart.h>
#include <ImfTiledOutputFile.h>
#include <ImfTiledOutputPart.h>
#include <ImfVecAttribute.h>
#include <ImfMisc.h>
#include <OpenEXRConfig.h>

#include <exception>
#include <iostream>
#include <sstream>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;
using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;

void
usageMessage (ostream& stream, const char* program_name, bool verbose = false)
{
    stream << "Usage: " << program_name << " [commands] infile outfile" << endl;

    if (verbose)
        stream
            << "\n"
               "Read OpenEXR image file infile, set the values of one\n"
               "or more attributes in the headers of the file, and save\n"
               "the result in outfile.  Infile and outfile must not refer\n"
               "to the same file (the program cannot edit an image file "
               "\"in place\").\n"
               "\n"
               "Command for selecting headers:\n"
               "\n"
               "  -part i\n"
               "        If i is greater than or equal to zero, and less\n"
               "        than the number of parts in the input file, then\n"
               "        the header for part i becomes \"current.\" If i\n"
               "        is \"any\" or -1, then all headers become current.\n"
               "        Subsequent attribute setting commands affect only\n"
               "        the current header or headers.  All headers are\n"
               "        current before the first -part command.\n"
               "\n"
               "        For example, the command sequence\n"
               "\n"
               "         -focus 3 -part 2 -aperture 8 -expTime 0.01 "
               "-part any -owner luke\n"
               "\n"
               "        sets the focus and owner attributes in all\n"
               "        headers, as well as the aperture and expTime\n"
               "        attributes in the header of part 2.\n"
               "\n"
               "Commands for setting attribute values:\n"
               "\n"
               "  -chromaticities f f f f f f f f\n"
               "        CIE xy chromaticities for the red, green\n"
               "        and blue primaries, and for the white point\n"
               "        (8 floats)\n"
               "\n"
               "  -whiteLuminance f\n"
               "        white luminance, in candelas per square meter\n"
               "        (float, >= 0.0)\n"
               "\n"
               "  -adoptedNeutral f f\n"
               "        CIE xy coordinates that should be considered\n"
               "        \"neutral\" during color rendering.  Pixels in\n"
               "        the image file whose xy coordinates match the\n"
               "        adoptedNeutral value should be mapped to neutral\n"
               "        values on the display. (2 floats)\n"
               "\n"
               "  -renderingTransform s\n"
               "        name of the CTL rendering transform for this\n"
               "        image (string). This attribute is deprecated.\n"
               "\n"
               "  -lookModTransform s\n"
               "        name of the CTL look modification transform for\n"
               "        this image (string). This attribute is deprecated.\n"
               "\n"
               "  -xDensity f\n"
               "        horizontal output density, in pixels per inch\n"
               "        (float, >= 0.0)\n"
               "\n"
               "  -owner s\n"
               "        name of the owner of the image (string)\n"
               "\n"
               "  -comments s\n"
               "        additional information about the image (string)\n"
               "\n"
               "  -capDate s\n"
               "        date when the image was created or\n"
               "        captured, in local time (string,\n"
               "        formatted as YYYY:MM:DD hh:mm:ss)\n"
               "\n"
               "  -utcOffset f\n"
               "        offset of local time at capDate from UTC, in\n"
               "        seconds (float, UTC == local time + x)\n"
               "\n"
               "  -longitude f\n"
               "  -latitude f\n"
               "  -altitude f\n"
               "        location where the image was recorded, in\n"
               "        degrees east of Greenwich and north of the\n"
               "        equator, and in meters above sea level\n"
               "        (float)\n"
               "\n"
               "  -focus f\n"
               "        the camera's focus distance, in meters\n"
               "        (float, > 0, or \"infinity\")\n"
               "\n"
               "  -expTime f\n"
               "        exposure time, in seconds (float, >= 0)\n"
               "\n"
               "  -aperture f\n"
               "        lens apterture, in f-stops (float, >= 0)\n"
               "\n"
               "  -isoSpeed f\n"
               "        effective speed of the film or image\n"
               "        sensor that was used to record the image\n"
               "        (float, >= 0)\n"
               "\n"
               "  -envmap s\n"
               "        indicates that the image is an environment map\n"
               "        (string, LATLONG or CUBE)\n"
               "\n"
               "  -framesPerSecond i i\n"
               "        playback frame rate expressed as a ratio of two\n"
               "        integers, n and d (the frame rate is n/d frames\n"
               "        per second)\n"
               "\n"
               "  -keyCode i i i i i i i\n"
               "        key code that uniquely identifies a motion\n"
               "        picture film frame using 7 integers:\n"
               "         * film manufacturer code (0 - 99)\n"
               "         * film type code (0 - 99)\n"
               "         * prefix to identify film roll (0 - 999999)\n"
               "         * count, increments once every perfsPerCount\n"
               "           perforations (0 - 9999)\n"
               "         * offset of frame, in perforations from\n"
               "           zero-frame reference mark (0 - 119)\n"
               "         * number of perforations per frame (1 - 15)\n"
               "         * number of perforations per count (20 - 120)\n"
               "\n"
               "  -timeCode i i\n"
               "        SMPTE time and control code, specified as a pair\n"
               "        of 8-digit base-16 integers.  The first number\n"
               "        contains the time address and flags (drop frame,\n"
               "        color frame, field/phase, bgf0, bgf1, bgf2).\n"
               "        The second number contains the user data and\n"
               "        control codes.\n"
               "\n"
               "  -wrapmodes s\n"
               "        if the image is used as a texture map, specifies\n"
               "        how the image should be extrapolated outside the\n"
               "        zero-to-one texture coordinate range\n"
               "        (string, e.g. \"clamp\" or \"periodic,clamp\")\n"
               "\n"
               "  -pixelAspectRatio f\n"
               "        width divided by height of a pixel\n"
               "        (float, >= 0)\n"
               "\n"
               "  -screenWindowWidth f\n"
               "        width of the screen window (float, >= 0)\n"
               "\n"
               "  -screenWindowCenter f f\n"
               "        center of the screen window (2 floats)\n"
               "\n"
               "  -string s s\n"
               "        custom string attribute\n"
               "        (2 strings, attribute name and value)\n"
               "\n"
               "  -float s f\n"
               "        custom float attribute (string + float,\n"
               "        attribute name and value)\n"
               "\n"
               "  -int s i\n"
               "        custom integer attribute (string + integer,\n"
               "        attribute name and value)\n"
               "\n"
               "Other options:\n"
               "\n"
	       "  -erase s      remove attribute with given name\n" 
               "  -h, --help    print this message\n"
               "      --version print version information\n"
               "\n"
               "Report bugs via https://github.com/AcademySoftwareFoundation/openexr/issues or email security@openexr.com\n"
               "";
}

struct SetAttr
{
    string     name;
    int        part;
    Attribute* attr;

    SetAttr (const string& name, int part, Attribute* attr)
        : name (name), part (part), attr (attr)
    {}
};

struct EraseAttr
{
    string name;
    int part;
     EraseAttr (const string& name, int part)
         : name (name), part (part)
     {}
};

typedef vector<SetAttr> SetAttrVector;
typedef vector<EraseAttr> EraseAttrVector;

void
isNonNegative (const char attrName[], float f)
{
    if (f < 0)
    {
        std::stringstream e;
        e << "The value for the " << attrName
          << " attribute must not be less than zero";
        throw invalid_argument (e.str ());
    }
}

void
isPositive (const char attrName[], float f)
{
    if (f <= 0)
    {
        std::stringstream e;
        e << "The value for the " << attrName
          << " attribute must be greater than zero";
        throw invalid_argument (e.str ());
    }
}

void
notValidDate (const char attrName[])
{
    std::stringstream e;
    e << "The value for the " << attrName
      << " attribute is not a valid date of the form \"YYYY:MM:DD yy:mm:ss\"";
    throw invalid_argument (e.str ());
}

int
strToInt (const char str[], int length)
{
    int x = 0;

    for (int i = 0; i < length; ++i)
    {
        if (str[i] < '0' || str[i] > '9') return -1;

        x = x * 10 + (str[i] - '0');
    }

    return x;
}

void
isDate (const char attrName[], const char str[])
{
    //
    // Check that str represents a valid
    // date of the form YYYY:MM:DD hh:mm:ss.
    //

    if (strlen (str) != 19 || str[4] != ':' || str[7] != ':' ||
        str[10] != ' ' || str[13] != ':' || str[16] != ':')
    {
        notValidDate (attrName);
    }

    int Y = strToInt (str + 0, 4);  // year
    int M = strToInt (str + 5, 2);  // month
    int D = strToInt (str + 8, 2);  // day
    int h = strToInt (str + 11, 2); // hour
    int m = strToInt (str + 14, 2); // minute
    int s = strToInt (str + 17, 2); // second

    if (Y < 0 || M < 1 || M > 12 || D < 1 || h < 0 || h > 23 || m < 0 ||
        m > 59 || s < 0 || s > 59)
    {
        notValidDate (attrName);
    }

    if (M == 2)
    {
        bool leapYear = (Y % 4 == 0) && (Y % 100 != 0 || Y % 400 == 0);

        if (D > (leapYear ? 29 : 28)) notValidDate (attrName);
    }
    else if (M == 4 || M == 6 || M == 9 || M == 11)
    {
        if (D > 30) notValidDate (attrName);
    }
    else
    {
        if (D > 31) notValidDate (attrName);
    }
}

void
getFloat (
    const char     attrName[],
    int            argc,
    char**         argv,
    int&           i,
    int            part,
    SetAttrVector& attrs,
    void (*check) (const char attrName[], float f) = 0)
{
    if (i > argc - 2) throw invalid_argument ("Expected a float");

    float f = static_cast<float> (strtod (argv[i + 1], 0));

    if (check) check (attrName, f);

    attrs.push_back (SetAttr (attrName, part, new FloatAttribute (f)));
    i += 2;
}

void
getPosFloatOrInf (
    const char     attrName[],
    int            argc,
    char**         argv,
    int&           i,
    int            part,
    SetAttrVector& attrs)
{
    if (i > argc - 2)
        throw invalid_argument ("Expected a positive float (or \"infinity\")");

    float f;

    if (!strcmp (argv[i + 1], "inf") || !strcmp (argv[i + 1], "infinity"))
    {
        f = float (half::posInf ());
    }
    else
    {
        f = static_cast<float> (strtod (argv[i + 1], 0));

        if (f <= 0)
        {
            std::stringstream e;
            e << "The value for the " << attrName
              << " attribute must be greater than zero, or \"infinity\"";
            throw invalid_argument (e.str ());
        }
    }

    attrs.push_back (SetAttr (attrName, part, new FloatAttribute (f)));
    i += 2;
}

void
getV2f (
    const char     attrName[],
    int            argc,
    char**         argv,
    int&           i,
    int            part,
    SetAttrVector& attrs,
    void (*check) (const char attrName[], const V2f& v) = 0)
{
    if (i > argc - 3) throw invalid_argument ("Expected two floats");

    V2f v (
        static_cast<float> (strtod (argv[i + 1], 0)),
        static_cast<float> (strtod (argv[i + 2], 0)));

    if (check) check (attrName, v);

    attrs.push_back (SetAttr (attrName, part, new V2fAttribute (v)));
    i += 3;
}

void
getRational (
    const char     attrName[],
    int            argc,
    char**         argv,
    int&           i,
    int            part,
    SetAttrVector& attrs,
    void (*check) (const char attrName[], const Rational& r) = 0)
{
    if (i > argc - 3) throw invalid_argument ("Expected a rational");

    Rational r (strtol (argv[i + 1], 0, 0), strtol (argv[i + 2], 0, 0));

    if (check) check (attrName, r);

    attrs.push_back (SetAttr (attrName, part, new RationalAttribute (r)));
    i += 3;
}

void
getString (
    const char     attrName[],
    int            argc,
    char**         argv,
    int&           i,
    int            part,
    SetAttrVector& attrs,
    void (*check) (const char attrName[], const char str[]) = 0)
{
    if (i > argc - 2) throw invalid_argument ("Expected a string");

    const char* str = argv[i + 1];

    if (check) check (attrName, str);

    attrs.push_back (SetAttr (attrName, part, new StringAttribute (str)));
    i += 2;
}

void
getNameAndString (int argc, char** argv, int& i, int part, SetAttrVector& attrs)
{
    if (i > argc - 3) throw invalid_argument ("Expected a name and string");

    const char* attrName = argv[i + 1];
    const char* str      = argv[i + 2];
    attrs.push_back (SetAttr (attrName, part, new StringAttribute (str)));
    i += 3;
}



void
getNameAndFloat (int argc, char** argv, int& i, int part, SetAttrVector& attrs)
{
    if (i > argc - 3) throw invalid_argument ("Expected a name and a float");

    const char* attrName = argv[i + 1];
    float       f        = static_cast<float> (strtod (argv[i + 2], 0));
    attrs.push_back (SetAttr (attrName, part, new FloatAttribute (f)));
    i += 3;
}

void
getNameAndInt (int argc, char** argv, int& i, int part, SetAttrVector& attrs)
{
    if (i > argc - 3) throw invalid_argument ("Expected a name and an integer");

    const char* attrName = argv[i + 1];
    int         j        = strtol (argv[i + 2], 0, 0);
    attrs.push_back (SetAttr (attrName, part, new IntAttribute (j)));
    i += 3;
}

void
getName (int argc, char** argv, int& i, int part, EraseAttrVector& attrs)
{
    if (i > argc - 2) throw invalid_argument ("Expected a name and an integer");

    const char* attrName = argv[i + 1];
    attrs.push_back (EraseAttr(attrName, part));
    i += 2;
}


void
getChromaticities (
    const char     attrName[],
    int            argc,
    char**         argv,
    int&           i,
    int            part,
    SetAttrVector& attrs)
{
    if (i > argc - 9) throw invalid_argument ("Expected 8 chromaticity values");

    ChromaticitiesAttribute* a =
        new ChromaticitiesAttribute (Chromaticities ());
    attrs.push_back (SetAttr (attrName, part, a));

    a->value ().red.x   = static_cast<float> (strtod (argv[i + 1], 0));
    a->value ().red.y   = static_cast<float> (strtod (argv[i + 2], 0));
    a->value ().green.x = static_cast<float> (strtod (argv[i + 3], 0));
    a->value ().green.y = static_cast<float> (strtod (argv[i + 4], 0));
    a->value ().blue.x  = static_cast<float> (strtod (argv[i + 5], 0));
    a->value ().blue.y  = static_cast<float> (strtod (argv[i + 6], 0));
    a->value ().white.x = static_cast<float> (strtod (argv[i + 7], 0));
    a->value ().white.y = static_cast<float> (strtod (argv[i + 8], 0));
    i += 9;
}

void
getEnvmap (
    const char     attrName[],
    int            argc,
    char**         argv,
    int&           i,
    int            part,
    SetAttrVector& attrs)
{
    if (i > argc - 2) throw invalid_argument ("Expected an env map");

    char*  str = argv[i + 1];
    Envmap type;

    if (!strcmp (str, "latlong") || !strcmp (str, "LATLONG"))
    {
        type = ENVMAP_LATLONG;
    }
    else if (!strcmp (str, "cube") || !strcmp (str, "CUBE"))
    {
        type = ENVMAP_CUBE;
    }
    else
    {
        std::stringstream e;
        e << "The value for the " << attrName
          << " attribute must be either LATLONG or CUBE";
        throw invalid_argument (e.str ());
    }

    attrs.push_back (SetAttr (attrName, part, new EnvmapAttribute (type)));
    i += 2;
}

void
getKeyCode (
    const char     attrName[],
    int            argc,
    char**         argv,
    int&           i,
    int            part,
    SetAttrVector& attrs)
{
    if (i > argc - 8) throw invalid_argument ("Expected a key code");

    KeyCodeAttribute* a = new KeyCodeAttribute;
    attrs.push_back (SetAttr (attrName, part, a));

    a->value ().setFilmMfcCode (strtol (argv[i + 1], 0, 0));
    a->value ().setFilmType (strtol (argv[i + 2], 0, 0));
    a->value ().setPrefix (strtol (argv[i + 3], 0, 0));
    a->value ().setCount (strtol (argv[i + 4], 0, 0));
    a->value ().setPerfOffset (strtol (argv[i + 5], 0, 0));
    a->value ().setPerfsPerFrame (strtol (argv[i + 6], 0, 0));
    a->value ().setPerfsPerCount (strtol (argv[i + 7], 0, 0));
    i += 8;
}

void
getTimeCode (
    const char     attrName[],
    int            argc,
    char**         argv,
    int&           i,
    int            part,
    SetAttrVector& attrs)
{
    if (i > argc - 3) throw invalid_argument ("Expected a time code");

    TimeCodeAttribute* a = new TimeCodeAttribute;
    attrs.push_back (SetAttr (attrName, part, a));

    a->value ().setTimeAndFlags (strtoul (argv[i + 1], 0, 16));
    a->value ().setUserData (strtoul (argv[i + 2], 0, 16));
    i += 3;
}

void
getPart (const char attrName[], int argc, char** argv, int& i, int& part)
{
    if (i > argc - 2)
        throw invalid_argument ("Expected a part number (or \"any\")");

    if (!strcmp (argv[i + 1], "any"))
        part = -1;
    else
        part = strtol (argv[i + 1], 0, 0);

    i += 2;
}

int
main (int argc, char** argv)
{
    //
    // Parse the command line.
    //

    if (argc < 2)
    {
        usageMessage (cerr, argv[0], false);
        return 1;
    }

    try
    {
        const char* inFileName  = 0;
        const char* outFileName = 0;

        SetAttrVector attrs;
        EraseAttrVector eraseattrs;
        int           part = -1;
        int           i    = 1;

        while (i < argc)
        {
            const char* attrName = argv[i] + 1;

            if (!strcmp (argv[i], "-part"))
            {
                getPart (attrName, argc, argv, i, part);
            }
            else if (!strcmp (argv[i], "-chromaticities"))
            {
                getChromaticities (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-whiteLuminance"))
            {
                getFloat (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-adoptedNeutral"))
            {
                getV2f (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-renderingTransform"))
            {
                getString (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-lookModTransform"))
            {
                getString (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-xDensity"))
            {
                getFloat (attrName, argc, argv, i, part, attrs, isPositive);
            }
            else if (!strcmp (argv[i], "-owner"))
            {
                getString (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-comments"))
            {
                getString (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-capDate"))
            {
                getString (attrName, argc, argv, i, part, attrs, isDate);
            }
            else if (!strcmp (argv[i], "-utcOffset"))
            {
                getFloat (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-longitude"))
            {
                getFloat (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-latitude"))
            {
                getFloat (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-altitude"))
            {
                getFloat (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-focus"))
            {
                getPosFloatOrInf (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-expTime"))
            {
                getFloat (attrName, argc, argv, i, part, attrs, isPositive);
            }
            else if (!strcmp (argv[i], "-aperture"))
            {
                getFloat (attrName, argc, argv, i, part, attrs, isPositive);
            }
            else if (!strcmp (argv[i], "-isoSpeed"))
            {
                getFloat (attrName, argc, argv, i, part, attrs, isPositive);
            }
            else if (!strcmp (argv[i], "-envmap"))
            {
                getEnvmap (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-framesPerSecond"))
            {
                getRational (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-keyCode"))
            {
                getKeyCode (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-timeCode"))
            {
                getTimeCode (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-wrapmodes"))
            {
                getString (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-pixelAspectRatio"))
            {
                getFloat (attrName, argc, argv, i, part, attrs, isPositive);
            }
            else if (!strcmp (argv[i], "-screenWindowWidth"))
            {
                getFloat (attrName, argc, argv, i, part, attrs, isNonNegative);
            }
            else if (!strcmp (argv[i], "-screenWindowCenter"))
            {
                getV2f (attrName, argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-string"))
            {
                getNameAndString (argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-float"))
            {
                getNameAndFloat (argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-int"))
            {
                getNameAndInt (argc, argv, i, part, attrs);
            }
            else if (!strcmp (argv[i], "-erase"))
            {
                getName ( argc,argv,i,part,eraseattrs);
            }
            else if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
            {
                usageMessage (cout, "exrstdattr", true);
                return 0;
            }
            else if (!strcmp (argv[i], "--version"))
            {
                const char* libraryVersion = getLibraryVersion ();

                cout << "exrstdattr (OpenEXR) " << OPENEXR_VERSION_STRING;
                if (strcmp (libraryVersion, OPENEXR_VERSION_STRING))
                    cout << "(OpenEXR version " << libraryVersion << ")";
                cout << " https://openexr.com" << endl;
                cout << "Copyright (c) Contributors to the OpenEXR Project"
                     << endl;
                cout << "License BSD-3-Clause" << endl;

                return 0;
            }
            else
            {
                if (inFileName == 0)
                    inFileName = argv[i];
                else
                    outFileName = argv[i];

                i += 1;
            }
        }

        if (inFileName == 0) throw invalid_argument ("Missing input filename");
        if (outFileName == 0) throw invalid_argument ("Missing input filename");

        if (!strcmp (inFileName, outFileName))
            throw invalid_argument ("Input and output cannot be the same file");

        //
        // Load the headers from the input file
        // and add attributes to the headers.
        //

        MultiPartInputFile in (inFileName);
        int                numParts = in.parts ();
        vector<Header>     headers;

        //
        // Treat attributes added to a header in its constructor
        // as critical and don't allow them to be deleted.
        // 'name' and 'type' are only required in multipart
        // file and errors will be reported if they
        // are erased
        //
        Header stdHdr;

        for (int part = 0; part < numParts; ++part)
        {
            Header h = in.header (part);

            //
            // process attributes to erase first, so they can be reinserted
            // with a different type
            //
            for (size_t i = 0 ; i < eraseattrs.size() ; ++i)
            {
                const EraseAttr& attr = eraseattrs[i];
                if (attr.part == -1 || attr.part == part)
                {
                    if( stdHdr.find(attr.name)!=stdHdr.end() )
                    {
                        cerr << "Cannot erase attribute " << attr.name
                             << ". "
                             << "It is an essential attribute" << endl;
                        return 1;
                    }
                    h.erase( attr.name );
                }
                else if (attr.part < 0 || attr.part >= numParts)
                {
                    cerr << "Invalid part number " << attr.part
                         << ". "
                            "Part numbers in file "
                         << inFileName
                         << " "
                            "go from 0 to "
                         << numParts - 1 << "." << endl;

                    return 1;
                }
            }


            for (size_t i = 0; i < attrs.size (); ++i)
            {
                const SetAttr& attr = attrs[i];

                if (attr.part == -1 || attr.part == part)
                {
                    h.insert (attr.name, *attr.attr);
                }
                else if (attr.part < 0 || attr.part >= numParts)
                {
                    cerr << "Invalid part number " << attr.part
                         << ". "
                            "Part numbers in file "
                         << inFileName
                         << " "
                            "go from 0 to "
                         << numParts - 1 << "." << endl;

                    return 1;
                }
            }

            headers.push_back (h);
        }

        //
        // Create an output file with the modified headers,
        // and copy the pixels from the input file to the
        // output file.
        //

        MultiPartOutputFile out (outFileName, &headers[0], numParts);

        for (int p = 0; p < numParts; ++p)
        {
            const Header& h    = in.header (p);
            const string& type = h.type ();

            if (type == SCANLINEIMAGE)
            {
                InputPart  inPart (in, p);
                OutputPart outPart (out, p);
                outPart.copyPixels (inPart);
            }
            else if (type == TILEDIMAGE)
            {
                TiledInputPart  inPart (in, p);
                TiledOutputPart outPart (out, p);
                outPart.copyPixels (inPart);
            }
            else if (type == DEEPSCANLINE)
            {
                DeepScanLineInputPart  inPart (in, p);
                DeepScanLineOutputPart outPart (out, p);
                outPart.copyPixels (inPart);
            }
            else if (type == DEEPTILE)
            {
                DeepTiledInputPart  inPart (in, p);
                DeepTiledOutputPart outPart (out, p);
                outPart.copyPixels (inPart);
            }
        }

        for (size_t i = 0; i < attrs.size (); i++)
            delete attrs[i].attr;
    }
    catch (const exception& e)
    {
        cerr << argv[0] << ": " << e.what () << endl;
        return 1;
    }

    return 0;
}
