#!/usr/bin/env python3

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os
from subprocess import PIPE, run

exrheader = "exrheader"

convert_png = False
verbose = False
#verbose = True

gallery = False

def parse_header(lines):
    
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

def write_list_row(outfile, attr_name, rows, columns, header, ignore_attr_name=False):
    
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

def write_exr_rst(rst_filename, exr_full_path, exr_filename, exr_lpath, jpg_lpath):
    
    result = run ([exrheader, exr_full_path],
                  stdout=PIPE, stderr=PIPE, universal_newlines=True)
#    print(" ".join(result.args))
#    print(f"stdout: {result.stdout}")
#    print(f"stderr: {result.stderr}")
    assert(result.returncode == 0)

    lines = result.stdout.split('\n') 
        
    name,version = lines[3].split(':')

    header, rows, columns = parse_header(lines)

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
        rstfile.write(f':download:`https://github.com/openexr-images/v1.0/{exr_lpath}<https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/v1.0/{exr_lpath}>`\n')
        rstfile.write(f'\n')

        rstfile.write(f'.. image:: https://raw.githubusercontent.com/cary-ilm/openexr-images/docs/{jpg_lpath}\n')
        rstfile.write(f'   :target: https://raw.githubusercontent.com/cary-ilm/openexr-images/docs/{exr_lpath}\n')
        rstfile.write(f'\n')

        rstfile.write(f'.. list-table::\n')
        rstfile.write(f'   :align: left\n')
        
        if 'name' in rows:
            if len(columns) > 2:
                rstfile.write(f'   :header-rows: 1\n')
                rstfile.write(f'\n')
                write_list_row(rstfile, 'name', rows, columns, header, True)
            else:
                rstfile.write(f'\n')
                write_list_row(rstfile, 'name', rows, columns, header)
        else:
            rstfile.write(f'\n')

        for attr_name,height in rows.items():
            if attr_name == 'name':
                continue
            write_list_row(rstfile, attr_name, rows, columns, header)

    return header

def readme_notes(rst_path, exr_filename):
    text = ''
    found = False
    with open(rst_path) as infile:
        for line in infile.readlines():
            if line.startswith('   * -'):
                if exr_filename in line:
                    found = True
                elif found:
                    break
            else:
                if found:
                    if line != '\n':
                        if not line.startswith(' '):
                            break
                        text += '             ' + line[7:].replace(' ``',' <b>').replace('``','</b>')
    return text

def write_directory(index_file, exr_root, directory):
    
    print(f'write_directory({exr_root}, {directory}')
    
    toctree = []
    exrs = []
    
    for filename in os.listdir(directory):
        if filename.startswith('.'):
            continue
        
        path = f'{directory}/{filename}'

        if os.path.basename(filename) == 'README.rst':
            index_file.write('\n')
            with open(path, 'r') as f:
                for line in f.readlines():
                    if '.. list-table::' in line:
                        break
                    index_file.write(line)
            index_file.write('\n')
        elif filename.endswith('.exr'):
            exrs.append(path)
                                 
    index_file.write('.. raw:: html\n')
    index_file.write('\n')
    index_file.write('   <embed>\n')
    index_file.write('   <table style="border:0">\n')
    if gallery:
        index_file.write('     <tr>\n')
            
    row = 0
    
    exrs.sort()
    
    for path in exrs:

        print(f'processing {path}')
    
        # path=/home/cary/src/cary-ilm/openexr-images/v2/LeftView/Ground.exr
        
        exr_lpath = path.lstrip(exr_root) # v2/LeftView/Ground.exr
        dirname = os.path.dirname(exr_lpath) # v2/LeftView
        os.makedirs(dirname, exist_ok=True)
        exr_filename = os.path.basename(exr_lpath) # Ground.exr
        base_path = os.path.splitext(exr_lpath)[0] # v2/LeftView/Ground
        jpg_lpath = base_path + '.jpg' # v2/LeftView/Ground.jpg
        rst_filename = base_path + '.rst' # v2/LeftView/Ground.rst
        readme_path = os.path.dirname(path) + '/README.rst' 
        header = write_exr_rst(rst_filename, path, exr_filename, exr_lpath, jpg_lpath)

        num_parts = len(header)
        num_channels = 0
        for p,v in header.items():
            num_channels += len(v)

        if gallery:
            
            if row > 0 and row % 3 == 0:
                index_file.write('     </tr>\n')
                index_file.write('     <tr>\n')
        
            src = f'https://raw.githubusercontent.com/cary-ilm/openexr-images/docs/{jpg_lpath}'
            index_file.write(f'          <td style="vertical-align: top;">\n')
            index_file.write(f'            <figure style="margin:0">\n') 
            index_file.write(f'              <a href={base_path}.html> <img width="235" src="{src}"> </a>\n') 
            index_file.write(f'              <figcaption>\n')
            index_file.write(f'              {exr_filename}\n')
            index_file.write(f'              <ul>\n')

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

            index_file.write(f'              </ul>\n')
            index_file.write(f'              </figcaption>\n')
            index_file.write(f'            </figure>\n')
            index_file.write(f'          </td> \n')
        else:

            src = f'https://raw.githubusercontent.com/cary-ilm/openexr-images/docs/{jpg_lpath}'
            index_file.write('     <tr>\n')
            index_file.write(f'          <td style="vertical-align: top; width:250px">\n')
            index_file.write(f'              <a href={base_path}.html> <img width="250" src="{src}"> </a>\n') 
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

            notes = readme_notes(readme_path, exr_filename)
            if notes:
                index_file.write(f'          <td style="vertical-align: top; width:400px">\n')
                index_file.write(f'          <p>\n')
                index_file.write(notes)
                index_file.write(f'          </p>\n')
                index_file.write(f'          </td>\n')
                
            index_file.write('     </tr>\n')
        row += 1

        toctree.append(f'{base_path}')
    
        if convert_png:
            command = ['ffmpeg', '-i', exr_full_path, png_filename]
            print(f"running {' '.join(command)}")
            result = run (command, stdout=PIPE, stderr=PIPE, universal_newlines=True)
            print(f"stdout: {result.stdout}")
            print(f"stderr: {result.stderr}")
            
    if gallery:
        index_file.write('     </tr>\n')

    index_file.write('   </table>\n')
    index_file.write('   </embed>\n')
    index_file.write('\n')

    return toctree
        
def write_index(exr_root):
    
    index_filename = "index.rst"
    index_file = open(index_filename, 'w')

    index_file.write('..\n')
    index_file.write('  SPDX-License-Identifier: BSD-3-Clause\n')
    index_file.write('  Copyright Contributors to the OpenEXR Project.\n')
    index_file.write('\n')
    index_file.write('.. _Test Images:\n')
    index_file.write('\n')

    index_file.write('.. toctree::\n')
    index_file.write('   :caption: Test Images\n')
    index_file.write('   :maxdepth: 2\n')
    index_file.write('\n')
    index_file.write('   index_toctree\n')
    index_file.write('\n')

    sections = [ 
            "TestImages",
            "ScanLines",
            "Tiles",
            "Chromaticities",
            "LuminanceChroma",
            "DisplayWindow",
            "Beachball",
            "MultiView",
            "MultiResolution",
            "v2/Stereo",
            "v2/LeftView",
            "v2/LowResLeftView",
    ]

    toctree = []
    for section in sections:
        toctree += write_directory(index_file, exr_root, exr_root + '/' + section) 
    
    with open('index_toctree.rst', 'w') as toctree_file:
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
    
print(f'sys.argv: {sys.argv}')

if len(sys.argv) > 1:
    exr_root = sys.argv[1]
else:
    exr_root = '/home/cary/src/cary-ilm/openexr-images'

write_index(exr_root)


        
