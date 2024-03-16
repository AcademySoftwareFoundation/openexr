//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//
// example of writing a file with DeepIDs.
// Creates a deep file with a series of circles and 'blobs' (2D Guassian-like objects)
// of different colors and sizes, each with two or three ID channels, and an idmanifest
// to store the respective names.
//
//

#include <ImfChannelList.h>
#include <ImfDeepScanLineOutputFile.h>
#include <ImfDeepFrameBuffer.h>
#include <ImfIDManifest.h>
#include <ImfStandardAttributes.h>

#include <string.h>

#include <algorithm>
#include <iostream>
#include <random>
#include <set>
#include <vector>

using namespace OPENEXR_IMF_NAMESPACE;

using std::cerr;
using std::set;
using std::sort;
using std::vector;

void
printHelp ()
{
    cerr << "syntax: deepidexample options output.deep.exr\n\n";
    cerr
        << "--multivariate      : combine 'material' and 'model' name into a single ID channel, rather than separate channels\n";
    cerr
        << "--64                : use 64 bit hashes in two channels, rather than a single channel 32 bit hash\n";
    cerr
        << "--frame  number     : specify animation frame number. Animation cycles every 200 frames\n";
    cerr
        << "--objectid          : store object ids in a simple stringvector format rather than the idmanifest attribute\n";
    cerr
        << "--size width height : specify image dimensions for output (default 256 256)\n";
    cerr << "--count number      : number of objects to write (default 100)\n";
}

//
// a 'sample' as written to the deep file, with RGBA, depth, and up to five ID channels
//
struct Rgbaz
{
    half     r, g, b, a, z;
    uint32_t id0, id1, id2, id3, id4;

    //
    // depth sort (use ID to resolve ties)
    //
    bool operator< (const Rgbaz& other) const
    {
        if (z < other.z) { return true; }
        if (z > other.z) { return false; }
        return id4 < other.id4;
    }
};

//
// object model names
//
static const char* shapeNames[] = {"blob", "circle"};
static const char* sizeNames[]  = {"small", "medium", "big"};

//
// seven colors, names and RGB values of each
//
static const char* colorNames[] = {
    "white", "red", "green", "blue", "cyan", "magenta", "yellow"};
struct Rgbaz colors[] = {
    {0.9, 0.9, 0.9},
    {0.9, 0.1, 0.1},
    {0.1, 0.9, 0.1},
    {0.1, 0.1, 0.9},
    {0.1, 0.9, 0.9},
    {0.9, 0.1, 0.9},
    {0.9, 0.9, 0.1}};

int random (std::default_random_engine& generator, int max);

void drawBlob (
    std::vector<std::vector<Rgbaz>>& image,
    int                              width,
    int                              height,
    float                            x,
    float                            y,
    float                            z,
    int                              size,
    int                              color,
    const uint32_t*                  ids);
void drawCircle (
    std::vector<std::vector<Rgbaz>>& image,
    int                              width,
    int                              height,
    float                            x,
    float                            y,
    float                            z,
    int                              size,
    int                              color,
    const uint32_t*                  ids);

int
main (int argc, char* argv[])
{
    char* outputFile   = nullptr;
    bool  hash64       = false;
    bool  multivariate = false;
    bool  objectID     = false;
    int   width        = 256;
    int   height       = 256;
    int   count        = 100;
    int   frame        = 0;

    // parse options
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp (argv[i], "--64", 3) == 0) { hash64 = true; }
        else if (strncmp (argv[i], "--multivariate", 3) == 0)
        {
            multivariate = true;
        }
        else if (strncmp (argv[i], "--help", 3) == 0)
        {
            printHelp ();
            return 0;
        }
        else if (strncmp (argv[i], "--size", 3) == 0)
        {
            if (argc < i + 2)
            {
                printHelp ();
                return 1;
            }
            width  = atoi (argv[i + 1]);
            height = atoi (argv[i + 2]);
            i += 2;
        }
        else if (strncmp (argv[i], "--count", 3) == 0)
        {
            if (argc < i + 1)
            {
                printHelp ();
                return 1;
            }
            count = atoi (argv[i + 1]);
            i += 1;
        }
        else if (strncmp (argv[i], "--frame", 3) == 0)
        {
            if (argc < i + 1)
            {
                printHelp ();
                return 1;
            }
            frame = atoi (argv[i + 1]);
            i += 1;
        }
        else if (strncmp (argv[i], "--objectid", 3) == 0) { objectID = true; }
        else if (outputFile == nullptr) { outputFile = argv[i]; }
        else
        {
            printHelp ();
            return 1;
        }
    }

    if (outputFile == nullptr)
    {
        cerr << "error: need to specify output filename\n";
        printHelp ();
        return 1;
    }

    if (objectID)
    {
        if (!multivariate || hash64)
        {
            cerr
                << "error: --objectid mode only works with --multivariate on and --64 off\n";
            return 1;
        }
    }

    //
    // initialize manifest object
    //

    IDManifest::ChannelGroupManifest modelOrMultiManifest;
    IDManifest::ChannelGroupManifest materialManifest;

    std::vector<std::string> oldObjectID;

    if (multivariate)
    {
        if (hash64)
        {
            set<string> ids;
            ids.insert ("id0");
            ids.insert ("id1");
            modelOrMultiManifest.setChannels (ids);
            modelOrMultiManifest.setEncodingScheme (IDManifest::ID2_SCHEME);
            modelOrMultiManifest.setHashScheme (IDManifest::MURMURHASH3_64);
        }
        else
        {
            modelOrMultiManifest.setChannel ("id");
            modelOrMultiManifest.setEncodingScheme (IDManifest::ID_SCHEME);
            modelOrMultiManifest.setHashScheme (IDManifest::MURMURHASH3_32);
        }
        vector<string> components (2);
        components[0] = "model";
        components[1] = "material";
        modelOrMultiManifest.setComponents (components);
    }
    else
    {
        if (hash64)
        {
            set<string> model;
            model.insert ("model.id0");
            model.insert ("model.id1");
            modelOrMultiManifest.setChannels (model);
            modelOrMultiManifest.setEncodingScheme (IDManifest::ID2_SCHEME);
            modelOrMultiManifest.setHashScheme (IDManifest::MURMURHASH3_64);

            set<string> material;
            material.insert ("material.id0");
            material.insert ("material.id1");
            materialManifest.setChannels (material);
            materialManifest.setEncodingScheme (IDManifest::ID2_SCHEME);
            materialManifest.setHashScheme (IDManifest::MURMURHASH3_64);
        }
        else
        {
            modelOrMultiManifest.setChannel ("modelid");
            materialManifest.setChannel ("materialid");

            modelOrMultiManifest.setEncodingScheme (IDManifest::ID_SCHEME);
            modelOrMultiManifest.setHashScheme (IDManifest::MURMURHASH3_32);
            materialManifest.setEncodingScheme (IDManifest::ID_SCHEME);
            materialManifest.setHashScheme (IDManifest::MURMURHASH3_32);
        }
        modelOrMultiManifest.setComponent ("model");
        materialManifest.setComponent ("material");
    }

    modelOrMultiManifest.setLifetime (IDManifest::LIFETIME_STABLE);
    materialManifest.setLifetime (IDManifest::LIFETIME_STABLE);

    //
    // draw image
    //

    vector<vector<Rgbaz>> deepImage (width * height);

    std::default_random_engine generator;
    generator.seed (2);

    uint32_t ids[5];

    //
    // animation oscillates between two random positions
    // over 100 frames
    //
    float blend = 0.5 - cos (double (frame) * M_PI / 100.) / 2;
    for (int object = 0; object < count; ++object)
    {
        int shape = random (generator, 1);
        int size  = random (generator, 2);
        int color = random (generator, 6);

        //
        // generate ID
        //
        if (multivariate)
        {
            vector<string> s (2);
            s[0] = string (shapeNames[shape]) + "/" + string (sizeNames[size]);
            s[1] = colorNames[color];
            uint64_t hash = modelOrMultiManifest.insert (s);
            ids[0]        = hash & 0xFFFFFFFF;

            // only needed for 64 bit hash scheme: store most significant 32 bits in ids[1]
            ids[1] = hash >> 32;

            if (objectID)
            {
                oldObjectID.push_back (
                    s[0] + "," + s[1] + "," + std::to_string (ids[0]));
            }
        }
        else
        {
            uint64_t hash = modelOrMultiManifest.insert (
                string (shapeNames[shape]) + "/" + string (sizeNames[size]));
            ids[0] = hash & 0xFFFFFFFF;
            ids[1] = hash >> 32;
            hash   = materialManifest.insert (colorNames[color]);
            ids[2] = hash & 0xFFFFFFFF;
            ids[3] = hash >> 32;
        }

        ids[4] = object; // particle ID

        //
        // randomized position, velocity, depth
        //
        int   x1 = random (generator, width);
        int   y1 = random (generator, height);
        int   x2 = random (generator, width);
        int   y2 = random (generator, height);
        float z  = random (generator, 4096) / 2.f;

        float x = blend * x2 + (1.0 - blend) * x1;
        float y = blend * y2 + (1.0 - blend) * y1;

        if (shape == 0)
        {
            drawBlob (deepImage, width, height, x, y, z, size, color, ids);
        }
        else
        {
            drawCircle (deepImage, width, height, x, y, z, size, color, ids);
        }
    }

    //
    // initialize pointers required by OpenEXR API to indicate address of first sample of each channel of each pixel
    //

    vector<int>   sampleCounts (width * height);
    vector<char*> ptrR (width * height);
    vector<char*> ptrG (width * height);
    vector<char*> ptrB (width * height);
    vector<char*> ptrA (width * height);
    vector<char*> ptrZ (width * height);
    vector<char*> ptrID0 (width * height);
    vector<char*> ptrID1 (width * height);
    vector<char*> ptrID2 (width * height);
    vector<char*> ptrID3 (width * height);
    vector<char*> ptrID4 (width * height);
    for (int i = 0; i < width * height; ++i)
    {
        sampleCounts[i] = int (deepImage[i].size ());
        if (sampleCounts[i] > 0)
        {
            //
            // store samples depth sorted
            //
            sort (deepImage[i].begin (), deepImage[i].end ());
            ptrR[i]   = (char*) &deepImage[i][0].r;
            ptrG[i]   = (char*) &deepImage[i][0].g;
            ptrB[i]   = (char*) &deepImage[i][0].b;
            ptrA[i]   = (char*) &deepImage[i][0].a;
            ptrZ[i]   = (char*) &deepImage[i][0].z;
            ptrID0[i] = (char*) &deepImage[i][0].id0;
            ptrID1[i] = (char*) &deepImage[i][0].id1;
            ptrID2[i] = (char*) &deepImage[i][0].id2;
            ptrID3[i] = (char*) &deepImage[i][0].id3;
            ptrID4[i] = (char*) &deepImage[i][0].id4;
        }
    }

    // save file

    Header h (width, height);
    h.compression () = ZIPS_COMPRESSION;

    DeepFrameBuffer buf;
    buf.insertSampleCountSlice (Slice (
        UINT,
        (char*) sampleCounts.data (),
        sizeof (int),
        sizeof (int) * width));

    h.channels ().insert ("R", Channel (HALF));
    buf.insert (
        "R",
        DeepSlice (
            HALF,
            (char*) ptrR.data (),
            sizeof (char*),
            sizeof (char*) * width,
            sizeof (Rgbaz)));

    h.channels ().insert ("G", Channel (HALF));
    buf.insert (
        "G",
        DeepSlice (
            HALF,
            (char*) ptrG.data (),
            sizeof (char*),
            sizeof (char*) * width,
            sizeof (Rgbaz)));

    h.channels ().insert ("B", Channel (HALF));
    buf.insert (
        "B",
        DeepSlice (
            HALF,
            (char*) ptrB.data (),
            sizeof (char*),
            sizeof (char*) * width,
            sizeof (Rgbaz)));

    h.channels ().insert ("A", Channel (HALF));
    buf.insert (
        "A",
        DeepSlice (
            HALF,
            (char*) ptrA.data (),
            sizeof (char*),
            sizeof (char*) * width,
            sizeof (Rgbaz)));

    h.channels ().insert ("Z", Channel (HALF));
    buf.insert (
        "Z",
        DeepSlice (
            HALF,
            (char*) ptrZ.data (),
            sizeof (char*),
            sizeof (char*) * width,
            sizeof (Rgbaz)));

    if (multivariate)
    {
        if (hash64)
        {
            h.channels ().insert ("id.id0", UINT);
            buf.insert (
                "id.id0",
                DeepSlice (
                    UINT,
                    (char*) ptrID0.data (),
                    sizeof (char*),
                    sizeof (char*) * width,
                    sizeof (Rgbaz)));
            h.channels ().insert ("id.id1", UINT);
            buf.insert (
                "id.id1",
                DeepSlice (
                    UINT,
                    (char*) ptrID1.data (),
                    sizeof (char*),
                    sizeof (char*) * width,
                    sizeof (Rgbaz)));
        }
        else
        {
            h.channels ().insert ("id", UINT);
            buf.insert (
                "id",
                DeepSlice (
                    UINT,
                    (char*) ptrID0.data (),
                    sizeof (char*),
                    sizeof (char*) * width,
                    sizeof (Rgbaz)));
        }
    }
    else
    {
        if (hash64)
        {
            h.channels ().insert ("model.id0", UINT);
            buf.insert (
                "model.id0",
                DeepSlice (
                    UINT,
                    (char*) ptrID0.data (),
                    sizeof (char*),
                    sizeof (char*) * width,
                    sizeof (Rgbaz)));
            h.channels ().insert ("model.id1", UINT);
            buf.insert (
                "model.id1",
                DeepSlice (
                    UINT,
                    (char*) ptrID1.data (),
                    sizeof (char*),
                    sizeof (char*) * width,
                    sizeof (Rgbaz)));

            h.channels ().insert ("material.id0", UINT);
            buf.insert (
                "material.id0",
                DeepSlice (
                    UINT,
                    (char*) ptrID2.data (),
                    sizeof (char*),
                    sizeof (char*) * width,
                    sizeof (Rgbaz)));
            h.channels ().insert ("material.id1", UINT);
            buf.insert (
                "material.id1",
                DeepSlice (
                    UINT,
                    (char*) ptrID3.data (),
                    sizeof (char*),
                    sizeof (char*) * width,
                    sizeof (Rgbaz)));
        }
        else
        {
            h.channels ().insert ("modelid", UINT);
            buf.insert (
                "modelid",
                DeepSlice (
                    UINT,
                    (char*) ptrID0.data (),
                    sizeof (char*),
                    sizeof (char*) * width,
                    sizeof (Rgbaz)));
            h.channels ().insert ("materialid", UINT);
            buf.insert (
                "materialid",
                DeepSlice (
                    UINT,
                    (char*) ptrID2.data (),
                    sizeof (char*),
                    sizeof (char*) * width,
                    sizeof (Rgbaz)));
        }
    }

    h.channels ().insert ("particleid", UINT);
    buf.insert (
        "particleid",
        DeepSlice (
            UINT,
            (char*) ptrID4.data (),
            sizeof (char*),
            sizeof (char*) * width,
            sizeof (Rgbaz)));

    if (objectID)
    {
        h.insert ("objectID", StringVectorAttribute (oldObjectID));
    }
    else
    {
        IDManifest manifest;
        manifest.add (modelOrMultiManifest);
        if (!multivariate) { manifest.add (materialManifest); }
        IDManifest::ChannelGroupManifest particleManifest;
        particleManifest.setChannel ("particleid");
        particleManifest.setEncodingScheme (IDManifest::ID_SCHEME);
        particleManifest.setHashScheme (IDManifest::NOTHASHED);
        particleManifest.setLifetime (IDManifest::LIFETIME_SHOT);
        manifest.add (particleManifest);

        addIDManifest (h, manifest);
    }

    //
    // samples are depth sorted, and do not overlap
    //
    addDeepImageState (h, DIS_TIDY);

    DeepScanLineOutputFile file (outputFile, h);
    file.setFrameBuffer (buf);
    file.writePixels (height);
}

int
random (std::default_random_engine& generator, int max)
{
    std::uniform_int_distribution<int> dist (0, max);
    return dist (generator);
}

void
drawBlob (
    std::vector<std::vector<Rgbaz>>& image,
    int                              width,
    int                              height,
    float                            x,
    float                            y,
    float                            z,
    int                              size,
    int                              color,
    const uint32_t*                  ids)
{
    //
    // draw windowed gaussian centered at (x,y)
    //

    float sigma = 5 + 40 * size;

    int ptr = 0;

    Rgbaz point;
    point.z   = z;
    point.id0 = ids[0];
    point.id1 = ids[1];
    point.id2 = ids[2];
    point.id3 = ids[3];
    point.id4 = ids[4];

    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < width; ++i)
        {
            float dist_sq = (x - i) * (x - i) + (j - y) * (j - y);
            float scale =
                exp (-dist_sq / sigma) * (1.f - sqrt (dist_sq) / sigma);

            if (scale > 0.001)
            {
                point.r = colors[color].r * scale;
                point.g = colors[color].g * scale;
                point.b = colors[color].b * scale;
                point.a = scale;

                image[ptr].push_back (point);
            }

            ptr++;
        }
    }
}

float
getAlpha (float xmin, float ymin, float xmax, float ymax, float size)
{
    bool in1 = xmin * xmin + ymin * ymin < size * size;
    bool in2 = xmax * xmax + ymin * ymin < size * size;
    bool in3 = xmin * xmin + ymax * ymax < size * size;
    bool in4 = xmin * xmin + ymax * ymax < size * size;

    if (in1 && in2 && in3 && in4) { return (xmax - xmin) * (ymax - ymin); }
    if (!in1 && !in2 && !in3 && !in4) { return 0.f; }
    if (xmax - xmin < 0.001 && ymax - ymin < 0.001) { return 0.f; }
    return getAlpha (xmin, ymin, (xmin + xmax) / 2, (ymin + ymax) / 2, size) +
           getAlpha ((xmin + xmax) / 2, ymin, xmax, (ymin + ymax) / 2, size) +
           getAlpha (xmin, (ymin + ymax) / 2, (xmin + xmax) / 2, ymax, size) +
           getAlpha ((xmin + xmax) / 2, (ymin + ymax) / 2, xmax, ymax, size);
}

void
drawCircle (
    std::vector<std::vector<Rgbaz>>& image,
    int                              width,
    int                              height,
    float                            x,
    float                            y,
    float                            z,
    int                              size,
    int                              color,
    const uint32_t*                  ids)
{
    int   ptr    = 0;
    float radius = 3 + 8 * size;
    Rgbaz point;
    point.z   = z;
    point.id0 = ids[0];
    point.id1 = ids[1];
    point.id2 = ids[2];
    point.id3 = ids[3];
    point.id4 = ids[4];

    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < width; ++i)
        {
            float alpha = getAlpha (
                i - x - 0.5f, j - y - 0.5f, i - x + 0.5f, j - y + 0.5f, radius);
            if (alpha > 0)
            {
                point.r = colors[color].r * alpha;
                point.g = colors[color].g * alpha;
                point.b = colors[color].b * alpha;
                point.a = alpha;

                image[ptr].push_back (point);
            }
            ptr++;
        }
    }
}
