///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2017, Weta Digital Ltd
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

#include "testIDManifest.h"


#include <assert.h>
#include <stdlib.h>

#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfIDManifest.h>
#include <ImfStandardAttributes.h>
#include "tmpDir.h"
#include <zlib.h>

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;
using std::vector;
using std::string;
using std::set;
using std::cout;
using std::endl;
using std::cerr;
namespace
{
    //
    // takes the given header and writes it to an EXR file, followed by a 1x1 pixel data block
    // to make the file complete
    //
    void writeFile(Header & h,const string& filename)
    {
        h.dataWindow().min.x=0;
        h.dataWindow().min.y=0;
        h.dataWindow().max.x=0;
        h.dataWindow().max.y=0;
        h.displayWindow() = h.dataWindow();
        ChannelList chans;
        chans.insert("id",Channel(UINT));
        h.channels() = chans;
        OutputFile file(filename.c_str(),h);
        FrameBuffer buf;
        unsigned int value = 1;
        buf.insert("id",Slice(UINT,(char*) &value,1,0));
        file.setFrameBuffer(buf);
        file.writePixels(1);
    }

    // manifest size, storing fixed size data only
    //  - used as a metric for how effective the optimisations are in serialize(), before
    // zip compression kicks in to squeeze things further
    
    
    
    
    std::ostream& operator<<(std::ostream & out,const IDManifest& mfst)
    {
         for(size_t i=0 ; i < mfst.size() ; ++i)
        {
            
            const IDManifest::ChannelGroupManifest& m = mfst[i];
            bool first=true;
            out << "chans:" ;
            for ( set<string>::const_iterator s = m.getChannels().begin() ; s!= m.getChannels().end(); ++s)
            {
                if(!first)
                {
                    out << ',';  
                }
                else
                {
                    first = false;
                }
                
                out << *s;
                
            }

            out << "\nhash:" <<m.getHashScheme() << endl;
            out << "encoding:" << m.getEncodingScheme() << endl;
            switch(m.getLifetime())
            {
                case IDManifest::LIFETIME_FRAME : out << "lifetime:frame\n";break;
                case IDManifest::LIFETIME_SHOT : out << "lifetime:shot\n";break;
                case IDManifest::LIFETIME_STABLE : out << "lifetime:stable\n";break;
            }
            out << ' ';
            for( vector< string >::const_iterator c = m.getComponents().begin() ; c!= m.getComponents().end() ; ++c)
            {
                out << ';' << *c;
            }
            out << endl;
            for( IDManifest::ChannelGroupManifest::ConstIterator q = m.begin() ; q!= m.end(); ++q)
            {
                out << q.id();
                for( vector< string >::const_iterator c = q.text().begin() ; c!=q.text().end() ; ++c)
                {
                   out << ';' << *c;   
                }
                out << '\n';
            }
            
            
        }
        return out;
    }
    
    void doReadWriteManifest(const IDManifest& mfst,const string& fn,bool dump)
    {
        Header h;
        addIDManifest(h,mfst);
        writeFile(h,fn);
        
        InputFile in(fn.c_str());
        
        const CompressedIDManifest& cmpd =idManifest(in.header()); 
        cerr << "compression: " << cmpd._uncompressedDataSize << " --> " << cmpd._compressedDataSize;
        cerr.flush();
        
        IDManifest read = idManifest(in.header());
        
        std::ostringstream str;
        str << read;
        
        cerr << " raw decoded size: " << str.str().size() << ' ';
    
#define COMPARE_WITH_SIMPLE_ZIP    
#ifdef COMPARE_WITH_SIMPLE_ZIP    
          //
          // allocate a buffer which is guaranteed to be big enough for compression
              //
                 uLongf compressedDataSize = compressBound(str.str().size());
                 vector<char> compressed(compressedDataSize);

                 ::compress((Bytef*) &compressed[0],&compressedDataSize,(const Bytef*) str.str().c_str(),str.str().size());
                 
                 cerr << "simple zip size: " << compressedDataSize << ' ';
#endif                 
        
        cerr.flush();
        if(dump) { cout << read;}
        if(read != mfst)
        {
            cerr << "read manifest didn't match written manifest\n";
            assert(read == mfst);
        }
        remove (fn.c_str());

    }
    
    void testReadWriteManifest(const string& tempDir)
    {
        const string fn = tempDir + "id_manifest.exr";
        IDManifest mfst;
        IDManifest::ChannelGroupManifest& idGroup = mfst.add("id");
        vector<string> comps(2);
        comps[0] = "model";
        comps[1] = "material";
        idGroup.setComponents(comps);
        idGroup.setHashScheme(IDManifest::NOTHASHED);
        idGroup.setLifetime(IDManifest::LIFETIME_STABLE);
        idGroup << 1 << "merino/body" << "wool";
        idGroup << 2 << "merino/body" << "skin";
        idGroup << 3 << "merino/body" << "skin";
        idGroup << 4 << "merino/eye" << "eye";

        set<string> chans2;
        chans2.insert("instance1");
        chans2.insert("instance2");
        IDManifest::ChannelGroupManifest& idGroup2 = mfst.add(chans2);
        idGroup2.setComponent("instance");
        idGroup2.setHashScheme(IDManifest::MURMURHASH3_64);
        idGroup2.setEncodingScheme(IDManifest::ID2_SCHEME);
        idGroup2.insert("1/2/3/4/5");
        idGroup2.insert("6/7/8/9/10");
        idGroup2.insert("11/12/13/14/15");

        doReadWriteManifest(mfst,fn,true);
    }
    
    // do things which are supposed to throw exceptions because they are bad.
    // it is an error if the code does NOT throw an exception
    void testDoingBadThings()
    {
        for(int i=0;i <= 6;++i)
        {
            IDManifest mfst;
            IDManifest::ChannelGroupManifest& idGroup = mfst.add("id");
            vector<string> comps(2);
            comps[0] = "model";
            comps[1] = "material";
            idGroup.setComponents(comps);
            try
            {
                switch(i)
                {
                    case 0 :
                        idGroup << "stringBeforeInt\n";
                        break;
                    case 1 :
                        idGroup << 1 << "notEnoughComponentsAdded";
                        idGroup << 2;
                        break;
                    case 2:
                        idGroup << 1 << "too " << "many " << "components";
                        break;
                    case 3:
                        idGroup.insert(2,"onlyOneComponentInserted");
                        break;
                    case 4 :
                    {
                        vector<string> comps(3);
                        comps[0] = "too";
                        comps[1] = "many";
                        comps[2] = "components";
                        idGroup.insert(3,comps);
                        break;
                    }
                    case 5:
                    {
                        idGroup << 1 << " first " << " entry ";
                        idGroup.setComponent("changeToJustOne");
                        break;
                    }
                    case 6 :
                    {
                        idGroup.insert("noHashSchemeSetSoHashMustBeProvided");
                        break;
                    }
                }
                std::cerr << "ERROR: bad usage of IDManifest not detected in test " << i << "\n";
                assert(false);
            }
           catch(IEX_NAMESPACE::ArgExc & problem)
           {
              cout << "test " << i << " passed: " << problem.what() << endl;
           }
        }
    }
    
    //
    // generate a string of up to 32 characters that could be something
    // found in a manifest, using A-Z,a-z,0-9 only
    //
    std::string randomWord(bool alphaNumeric,const std::vector<std::string>& options)
    {
        if(options.size()>0)
        {
            return options[ rand()% options.size()];
        }
        else
        {
            int length=rand()%32;
            std::string word(length,' ');
            for(int l=0;l<length;++l)
            {
                if(alphaNumeric)
                {
                    int index = rand()%62; // 26 letters*2 for case + 10 digits
                     if(index<26)
                     {
                         word[l] = 'A'+index;
                     }
                     else if(index<52)
                     {
                         word[l] = 'a'+(index-26);
                     }
                     else
                     {
                        word[l] = '0' + (index-52);
                     }
                }
                else
                {
                    word[l] = rand()&0xFF;
                }
            }
            return word;
        }
    }
    
    void testLargeManifest(const string& tempDir)
    {
        const string fn = tempDir + "id_manifest.exr";
        srand(1);
        //
        // generate 100 random files, looking for trouble
        
        for(int pass=0;pass<100;++pass)
        {
        
            //
            // decide on strategy for word generation
            // 
            //
            bool alphaNumeric =  rand()%2; // only characters in set [A-Za-z0-9], or all bytes
            bool useWordList = rand()%2;  // each 'word' is from a small list of choices - should improve compression
            vector<string> randomWords;
            if(useWordList)
            {
                int wordListSize = rand()%256 + 1;
                randomWords.resize(wordListSize);
                for(int word = 0 ; word < wordListSize; ++word)
                {
                    randomWords[word] = randomWord(alphaNumeric,std::vector<std::string>());
                }
            }
            
            IDManifest mfst;
            //
            // each manifest contains up to 10 channel groups, but can contain none
            //
            int groups = rand() % 10;
            cerr << " testing manifest with " << groups << " channel groups of size ";
            for(int group = 0 ; group < groups; group++)
            {
                //
                // insert random number of randomly named channels into this group
                // must be at least one
                //
                int channelsInGroup = (rand() % 9) + 1;
                set<string> channelGroup;
                for(int c=0;c<channelsInGroup;++c)
                {
                    channelGroup.insert(randomWord(alphaNumeric,std::vector<std::string>()));
                }
                IDManifest::ChannelGroupManifest& m= mfst.add(channelGroup);
                
                //
                // random header
                //
                m.setLifetime( IDManifest::IdLifetime(rand()%3));
                m.setEncodingScheme( rand()&1 ? IDManifest::ID_SCHEME : IDManifest::ID2_SCHEME);
                m.setHashScheme( rand()&1 ? IDManifest::MURMURHASH3_32 : IDManifest::MURMURHASH3_64);
                
                // pick a number of components and generate them as random words
                int componentsInGroup = rand()%10;
                vector<string> components(componentsInGroup);
                for( int c=0 ; c<componentsInGroup ; ++c )
                {
                    components[c] = randomWord(alphaNumeric,randomWords);
                }
                m.setComponents(components);
                
                
                //
                // insert entries - each will have the correct number of components
                //
                int entriesInGroup = rand()%(300*(pass+1));
                
                cerr << entriesInGroup << ' ';
                cerr.flush();
                
                for(int e = 0 ; e < entriesInGroup ; ++e )
                {
                    for(int c = 0 ; c < componentsInGroup ; ++c)
                    {
                        // each component consists of a random number of separated words
                        std::string s;
                        int words = rand()%10;
                        static const char* separators = "/_-=,\\,.;|\t";
                        for(int w=0;w<words;++w)
                        {
                            s += randomWord(alphaNumeric,randomWords)+separators[rand()%2];
                        }
                        components[c] = s;
                    }
                    // insert the component list into the manifest - hash will be automatically generated for us
                    m.insert(components);
                }
            }
            std::cerr << "....";
            cerr.flush();
            doReadWriteManifest(mfst,fn,false);
            std::cerr << "ok\n";
        }
    }
        
        
   void testMerge()
   {
       //
       // basic merge tests
       //
       Imf::IDManifest m1 , m2;
       

       m1.add("id");
       m1[0].setComponent("name");
       m1[0].insert(1,"entryOne");
       
       
       {
           // two manifests with non-colliding channels - should append the two
           m2.add("id2");
           m2[0].setComponent("name");
           m2[0].setEncodingScheme(Imf::IDManifest::ID2_SCHEME); // different scheme should not cause issues
           m2[0].insert(2,"entryTwo");
           
           
           Imf::IDManifest m3;
           
           bool reply = m3.merge(m1);
           if(reply)
           {
               cerr << "unexpected return value from merge command: m1 into m3";
               assert(reply==false);
           }
           reply = m3.merge(m2);
           if(reply)
           {
               cerr << "unexpected return value from merge command: m2 into m3";
               assert(reply==false);
           }
           
           if(m3.size()!=2)
           {
               cerr << "expected manifest with two ChannelGroupManifests after merge operation";
               assert(m3.size()==2);
           }
           
           if(m3[0] != m1[0])
           {
               cerr << "manifest merge of m1 failed\n";
               assert(m3[0]==m1[0]);
           }
           if(m3[1] != m2[0])
           {
               cerr << "manifest merge of m1 failed\n";
               assert(m3[0]==m1[0]);
           }
       }
       
       // two manifests with the same channel - should combine
       
       {
           Imf::IDManifest m3;
           m3.add("id");
           //
           // these values should be ignored - merge will succeed
           // but merged manifest m5 should contain values from m1, not m3;
           //
           m3[0].setEncodingScheme(Imf::IDManifest::ID2_SCHEME);
           m3[0].setLifetime(Imf::IDManifest::LIFETIME_SHOT);
           m3[0].setHashScheme(Imf::IDManifest::NOTHASHED);
           m3[0].setComponent("name");
           m3[0].insert(2,"entryTwo");
           
           Imf::IDManifest m4;
           m4.add("id");
           m4[0].setComponent("name");
           m4[0].insert(1,"entryOne");
           m4[0].insert(2,"entryTwo");
           
           Imf::IDManifest m5 = m1;
           
           bool reply = m5.merge(m3);
           if(reply)
           {
               cerr << "unexpected merge conflict of m3 into m5\n";
               cerr << "m1:\n" << m1 << "m3:\n" << m3 << "m5:\n" << m5;
               assert(reply==false);
           }

           if(m5!=m4)
           {
               cerr << "unexpected result from merge:\ngot " << m5 << "\nexpected " << m4 <<std::endl;
               assert(m4==m5);
           }
           
       }
       
       
       // check expected failure situations
       
       {
           Imf::IDManifest m6;
           m6.add("id");
           m6[0].setComponent("notname");
           
           Imf::IDManifest m7 = m1;
           bool reply = m7.merge(m6);
           if( reply == false )
           {
               cerr << "error: merge should have failed: different components in m6 and m7\n";
               assert(reply);
           }
           
           Imf::IDManifest m8;
           m8.add("id");
           m8[0].setComponent("name");
           m8[0].insert(1,"a_different_value");
           m8[0].insert(3,"a further value");
           
           Imf::IDManifest m9 = m1;
           reply = m9.merge(m8);
           if( reply == false )
           {
               cerr << "error: merge should have failed: conflicting values of ID 1 in m8 and m9\n";
               assert(reply);
           }
           //
           // what m9 should be
           //
           Imf::IDManifest m10;
           m10.add("id");
           m10[0].setComponent("name");
           m10[0].insert(1,"entryOne");
           m10[0].insert(3,"a further value");
           
           if( m10 != m9)
           {
               cerr << "unexpected result from merge:\ngot " << m9 << " expected " << m10 << endl;
               assert(m10==m9);
           }
           
           
           
           
       }
       
       
       
       
}
}



void testIDManifest(const std::string &tempDir)
{
    //
    // simple test that the manifest 'round trips' correctly
    //
    testReadWriteManifest(tempDir);
    
    //
    // test manifest merge operations
    //
    testMerge();
    
    // stress test - will randomly generate 'edge cases'
    testLargeManifest(tempDir);
    
    // test the API prevents creating invalid manifests
    testDoingBadThings();
}
