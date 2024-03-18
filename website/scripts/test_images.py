#!/usr/bin/env python3

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

#
# Generate the "Test Images" page for the openexr.com website
#
# The file "website/test_images.txt" contains urls for either .exr files
# or README.txt files from
# https://github.com/AcademySoftwareFoundation/openexr-images. This
# script processes them into .rst files with a main index listing all
# images (with thumbnails and summaries) and a separate page per image
# file showing the full image and the header contents in a table.
#
# The test_images.txt file references exr files expected to exist in
# the images repo. This script downloads the .exrs and run 'exrheader'
# on them to generate the associated .rst file, which becomes the
# descriptive content on the webpage. Each exr is expected to have an
# accompanying .jpg sidecar file for display on the webpage.
#
# The README.rst files are expected to have a heading, summary text,
# and a ".. list-table::" with descriptive text for selected
# images. The main index pages gets the main summary text, with the
# per-image descriptive text in the table next to the image thumbnail.
#
# Note that the README.rst files in the openexr-images repo are never
# actually processed directly by sphinx.
#
# The generated .rst files all go in the "website/test_images"
# directory underneath the source root. 
#
# Run this script from the source root, then commit the files to git.
#
# Note: The initial version of this process ran during cmake, ran the
# conversion to .jpg explicitly, and stored the proxy .jpg's in the
# source tree for processing by Sphinx. This avoided the need for
# permanent sidecar .jpg's in the openexr-images repo, but it
# complicated the website build, could not get the website build to
# work on Windows. Now we rely on running the exr-to-jpg conversion
# manually.
#

import sys, os, tempfile
from subprocess import PIPE, run

def exr_header(exr_path):
    '''Return a dict of the attributes in the exr file's header(s)
       The index of the dict is the part number as a string, i.e. "0", "1", etc.
       The value of each entry is a dict of attribute/value pairs.

       Also return in rows the list of all attributes across all
       parts, which equates to the complete list of rows in the table,
       since not all parts will necessarily have the same attributes.
    '''
    
    result = run (['exrheader', exr_path],
                  stdout=PIPE, stderr=PIPE, universal_newlines=True)
    if result.returncode != 0:
        raise Exception(f'failed to read header for {exr_path}')

    lines = result.stdout.split('\n') 
        
    name,version = lines[3].split(':')

    header = {}
    current_part = "0"
    current_attr = None
    
    for line in lines[4:]:
        if line.startswith(' part '):
            current_part = line[6:].rstrip(':')
            continue
        
        continuation = line.startswith("    ")
        line = line.strip()
        if len(line) == 0: # blank line
            continue
        if continuation:
            words = line.split(',')
            if len(words) > 1:
                line = words[0]
            line = line.strip().strip('"').replace('  ',' ') 
            if header[current_part][current_attr]:
                header[current_part][current_attr] += ', '
            header[current_part][current_attr] += line
            continue
        
        name_value = line.split(':')
        if len(name_value) > 1:
            current_attr = name_value[0].split(' ')[0]
            if current_part not in header:
                header[current_part] = {}
            header[current_part][current_attr] = name_value[1].strip().strip('"').replace('  ',' ')
                
    rows = set()
    for part_name,part in header.items():
        for attr_name,value in part.items():
            rows.add(attr_name)

    return header, rows

def write_rst_list_table_row(outfile, attr_name, header, rows, ignore_attr_name=False):
    '''Write a row in the rst list-table of parts/attributes on the website page for an exr file'''
    
    if ignore_attr_name:
        outfile.write(f'   * -\n')
    else:
        outfile.write(f'   * - {attr_name}\n')
    for part_number,part in header.items():
        if attr_name in part:
            value = part[attr_name]
            if type(value) is list:
                value = ' '.join(value)
        else:
            value = ''
        outfile.write(f'     - {value}\n')

def write_exr_page(rst_lpath, exr_url, exr_filename, exr_lpath, jpg_url, readme):
    '''Write the website page for each exr: title, image, and list-table of attribute name/value for each part
    
       rst_lpath:    the name of .rst file to write, including directory relative to the source root
       exr_url:      the url of exr
       exr_filename: the exr filename without directory
       exr_lpath:    the exr filename with directory relative to the source root
       jpg_url:      the url of the jpg image (from https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images)

       Return a dict of of the header parts/attributes
    '''
    
    # Download the exr to a temp file
    fd, local_exr = tempfile.mkstemp(".exr")
    os.close(fd)

    header = None
    
    try:
        
        # Download the exr via wget
        
        print(f'wget {exr_url}')
        result = run (['wget', exr_url, '-O', local_exr], 
                      stdout=PIPE, stderr=PIPE, universal_newlines=True)
        if result.returncode != 0 or not os.path.isfile(local_exr):
            raise Exception(f'failed to read {exr_url}: no such file {local_exr}')
        
        # Read the header
        
        header, rows = exr_header(local_exr)

        os.remove(local_exr)

        if not header:
            raise Exception(f'can\'t read header for {local_exr}')
        
        print(f'write {rst_lpath}')

        with open(rst_lpath, 'w') as rst_file:

            # copyright
            rst_file.write(f'..\n')  
            rst_file.write(f'  SPDX-License-Identifier: BSD-3-Clause\n')
            rst_file.write(f'  Copyright Contributors to the OpenEXR Project.\n')
            rst_file.write(f'\n')

            # :download:
            rst_file.write(f'{exr_filename}\n')
            for i in range(0,len(exr_filename)):
                rst_file.write('#')
            rst_file.write(f'\n')
            rst_file.write(f'\n')
            rst_file.write(f':download:`{exr_url}<{exr_url}>`\n')
            rst_file.write(f'\n')

            # .. image::
            rst_file.write(f'.. image:: {jpg_url}\n')
            rst_file.write(f'   :target: {exr_url}\n')
            rst_file.write(f'\n')

            if readme:
                notes = readme_notes(readme, exr_filename, False)
                if notes:
                    rst_file.write(f'\n')
                    rst_file.write(notes)
                    rst_file.write(f'\n')
            
            # .. list-table::
            rst_file.write(f'.. list-table::\n')
            rst_file.write(f'   :align: left\n')
        
            # For multi-part files, add a header. For single-part, put the 'name' attribute at the top, if there is one
            if 'name' in rows:
                if len(header) > 2:
                    rst_file.write(f'   :header-rows: 1\n')
                    rst_file.write(f'\n')
                    write_rst_list_table_row(rst_file, 'name', header, rows, True)
                else:
                    rst_file.write(f'\n')
                    write_rst_list_table_row(rst_file, 'name', header, rows)
            else:
                rst_file.write(f'\n')

            # Write each attribute
            for attr_name in rows:
                if attr_name == 'name':
                    continue
                write_rst_list_table_row(rst_file, attr_name, header, rows)

    except Exception as e:

        os.remove(local_exr)

        raise e
    
    return header

def write_readme(index_file, repo, tag, lpath):
    '''Download the README.txt file and write its contents up to the first ".. list-table::"
       Return a list of all lines in the file.
    '''

    # Download to a temp file
    
    fd, local_readme = tempfile.mkstemp(".rst")

    try:
        
        # Download via wget
        
        readme_url = f'{repo}/{tag}/{lpath}' 
        result = run (['wget', readme_url, '-O', local_readme], 
                      stdout=PIPE, stderr=PIPE, universal_newlines=True)
        if result.returncode != 0:
            raise FileNotFoundError(result.stderr)
    
        text = ''
        found = False
        with open(local_readme, 'r') as readme_file:
            lines = readme_file.readlines()
            for line in lines:
                if '.. list-table::' in line:
                    break
                index_file.write(line[:-1])
                index_file.write('\n')

            os.unlink(local_readme)
            return lines

    except Exception as e:

        os.unlink(local_readme)
        
        raise e
    
    return None
        
def readme_notes(readme, exr_filename, html):
    '''Extract the section of the README.rst file's list-table for the given exr file.

       The list-table line lists the exr as a row header, i.e. with "* -" prefix. Return the lines up to the next row.

       Return None if there's no entry.
    '''

    found = False
    text = ''
    
    for line in readme:
        if line.startswith('   * -'):
            if exr_filename in line:
                found = True
            elif found:
                break
        else:
            if found and line != '\n':
                if not line.startswith(' '):
                    break
                if html:
                    text += '             '
                    text += line[7:].replace(' ``',' <b>').replace('``','</b>')
                else:
                    text += line[7:]
    return text

def write_exr_to_index(index_file, repo, tag, exr_lpath, readme):
    '''Write an entry to the index.rst file for the give exr, in raw html format.

       index_file: the open index.rst file descriptor
       repo:       the repo url
       tag:        the tag/branch of the repo
       exr_lpath:  the name of the exr file, with directory relative to the repo root
       readme:     the lines of the README.txt, returned by write_readme()
    '''
    
    # Examples:
    #    repo = 'https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images'
    #    tag = 'main'
    #    exr_lpath = v2/LeftView/Ground.exr

    test_images = 'website/test_images/'
    output_dirname = test_images + os.path.dirname(exr_lpath)    # website/test_images/v2/LeftView
    os.makedirs(output_dirname, exist_ok=True)
    base_path = os.path.splitext(exr_lpath)[0]                   # v2/LeftView/Ground
    exr_filename = os.path.basename(exr_lpath)                   # Ground.exr
    exr_basename = os.path.splitext(exr_filename)[0]             # Ground
    exr_dirname = os.path.dirname(exr_lpath)                     # v2/LeftView
    rst_lpath = f'{test_images}{exr_dirname}/{exr_basename}.rst' # website/test_images/v2/LeftView/Ground.rst
    jpg_url = f'{repo}/{tag}/{base_path}.jpg'
    exr_url = f'{repo}/{tag}/{exr_lpath}'
    
    # Write the exr page
    
    header = write_exr_page(rst_lpath, exr_url, exr_filename, exr_lpath, jpg_url, readme)

    if not header:
        raise Exception(f'no header for {exr_lpath}')
    
    num_parts = len(header)
    num_channels = 0
    for p,v in header.items():
        num_channels += len(v)

    # open the row
    index_file.write('     <tr>\n')

    # row for the image
    index_file.write(f'          <td style="vertical-align: top; width:250px">\n')
    index_file.write(f'              <a href={base_path}.html> <img width="250" src="{jpg_url}"> </a>\n') 
    index_file.write(f'          </td>\n')

    # row for the summary
    index_file.write(f'          <td style="vertical-align: top; width:250px">\n')
    index_file.write(f'            <b> {exr_filename} </b>\n')
    index_file.write(f'            <ul>\n')
    if num_parts == 1:
        index_file.write(f'                <li> single part </li>\n')
    else:
        index_file.write(f'                <li> {num_parts} parts </li>\n')
    if num_parts == 1:
        index_file.write(f'                <li> 1 channel </li>\n')
    else:
        index_file.write(f'                <li> {num_channels} channels </li>\n')

    if "type" in header["0"]:
        index_file.write(f'                <li> {header["0"]["type"]} </li>\n')
    if "compression" in header["0"]:
        compression = header["0"]["compression"]
        if compression == "zip, individual scanlines":
            compression = "zip"
        elif compression == "zip, multi-scanline blocks":
            compression = "zips"
        index_file.write(f'                <li> {compression} compression </li>\n')
    if "envmap" in header["0"]:
        index_file.write(f'                <li> {header["0"]["envmap"]} </li>\n')

    index_file.write(f'            </ul>\n')
    index_file.write(f'          </td>\n')

    # row for the readme notes, if there are any
    
    if readme:
        notes = readme_notes(readme, exr_filename, True)
        if notes:
            index_file.write(f'          <td style="vertical-align: top; width:400px">\n')
            index_file.write(f'          <p>\n')
            index_file.write(notes)
            index_file.write(f'          </p>\n')
            index_file.write(f'          </td>\n')

    # close the row
    index_file.write('     </tr>\n')

    return base_path

def write_table_open(index_file):

    index_file.write(f'\n')
    index_file.write(f'.. raw:: html\n')
    index_file.write(f'\n')
    index_file.write(f'   <embed>\n')
    index_file.write(f'   <table>\n')
    index_file.write(f'\n')
    
def write_table_close(index_file):

    index_file.write(f'\n')
    index_file.write(f'   </table>\n')
    index_file.write(f'   </embed>\n')
    index_file.write(f'\n')
    
print(f'generating rst for test images ...')

print(f'PATH={os.environ["PATH"]}')
result = run (['which', 'exrheader'],
              stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(f'exrheader={result.stdout}')

repo = sys.argv[1] if len(sys.argv) > 1 else 'https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images'
tag = sys.argv[2] if len(sys.argv) > 2 else 'main'

try:
    
    with open('website/test_images/index.rst', 'w') as index_file:

        index_file.write('Test Images\n')
        index_file.write('###########\n')
        index_file.write('\n')
        index_file.write('.. toctree::\n')
        index_file.write('   :caption: Test Images\n')
        index_file.write('   :maxdepth: 2\n')
        index_file.write('\n')
        index_file.write('   toctree\n')
        index_file.write('\n')

        toctree = []
        readme = None
        table_opened = False
    
        # Process each url in the .txt file
        
        with open('website/test_images.txt', 'r') as test_images_file:
            for line in test_images_file.readlines():

                if line.startswith('#'):
                    continue
                
                lpath = line.strip('\n')
            
                if os.path.basename(lpath) == "README.rst":
                    
                    if table_opened:
                        write_table_close(index_file)
                        table_opened = False

                    readme = write_readme(index_file, repo, tag, lpath)
                
                elif lpath.endswith('.exr'):

                    if not table_opened:
                        write_table_open(index_file)
                        table_opened = True
                    
                    base_path = write_exr_to_index(index_file, repo, tag, lpath, readme)
                    if base_path:
                        toctree.append(base_path)

            if table_opened:
                write_table_close(index_file)

        # Write the toctree file, one entry per .exr page
        
        with open('website/test_images/toctree.rst', 'w') as toctree_file:
            toctree_file.write('..\n')
            toctree_file.write('  SPDX-License-Identifier: BSD-3-Clause\n')
            toctree_file.write('  Copyright Contributors to the OpenEXR Project.\n')
            toctree_file.write('\n')
            toctree_file.write('.. toctree::\n')
            toctree_file.write('   :maxdepth: 0\n')
            toctree_file.write('   :hidden:\n')
            toctree_file.write('\n')
            for t in toctree:
                toctree_file.write(f'   {t}\n')

except Exception as e:

    print(f'error: {str(e)}', file=sys.stderr)
    exit(-1)

exit(0)

                

