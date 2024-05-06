#!/usr/bin/python3
import os
import sys
from pathlib import Path
import matplotlib.pyplot as plt

def countFileTypes(directory):
    fileTypes = {
        'regular': 0,
        'directory': 0,
        'link': 0,
        'fifo': 0,
        'socket': 0,
        'block': 0,
        'character': 0
    }
    for root, dirs, files in os.walk(directory):
        for d in dirs:
            osPath = os.path.join(root, d)
            pathPath = Path(osPath)
            if pathPath.is_symlink():
                fileTypes['link'] += 1
            elif os.path.isdir(osPath):
                fileTypes['directory'] += 1
        for f in files:
            osPath = os.path.join(root, f)
            pathPath = Path(osPath)
            if pathPath.is_symlink():
                fileTypes['link'] += 1
            elif pathPath.is_fifo():
                fileTypes['fifo'] += 1
            elif pathPath.is_socket():
                fileTypes['socket'] += 1
            elif pathPath.is_block_device():
                fileTypes['block'] += 1
            elif pathPath.is_char_device():
                fileTypes['character'] += 1
            elif os.path.isfile(osPath):
                fileTypes['regular'] += 1
    return fileTypes

def plotHistogram(fileTypes, directory):
    types = []
    counts = []
    for key in fileTypes:
        types += [key]
        counts += [fileTypes[key]]
    if directory == ".":
        directory = "Current Directory"
    
    plt.bar(types, counts)
    plt.xlabel('File Types')
    plt.ylabel('Frequency')
    plt.title('File Type Frequency In ' + directory)
    plt.savefig('histogram.jpg')
    plt.show()

if __name__ == "__main__":
    fileTypes = countFileTypes(sys.argv[1])
    plotHistogram(fileTypes, sys.argv[1])
    print ('''<html>
    <head><title>CS410 Server</title></head>
    <body>
    <div style="text-align: center">
    <font size="16" color="red" display ="block">CS410 Server</br></font>
    <img src="histogram.jpg" alt="histogram" display="block" margin-left="auto" margin-right="auto" width="50%"/>
    </div>
    </body>
    </html>''')