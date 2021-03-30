// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.

#include <ImfCheckFile.h>
#include <ImathConfig.h>

#include <iostream>
#include <fstream>
#include <string.h>
#if defined _WIN32 || defined _WIN64
# include <io.h>
#else
# include <unistd.h>
#endif
#include <vector>

using namespace OPENEXR_IMF_NAMESPACE;
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::vector;
using std::streampos;

void
usageMessage (const char argv0[])
{
    cerr << "usage: " << argv0 << " [options] imagefile [imagefile ...]\n";
    cerr << "options: \n";
    cerr << "  -m : avoid excessive memory allocation (some files will not be fully checked)\n";
    cerr << "  -t : avoid spending excessive time (some files will not be fully checked)\n";
    cerr << "  -s : use stream API instead of file API\n";
    cerr << "  -v : print OpenEXR and Imath software libary version info\n";

}


bool
exrCheck(const char* filename, bool reduceMemory, bool reduceTime, bool useStream)
{
  if (useStream)
  {
      //
      // open file as stream, check size
      //
      ifstream instream(filename,ifstream::binary);
      instream.seekg(0,instream.end);
      streampos length = instream.tellg();
      instream.seekg(0,instream.beg);

      //
      // read into memory
      //
      vector<char> data(length);
      instream.read( data.data() , length);
      if (instream.bad())
      {
          cerr << "internal error: failed to read file " << filename << endl;

      }
      return checkOpenEXRFile( data.data() , length , reduceMemory , reduceTime);
  }
  else
  {
      return checkOpenEXRFile( filename , reduceMemory , reduceTime);
  }

}

int
main(int argc, char **argv)
{
    if (argc < 2)
    {
        usageMessage (argv[0]);
        return 1;
    }

    bool reduceMemory = false;
    bool reduceTime = false;
    bool badFileFound = false;
    bool useStream = false;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp (argv[i], "-h"))
        {
            usageMessage (argv[0]);
            return 1;
        }
        else if (!strcmp (argv[i], "-m"))
        {
            reduceMemory = true;
        }
        else if (!strcmp (argv[i], "-t"))
        {
            reduceTime = true;
        }
        else if (!strcmp (argv[i],"-s"))
        {
            useStream = true;
        }
        else if (!strcmp (argv[i],"-v"))
        {
            std::cout << OPENEXR_PACKAGE_STRING
                      << " Lib API: " << OPENEXR_LIB_VERSION_STRING
                      << ", " << IMATH_PACKAGE_STRING
#if defined(IMATH_LIB_VERSION_STRING)
                      << " Lib API: " << IMATH_LIB_VERSION_STRING
#endif
                      << std::endl;
            exit(0);
        }
        else
        {

#if defined _WIN32 || defined _WIN64
            if (_access (argv[i], 4) != 0)
#else
            if (access (argv[i], R_OK) != 0)
#endif
            {
               cerr << "No such file: " << argv[i] << endl;
               exit (-1);
            }

            cout << " file " << argv[i] << ' ';
            cout.flush();

            bool hasError = exrCheck(argv[i],reduceMemory,reduceTime,useStream);
            if (hasError)
            {
                cout << "bad\n";
                badFileFound = true;
            }
            else
            {
                cout << "OK\n";
            }
        }
    }

    return badFileFound;

}
