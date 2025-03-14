
//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "exrmetrics.h"

#include "ImfCompression.h"
#include "ImfMisc.h"
#include "ImfThreading.h"
#include "IlmThreadPool.h"
#include "ImfMultiPartInputFile.h"
#include "ImfVersion.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <vector>

#include <math.h>
#include <stdlib.h>
#include <string.h>

using std::cerr;
using std::cout;
using std::endl;
using std::list;
using std::max;
using std::min;
using std::quoted;
using std::sort;

using std::numeric_limits;
using std::ostream;
using std::vector;
using namespace OPENEXR_IMF_NAMESPACE;
using namespace ILMTHREAD_NAMESPACE;

void
usageMessage (ostream& stream, const char* program_name, bool verbose = false)
{
    stream << "Usage: " << program_name
           << " [options] infile [infile2...] [-o outfile]" << endl;

    if (verbose)
    {
        std::string compressionNames;
        getCompressionNamesString ("/", compressionNames);
        stream
            << "Read an OpenEXR image from infile, write an identical copy to outfile"
               " reporting time taken to read/write and file sizes.\n"
               "\n"
               "Options:\n"
               "\n"
               "  -o file                     file to write to. If no file specified, uses a memory buffer\n"
               "                              note: file may be overwritten multiple times during tests\n"
               "  -p n                        part number to copy, or \"all\" for all parts\n"
               "                              default is \"all\" \n"
               "\n"
               "  -m                          set to multi-threaded (system selected thread count)\n"
               "  -t n                        Use a pool of n worker threads for processing files.\n"
               "                              Default is single threaded (no thread pool)\n"
               "\n"
               "  -l level                    set DWA or ZIP compression level\n"
               "\n"
               "  -z,--compression list       list of compression methods to test\n"
               "                              ("
            << compressionNames.c_str ()
            << ",orig,all\n"
               "                              default orig: retains original method)\n"

               "  --convert                   shorthand options for writing a new file with no metrics:\n"
               "                              -p all --type orig --time none --type orig --no-size --passes 1\n"
               "                              change pixel type or compression by specifying --type or -z after --convert\n"
               "  --bench                     shorthand options for robust performance benchmarking:\n"
               "                              -p all --compression all --time write,reread --passes 10 --type half,float --no-size --csv\n"
               "\n"
               "  -16 rgba|all                [DEPRECATED] force 16 bit half float: either just RGBA, or all channels\n"
               "                              Use --type half or --type mixed instead\n"
               " --pixelmode list             list of pixel types to use (float,half,mixed,orig)\n"
               "                              mixed uses half for RGBA, float for others. Default is 'orig'\n"
               " --time list                  comma separated list of operations to report timing for.\n"
               "                              operations can be any of read,write,reread (use --time none for no timing)\n"
               " --no-size                    don't output size data\n"
               " --json                       print output as JSON dictionary (Default mode)\n"
               " --csv                        print output in csv mode. If passes>1, show median timing\n"
               "                              default is JSON mode\n"
               " --passes num                 write and re-read file num times (default 1)\n"
               "\n"
               "  -h, --help                  print this message\n"
               "  -v                          output progress messages\n"
               "\n"
               "  --version                   print version information\n"
               "\n";
    }
}

struct options
{
    enum
    {
        TIME_NONE   = 0,
        TIME_READ   = 1,
        TIME_WRITE  = 2,
        TIME_REREAD = 4
    };

    const char*              outFile = nullptr;
    std::vector<const char*> inFiles;
    int                      part    = -1;
    int                      threads = 0;
    float                    level   = INFINITY;
    int                      passes  = 1;
    int                      timing  = TIME_READ | TIME_REREAD | TIME_WRITE;
    bool                     outputSizeData = true;
    bool                     verbose        = false;
    bool                     csv            = false;
    std::vector<PixelMode>   pixelModes;
    std::vector<OPENEXR_IMF_NAMESPACE::Compression> compressions;

    int parse (int argc, char* argv[]);
};

struct runData
{
    const char* file;
    PixelMode   mode;
    Compression compression;
    fileMetrics metrics;
};

double
median (const vector<double>& perf)
{
    if (perf.size () == 1) { return perf[0]; }
    vector<double> d = perf;
    sort (d.begin (), d.end ());
    return (d[d.size () / 2] + d[(d.size () - 1) / 2]) / 2.0;
}

void
printTiming (const vector<double>& perf, ostream& out, bool raw, bool stats)
{

    if (perf.size () == 0) { return; }

    if (perf.size () == 1)
    {
        out << perf[0];
        return;
    }

    out << '{';
    if (raw) { out << "\"values\": [ "; }
    double n       = perf.size ();
    double k       = perf[0];
    double sum     = 0.;
    double x       = 0.;
    double x2      = 0.;
    double minimum = numeric_limits<double>::max ();
    double maximum = -minimum;

    bool first = true;
    for (double i: perf)
    {

        if (raw)
        {
            if (!first) { out << " , "; }
            first = false;
            out << i;
        }
        sum += i;
        x += i - k;
        x2 += (i - k) * (i - k);
        maximum = max (maximum, i);
        minimum = min (minimum, i);
    }
    if (raw) { out << " ] , "; }
    if (stats)
    {
        out << "\"min\": " << minimum << ", \"max\": " << maximum
            << ", \"mean\": " << sum / n << ", \"median\": " << median (perf)
            << ", \"std dev\": ";
        out << sqrt ((x2 - x * x / n) / n);
    }
    out << "}";
}

void
printPartStats (
    ostream&         out,
    const partStats& data,
    const string     indent,
    int              timing,
    bool             raw,
    bool             stats)
{
    bool output = false;
    if (timing & options::TIME_READ)
    {
        if (data.sizeData.isDeep)
        {
            out << indent << "\"count read time\": ";
            printTiming (data.countReadPerf, out, raw, stats);
            output = true;
        }
        if (output) { out << ",\n"; }
        out << indent << "\"read time\": ";
        printTiming (data.readPerf, out, raw, stats);
        output = true;
    }

    if (timing & options::TIME_WRITE)
    {
        if (output) { out << ",\n"; }
        output = true;
        out << indent << "\"write time\": ";
        printTiming (data.writePerf, out, raw, stats);
    }

    if (timing & options::TIME_REREAD)
    {
        if (data.sizeData.isDeep)
        {
            if (output) { out << ",\n"; }
            output = true;
            out << indent << "\"count re-read time\": ";
            printTiming (data.countRereadPerf, out, raw, stats);
        }
        if (output) { out << ",\n"; }
        output = true;
        out << indent << "\"re-read time\": ";
        printTiming (data.rereadPerf, out, raw, stats);
    }
}

void
jsonStats (
    ostream&       out,
    list<runData>& data,
    bool           outputSizeData,
    int            timing,
    bool           raw,
    bool           stats)
{

    static const char* lastFileName = nullptr;
    out << '[' << endl;
    bool firstEntryForFile = true;
    for (runData run: data)
    {
        if (run.file != lastFileName)
        {
            if (lastFileName != nullptr)
            {
                out << "\n   ]\n";
                out << " },\n";
            }
            out << " {\n"
                << "  \"file\":" << quoted (run.file) << ",\n";
            lastFileName = run.file;
            if (outputSizeData)
            {
                out << "  \"input file size\": " << run.metrics.inputFileSize
                    << ",\n";
                out << "  \"pixels\": "
                    << run.metrics.totalStats.sizeData.pixelCount << ",\n";
                out << "  \"channels\": "
                    << run.metrics.totalStats.sizeData.channelCount << ",\n";
                out << "  \"total raw size\": "
                    << run.metrics.totalStats.sizeData.rawSize << ",\n";
                string compName;
                if (run.metrics.totalStats.sizeData.compression ==
                    NUM_COMPRESSION_METHODS)
                {
                    compName = "mixed";
                }
                else
                {
                    getCompressionNameFromId (
                        run.metrics.totalStats.sizeData.compression, compName);
                }
                out << "  \"compression\": \"" << compName << "\",\n";

                out << "  \"part type\": \"";
                if (run.metrics.totalStats.sizeData.partType == "")
                {
                    out << "mixed";
                }
                else { out << run.metrics.totalStats.sizeData.partType; }
                out << "\",\n";

                if (run.metrics.totalStats.sizeData.isTiled)
                {
                    out << "  \"tile count\": "
                        << run.metrics.totalStats.sizeData.tileCount << ",\n";
                }

                if (run.metrics.stats.size () > 1)
                {
                    out << "  \"parts\":\n";
                    out << "   [\n";

                    for (size_t part = 0; part < run.metrics.stats.size ();
                         ++part)
                    {
                        out << "    {\n";
                        out << "      \"part\": " << part << ",\n";
                        out << "      \"pixels\": "
                            << run.metrics.stats[part].sizeData.pixelCount
                            << ",\n";
                        out << "      \"channels\": "
                            << run.metrics.stats[part].sizeData.channelCount
                            << ",\n";
                        string compName;
                        getCompressionNameFromId (
                            run.metrics.stats[part].sizeData.compression,
                            compName);
                        out << "      \"compression\": \"" << compName
                            << "\",\n";
                        out << "      \"part type\": \""
                            << run.metrics.stats[part].sizeData.partType
                            << "\",\n";
                        if (run.metrics.stats[part].sizeData.isTiled)
                        {
                            out << "      \"tile count\": "
                                << run.metrics.stats[part].sizeData.tileCount
                                << ",\n";
                        }
                        out << "      \"total raw size\": "
                            << run.metrics.stats[part].sizeData.rawSize << "\n";
                        out << "    }";
                        if (part < run.metrics.stats.size () - 1)
                        {
                            out << ',';
                        }
                        out << "\n";
                    }
                    out << "   ],\n";
                }
            }
            out << "  \"metrics\":\n";
            out << "   [";
            firstEntryForFile = true;
        }

        string compName;
        if (run.compression == NUM_COMPRESSION_METHODS)
        {
            compName = "original";
        }
        else { getCompressionNameFromId (run.compression, compName); }

        if (!firstEntryForFile) { out << ','; }
        out << '\n';
        out << "    {\n";
        out << "      \"compression\": \"" << compName << "\",\n";
        out << "      \"pixel mode\": \"" << modeName (run.mode) << "\"";

        if (outputSizeData)
        {
            out << ",\n";
            out << "      \"output size\": " << run.metrics.outputFileSize;
        }
        if (timing)
        {
            out << ",\n";
            printPartStats (
                out, run.metrics.totalStats, "      ", timing, raw, stats);
        }
        if (timing && run.metrics.stats.size () > 1)
        {
            out << ",\n";
            out << "      \"parts\":\n";
            out << "       [\n";
            //first print total statistics, then print all part data, unless there's only one part
            for (size_t part = 0; part < run.metrics.stats.size (); ++part)
            {
                out << "        {\n";
                out << "          \"part\": " << part << ",\n";

                printPartStats (
                    out,
                    run.metrics.stats[part],
                    "          ",
                    timing,
                    raw,
                    stats);
                out << "\n        }";
                if (part < run.metrics.stats.size () - 1) { out << ','; }
                out << endl;
            }
            out << "       ]\n";
        }
        else { out << "\n"; }
        out << "    }";
        firstEntryForFile = false;
    }

    if (lastFileName)
    {
        out << "\n   ]\n";
        out << " }\n";
    }
    out << "]\n";
}

void
csvStats (ostream& out, list<runData>& data, bool outputSizeData, int timing)
{
    out << "file name";
    if (outputSizeData)
    {
        out << ",input size,pixel count,channel count,tile count,raw size";
    }
    out << ",compression,pixel mode";
    if (outputSizeData) { out << ",output size"; }
    if (timing & options::TIME_READ)
    {
        out << ",count read time";
        out << ",read time";
    }
    if (timing & options::TIME_WRITE) { out << ",write time"; }
    if (timing & options::TIME_REREAD)
    {
        out << ",count reread time";
        out << ",reread time";
    }
    cout << "\n";
    for (runData run: data)
    {
        out << run.file;
        if (outputSizeData)
        {
            out << ',' << run.metrics.inputFileSize << ','
                << run.metrics.totalStats.sizeData.pixelCount << ','
                << run.metrics.totalStats.sizeData.channelCount;
            if (run.metrics.totalStats.sizeData.isTiled)
            {
                out << ',' << run.metrics.totalStats.sizeData.tileCount;
            }
            else { out << ",---"; }
            out << ',' << run.metrics.totalStats.sizeData.rawSize;
        }
        string compName;
        if (run.compression == NUM_COMPRESSION_METHODS)
        {
            compName = "original";
        }
        else { getCompressionNameFromId (run.compression, compName); }
        out << ',' << compName << ',' << modeName (run.mode);

        if (outputSizeData) { out << ',' << run.metrics.outputFileSize; }
        if (timing & options::TIME_READ)
        {
            if (run.metrics.totalStats.sizeData.isDeep)
            {
                out << ',' << median (run.metrics.totalStats.countReadPerf);
            }
            else { out << ",---"; }
            out << ',' << median (run.metrics.totalStats.readPerf);
        }
        if (timing & options::TIME_WRITE)
        {
            out << ',' << median (run.metrics.totalStats.writePerf);
        }
        if (timing & options::TIME_REREAD)
        {
            if (run.metrics.totalStats.sizeData.isDeep)
            {
                out << ',' << median (run.metrics.totalStats.countRereadPerf);
            }
            else { out << ",---"; }
            out << ',' << median (run.metrics.totalStats.rereadPerf);
        }
        out << "\n";
    }
}

int
main (int argc, char** argv)
{

    options opts;

    if (opts.parse (argc, argv)) { return 1; }

    list<runData> data;
    try
    {
        if (opts.threads < 0)
            setGlobalThreadCount (ThreadPool::estimateThreadCountForFileIO ());
        else
            setGlobalThreadCount (opts.threads);

        for (const char* inFile: opts.inFiles)
        {
            bool hasDeep = false;

            //
            // unless using original compression method, check whether file is deep
            // to skip incompatible compression methods
            //
            if (opts.compressions.size () > 1 &&
                opts.compressions[0] != NUM_COMPRESSION_METHODS)
            {
                MultiPartInputFile in (inFile);
                hasDeep = isNonImage (in.version ());
            }
            for (Compression compression: opts.compressions)
            {
                if (!hasDeep || compression == NUM_COMPRESSION_METHODS ||
                    isValidDeepCompression (compression))
                {
                    for (PixelMode mode: opts.pixelModes)
                    {
                        runData d;
                        d.file        = inFile;
                        d.compression = compression;
                        d.mode        = mode;
                        d.metrics     = exrmetrics (
                            inFile,
                            opts.outFile,
                            opts.part,
                            compression,
                            opts.level,
                            opts.passes,
                            opts.outFile || opts.outputSizeData ||
                                opts.timing & options::TIME_WRITE,
                            opts.timing & options::TIME_REREAD,
                            mode,
                            opts.verbose);
                        data.push_back (d);
                    }
                }
            }
        }
    }
    catch (std::exception& what)
    {
        cerr << "error from exrmetrics: " << what.what () << endl;
        return 1;
    }

    if (opts.timing || opts.outputSizeData)
    {

        if (opts.csv)
        {
            csvStats (cout, data, opts.outputSizeData, opts.timing);
        }
        else
        {
            jsonStats (
                cout, data, opts.outputSizeData, opts.timing, true, true);
        }
    }

    return 0;
}

std::list<std::string>
split (const char* str, char splitChar)
{
    std::istringstream     stream (str);
    std::list<std::string> items;
    std::string            token;
    while (std::getline (stream, token, splitChar))
    {
        items.push_back (token);
    }
    return items;
}

int
options::parse (int argc, char* argv[])
{

    int i = 1;

    timing = TIME_READ | TIME_WRITE | TIME_REREAD;

    if (argc == 1)
    {
        usageMessage (cerr, "exrmetrics", true);
        return 1;
    }

    while (i < argc)
    {
        if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
        {
            usageMessage (cout, "exrmetrics", true);
            return 0;
        }

        else if (!strcmp (argv[i], "--version"))
        {
            const char* libraryVersion = getLibraryVersion ();

            cout << "exrmetrics (OpenEXR) " << OPENEXR_VERSION_STRING;
            if (strcmp (libraryVersion, OPENEXR_VERSION_STRING))
                cout << "(OpenEXR version " << libraryVersion << ")";
            cout << " https://openexr.com" << endl;
            cout << "Copyright (c) Contributors to the OpenEXR Project" << endl;
            cout << "License BSD-3-Clause" << endl;
            return 0;
        }
        else if (!strcmp (argv[i], "-m"))
        {
            threads = -1;
            i += 1;
        }
        else if (!strcmp (argv[i], "-t"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing thread count value with -t option\n";
                return 1;
            }

            threads = atoi (argv[i + 1]);
            if (threads < 0)
            {
                cerr << "bad thread count " << argv[i + 1]
                     << " specified to -t option\n";
                return 1;
            }

            i += 2;
        }
        else if (!strcmp (argv[i], "--bench"))
        {
            compressions.clear ();
            for (int c = 0; c < NUM_COMPRESSION_METHODS; ++c)
            {
                compressions.push_back (Compression (c));
            }
            passes         = 10;
            outputSizeData = false;
            csv            = true;
            part           = -1;
            timing         = TIME_WRITE | TIME_REREAD;
            pixelModes.resize (2);
            pixelModes[0] = PIXELMODE_ALL_HALF;
            pixelModes[1] = PIXELMODE_ALL_FLOAT;
            i += 1;
        }
        else if (!strcmp (argv[i], "--convert"))
        {
            pixelModes.resize (1);
            pixelModes[0]  = PIXELMODE_ORIGINAL;
            passes         = 1;
            outputSizeData = false;
            timing         = 0;
            part           = -1;
            i += 1;
        }
        else if (!strcmp (argv[i], "--passes"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing pass count value with --passes option\n";
                return 1;
            }
            passes = atoi (argv[i + 1]);
            if (passes < 0)
            {
                cerr << "bad value for passes " << argv[i + 1]
                     << " specified to --passes option\n";
                return 1;
            }
            i += 2;
        }
        else if (!strcmp (argv[i], "-v"))
        {
            verbose = true;
            i += 1;
        }
        else if (!strcmp (argv[i], "--csv"))
        {
            csv = true;
            i += 1;
        }
        else if (!strcmp (argv[i], "--json"))
        {
            csv = false;
            i += 1;
        }
        else if (!strcmp (argv[i], "-z") || !strcmp (argv[i], "--compression"))
        {
            compressions.clear ();
            if (i > argc - 2)
            {
                cerr << "Missing compression value with " << argv[i]
                     << " option\n";
                return 1;
            }
            std::list<string> items = split (argv[i + 1], ',');

            for (string i: items)
            {

                if (i == "orig")
                {
                    compressions.push_back (NUM_COMPRESSION_METHODS);
                }
                else if (i == "all")
                {
                    for (int c = 0; c < NUM_COMPRESSION_METHODS; ++c)
                    {
                        compressions.push_back (Compression (c));
                    }
                }
                else
                {
                    Compression compression;
                    getCompressionIdFromName (i, compression);
                    if (compression == Compression::NUM_COMPRESSION_METHODS)
                    {
                        cerr << "unknown compression type " << i << endl;
                        return 1;
                    }
                    compressions.push_back (compression);
                }
            }
            i += 2;
        }
        else if (!strcmp (argv[i], "-p"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing part number with -p option\n";
                return 1;
            }
            if (!strcmp (argv[i + 1], "all")) { part = -1; }
            else
            {
                part = atoi (argv[i + 1]);
                if (part < -1)
                {
                    cerr << "bad part " << part << " specified to -p option\n";
                    return 1;
                }
            }

            i += 2;
        }
        else if (!strcmp (argv[i], "-l"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing compression level number with -l option\n";
                return 1;
            }
            level = atof (argv[i + 1]);
            if (level < 0)
            {
                cerr << "bad level " << level << " specified to -l option\n";
                return 1;
            }

            i += 2;
        }
        else if (!strcmp (argv[i], "-o"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing filename specified with -o\n";
                return 1;
            }
            if (outFile)
            {
                cerr << "-o output filename can only be specified once\n";
                return 1;
            }
            outFile = argv[i + 1];
            i += 2;
        }
        // deprecated flag for backwards compatibility
        else if (!strcmp (argv[i], "-16"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing mode with -16 option\n";
                return 1;
            }
            if (!strcmp (argv[i + 1], "all"))
            {
                pixelModes.push_back (PIXELMODE_ALL_HALF);
            }
            else if (!strcmp (argv[i + 1], "rgba"))
            {
                pixelModes.push_back (PIXELMODE_MIXED_HALF_FLOAT);
            }
            else
            {
                cerr << " bad mode for -16 option: must be 'all' or 'rgba'\n";
                return 1;
            }
            i += 2;
        }
        else if (!strcmp (argv[i], "--pixelmode"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing type list with  with --pixelmode option\n";
                return 1;
            }
            std::list<string> items = split (argv[i + 1], ',');
            pixelModes.clear ();
            for (string i: items)
            {
                if (i == "half") { pixelModes.push_back (PIXELMODE_ALL_HALF); }
                else if (i == "float")
                {
                    pixelModes.push_back (PIXELMODE_ALL_FLOAT);
                }
                else if (i == "rgba" || i == "mixed")
                {
                    pixelModes.push_back (PIXELMODE_MIXED_HALF_FLOAT);
                }
                else if (i == "orig")
                {
                    pixelModes.push_back (PIXELMODE_ORIGINAL);
                }
                else
                {
                    cerr
                        << "bad pixel type " << i
                        << " for --pixelmode: must be half,float,rgba,mixed or orig\n";
                    return 1;
                }
            }
            i += 2;
        }
        else if (!strcmp (argv[i], "--time"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing value list with --time option\n";
                return 1;
            }

            timing = TIME_NONE;
            if (strcmp (argv[i + 1], "none"))
            {
                std::list<string> items = split (argv[i + 1], ',');
                for (string i: items)
                {
                    if (i == "read") { timing |= TIME_READ; }
                    else if (i == "reread") { timing |= TIME_REREAD; }
                    else if (i == "write") { timing |= TIME_WRITE; }
                    else
                    {
                        cerr
                            << "bad value in timing list. Options are read,write,reread\n";
                        return 1;
                    }
                }
            }
            i += 2;
        }
        else if (!strcmp (argv[i], "--no-size"))
        {
            outputSizeData = false;
            i += 1;
        }
        else if (!strcmp (argv[i], "-i"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing filename with -i option\n";
                return 1;
            }
            inFiles.push_back (argv[i + 1]);
            i += 2;
        }
        else
        {
            inFiles.push_back (argv[i]);
            i += 1;
        }
    }
    if (inFiles.size () == 0)
    {
        cerr << "Missing input file\n";
        usageMessage (cerr, "exrmetrics", false);
        return 1;
    }

    if (!outputSizeData && !timing && !outFile)
    {
        cerr
            << "Nothing to do: no output file specified, and all performance/size data disabled";
        cerr << "Use -o to specify output image filename\n";
        return 1;
    }

    // default options if none specified
    if (pixelModes.size () == 0) { pixelModes.push_back (PIXELMODE_ORIGINAL); }

    if (compressions.size () == 0)
    {
        compressions.push_back (NUM_COMPRESSION_METHODS);
    }

    return 0;
}
