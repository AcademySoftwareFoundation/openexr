#!/usr/bin/env python3

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from subprocess import PIPE, run

verbose = False
#verbose = True

def exr_header(exr_path):
    '''Return a dict of the attributes in the exr file's header(s)'''
    
    result = run (['exrheader', exr_path],
                  stdout=PIPE, stderr=PIPE, universal_newlines=True)
    if result.returncode != 0:
        print(result.stderr)
        return None, None, None

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
                
    rows = {}
    columns = {"-1" : 0}
    for part_name,part in header.items():
        columns[part_name] = 0
        for attr_name,value in part.items():
            rows[attr_name] = 0

    for part_name,part in header.items():
        for attr_name,value in part.items():
            width = len(str(value))
            columns[part_name] = max(columns[part_name], width)
            columns["-1"] = max(columns["-1"], len(attr_name))
            rows[attr_name] = max(rows[attr_name], 1)

    return header, rows, columns

def write_rst_list_table_row(outfile, attr_name, rows, columns, header, ignore_attr_name=False):
    '''Write a row in the rst list-table of parts/attributes on the page for an exr file'''
    
    if ignore_attr_name:
        outfile.write(f'   * -\n')
    else:
        outfile.write(f'   * - {attr_name}\n')
    for part_name,width in columns.items():
        if part_name == '-1':
            continue
        if part_name in header and attr_name in header[part_name]:
            value = header[part_name][attr_name]
            if type(value) is list:
                value = ' '.join(value)
        else:
            value = ''
        outfile.write(f'     - {value}\n')

def write_exr_page(rst_filename, exr_url, exr_filename, exr_lpath, jpg_lpath):
    
    fd, local_exr = tempfile.mkstemp(".exr")
    os.close(fd)

    try:
        
        result = run (['curl', '-f', exr_url, '-o', local_exr], 
                      stdout=PIPE, stderr=PIPE, universal_newlines=True)
        print(' '.join(result.args))
        if result.returncode != 0:
            raise Exception(f'error: failed to read {exr_url}')
    
        if not os.path.isfile(local_exr):
            raise Exception(f'error: failed to read {exr_url}: no such file {local_exr}')
        
        
        if not os.path.isfile(jpg_lpath):
            result = run (['convert', local_exr, jpg_lpath], 
                          stdout=PIPE, stderr=PIPE, universal_newlines=True)
            print(' '.join(result.args))
        
            if result.returncode != 0:
                raise Exception(f'error: failed to convert {exr_url} to {jpg_lpath}')
            if not os.path.isfile(jpg_lpath):
                print(f'error: no jpg file generated: {jpg_lpath}', file=sys.stderr)
        
        header, rows, columns = exr_header(local_exr)

        os.remove(local_exr)

        if not header:
            print(f'error: no header for {local_exr}', file=sys.stderr)
            return None
        
        print(f'writing rst {rst_filename}')

        with open(rst_filename, 'w') as rstfile:

            rstfile.write(f'..\n')  
            rstfile.write(f'  SPDX-License-Identifier: BSD-3-Clause\n')
            rstfile.write(f'  Copyright Contributors to the OpenEXR Project.\n')
            rstfile.write(f'\n')

            rstfile.write(f'{exr_filename}\n')
            for i in range(0,len(exr_filename)):
                rstfile.write('#')
            rstfile.write(f'\n')
            rstfile.write(f'\n')
            rstfile.write(f':download:`{exr_url}<{exr_url}>`\n')
            rstfile.write(f'\n')

            rstfile.write(f'.. image:: {os.path.basename(jpg_lpath)}\n')
            rstfile.write(f'   :target: {exr_url}\n')
            rstfile.write(f'\n')

            rstfile.write(f'.. list-table::\n')
            rstfile.write(f'   :align: left\n')
        
            if 'name' in rows:
                if len(columns) > 2:
                    rstfile.write(f'   :header-rows: 1\n')
                    rstfile.write(f'\n')
                    write_rst_list_table_row(rstfile, 'name', rows, columns, header, True)
                else:
                    rstfile.write(f'\n')
                    write_rst_list_table_row(rstfile, 'name', rows, columns, header)
            else:
                rstfile.write(f'\n')

            for attr_name,height in rows.items():
                if attr_name == 'name':
                    continue
                write_rst_list_table_row(rstfile, attr_name, rows, columns, header)

    except e:

        print(f'error: {str(e)}', file=sys.stderr)
        
        os.remove(local_exr)
        return None

    return header

def write_readme(index_file, repo, tag, lpath):

    fd, local_readme = tempfile.mkstemp(".rst")

    try:
        
        readme_url = f'{repo}/{tag}/{lpath}' 
        result = run (['curl', '-f', readme_url, '-o', local_readme], 
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

    except e:

        print(f'failed to read {readme_url}: {str(e)}', file=os.stderr)
        os.unlink(local_readme)
        
    return None
        
def readme_notes(readme, exr_filename):

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
                text += '             '
                text += line[7:].replace(' ``',' <b>').replace('``','</b>')
    return text

def write_exr_to_index(index_file, repo, tag, exr_lpath, readme):

    # repo = 'https://raw.githubusercontent.com/cary-ilm/openexr-images'
    # tag = 'docs'
    # exr_lpath = v2/LeftView/Ground.exr

    test_images = 'docs/_test_images/'
    output_dirname = test_images + os.path.dirname(exr_lpath) # docs/_test_images/v2/LeftView
    os.makedirs(output_dirname, exist_ok=True)
    base_path = os.path.splitext(exr_lpath)[0]       # v2/LeftView/Ground
    exr_filename = os.path.basename(exr_lpath)       # Ground.exr
    exr_basename = os.path.splitext(exr_filename)[0] # Ground
    exr_dirname = os.path.dirname(exr_lpath)         # v2/LeftView
    rst_lpath = f'{test_images}{exr_dirname}/{exr_basename}.rst' # docs/_test_images/v2/LeftView/Ground.rst
    jpg_rpath = f'{exr_dirname}/{exr_dirname.replace("/", "_")}_{exr_basename}.jpg'
    jpg_lpath =  test_images + jpg_rpath # docs/_test_images/v2/LeftView/Ground.K@#YSDF.jpg

    exr_url = f'{repo}/{tag}/{exr_lpath}'
    
    header = write_exr_page(rst_lpath, exr_url, exr_filename, exr_lpath, jpg_lpath)

    if not header:
        print(f'error: no header for {exr_lpath}', file=sys.stderr)
        return
    
    num_parts = len(header)
    num_channels = 0
    for p,v in header.items():
        num_channels += len(v)

    index_file.write('     <tr>\n')
    index_file.write(f'          <td style="vertical-align: top; width:250px">\n')
    index_file.write(f'              <a href={base_path}.html> <img width="250" src="../_images/{os.path.basename(jpg_rpath)}"> </a>\n') 
    index_file.write(f'          </td>\n')
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

    notes = readme_notes(readme, exr_filename)
    if notes:
        index_file.write(f'          <td style="vertical-align: top; width:400px">\n')
        index_file.write(f'          <p>\n')
        index_file.write(notes)
        index_file.write(f'          </p>\n')
        index_file.write(f'          </td>\n')

    index_file.write('     <t/r>\n')

    return base_path

repo = sys.argv[1] if len(sys.argv) > 1 else 'https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images'
tag = sys.argv[2] if len(sys.argv) > 2 else 'main'

repo = 'https://raw.githubusercontent.com/cary-ilm/openexr-images'
tag = 'docs'

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

os.makedirs('docs/_test_images', exist_ok=True)

with open('docs/_test_images/index.rst', 'w') as index_file:

    index_file.write('Test Images\n')
    index_file.write('###########\n')
    index_file.write('\n')
    index_file.write('.. toctree::\n')
    index_file.write('   :caption: Test Images\n')
    index_file.write('   :maxdepth: 2\n')
    index_file.write('\n')
    index_file.write('   toctree\n')

    toctree = []
    readme = None
    
    with open('docs/test_images.txt', 'r') as test_images_file:
        for line in test_images_file.readlines():

            lpath = line.strip('\n')
            
            if os.path.basename(lpath) == "README.rst":
                if readme:
                    write_table_close(index_file)
                readme = write_readme(index_file, repo, tag, lpath)
                write_table_open(index_file)
            elif lpath.endswith('.exr'):
                base_path = write_exr_to_index(index_file, repo, tag, lpath, readme)
                toctree.append(base_path)

        if readme:
            write_table_close(index_file)

    with open('docs/_test_images/toctree.rst', 'w') as toctree_file:
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
                

