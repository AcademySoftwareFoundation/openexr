
//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//
// output all the idmanifest information found in the file as plain text
//

#include <ImfMisc.h>
#include <ImfMultiPartInputFile.h>
#include <ImfIDManifestAttribute.h>
#include <ImfStandardAttributes.h>

#include <iostream>
#include <vector>
#include <string>

using namespace OPENEXR_IMF_NAMESPACE;

using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::max;
using std::ostream;
using std::set;
using std::string;
using std::to_string;
using std::vector;

size_t
dumpManifest (const IDManifest& mfst)
{

    size_t uncompressedSize = 0;

    for (size_t i = 0; i < mfst.size (); ++i)
    {
        const IDManifest::ChannelGroupManifest& m     = mfst[i];
        bool                                    first = true;
        if (i > 0) { cout << "\n\n"; }
        cout << " channels  : ";
        for (set<string>::const_iterator s = m.getChannels ().begin ();
             s != m.getChannels ().end ();
             ++s)
        {
            if (!first) { cout << ','; }
            else { first = false; }

            cout << *s;
            uncompressedSize += s->size () + 1;
        }

        cout << "\n hashScheme: " << m.getHashScheme () << endl;
        cout << " encoding  : " << m.getEncodingScheme () << endl;
        switch (m.getLifetime ())
        {
            case IDManifest::LIFETIME_FRAME:
                cout << " lifetime  : frame\n";
                break;
            case IDManifest::LIFETIME_SHOT:
                cout << " lifetime  : shot\n";
                break;
            case IDManifest::LIFETIME_STABLE:
                cout << " lifetime  : stable\n";
                break;
        }

        //
        // compute max field sizes
        //
        size_t         maxNumLen = 0;
        vector<size_t> componentLength (m.getComponents ().size ());
        for (size_t c = 0; c < m.getComponents ().size (); ++c)
        {
            size_t componentSize = m.getComponents ()[c].size ();
            uncompressedSize += componentSize + 1;
            componentLength[c] = max (componentLength[c], componentSize);
        }
        for (IDManifest::ChannelGroupManifest::ConstIterator q = m.begin ();
             q != m.end ();
             ++q)
        {

            size_t stringLen = to_string (q.id ()).size ();
            uncompressedSize += stringLen;
            maxNumLen = max (maxNumLen, stringLen);

            for (size_t i = 0; i < q.text ().size (); i++)
            {
                uncompressedSize += q.text ()[i].size () + 1;
                componentLength[i] =
                    max (componentLength[i], q.text ()[i].size ());
            }
        }

        cout << "     " << string (maxNumLen + 1, ' ');
        for (size_t c = 0; c < m.getComponents ().size (); ++c)
        {
            string s = m.getComponents ()[c];
            cout << s << string (componentLength[c] + 1 - s.size (), ' ');
        }
        cout << endl;
        for (IDManifest::ChannelGroupManifest::ConstIterator q = m.begin ();
             q != m.end ();
             ++q)
        {
            string id = to_string (q.id ());
            cout << "     " << id << string (maxNumLen + 1 - id.size (), ' ');
            for (size_t i = 0; i < q.text ().size (); i++)
            {
                string s = q.text ()[i];
                cout << s << string (componentLength[i] + 1 - s.size (), ' ');
            }
            cout << '\n';
        }
    }

    return uncompressedSize;
}

void
printManifest (const char fileName[])
{

    MultiPartInputFile in (fileName);
    //
    // extract objectID attribute
    //

    for (int part = 0; part < in.parts (); part++)
    {
        if (in.parts () > 1) { cout << fileName << " part " << part << ":\n"; }
        if (hasIDManifest (in.header (part)))
        {
            const OPENEXR_IMF_NAMESPACE::CompressedIDManifest& mfst =
                idManifest (in.header (part));
            size_t size = dumpManifest (mfst);
            cout << "raw text size    : " << size << endl;
            cout << "uncompressed size: " << mfst._uncompressedDataSize << endl;
            cout << "compressed size  : " << mfst._compressedDataSize << endl;
        }
        else { cout << "no manifest found\n"; }
    }
}

void
usageMessage (ostream& stream, const char* program_name, bool verbose = false)
{
    stream << "Usage: " << program_name << " imagefile [imagefile ...]\n";

    if (verbose)
        stream
            << "\n"
               "Read exr files and print the contents of the embedded manifest.\n"
               "\n"
               "Options:\n"
               "  -h, --help        print this message\n"
               "      --version     print version information\n"
               "\n"
               "Report bugs via https://github.com/AcademySoftwareFoundation/openexr/issues or email security@openexr.com\n"
               "";
}

int
main (int argc, char* argv[])
{

    if (argc < 2)
    {
        usageMessage (cerr, argv[0], false);
        return -1;
    }

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp (argv[i], "-h") || !strcmp (argv[1], "--help"))
        {
            usageMessage (cout, "exrmanifest", true);
            return 0;
        }
        else if (!strcmp (argv[i], "--version"))
        {
            const char* libraryVersion = getLibraryVersion ();

            cout << "exrmanifest (OpenEXR) " << OPENEXR_VERSION_STRING;
            if (strcmp (libraryVersion, OPENEXR_VERSION_STRING))
                cout << "(OpenEXR version " << libraryVersion << ")";
            cout << " https://openexr.com" << endl;
            cout << "Copyright (c) Contributors to the OpenEXR Project" << endl;
            cout << "License BSD-3-Clause" << endl;
            return 0;
        }
    }

    try
    {
        for (int i = 1; i < argc; ++i)
            printManifest (argv[i]);
    }
    catch (const exception& e)
    {
        cerr << argv[0] << ": " << e.what () << endl;
        return 1;
    }

    for (int i = 1; i < argc; ++i) {}
}
