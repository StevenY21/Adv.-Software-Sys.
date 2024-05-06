import os
import sys
from pathlib import Path
import matplotlib.pyplot as plt
import base64

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
        fileTypes['directory'] += len(dirs)
        for f in files:
            osPath = os.path.join(root, f)
            pathPath = Path(osPath)
            if pathPath.is_symlink():
                fileTypes['link'] += 1
            elif os.path.isdir(osPath):
                fileTypes['directory'] += 1
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
    plt.savefig('histogram.jpg')  # Save histogram as jpeg
    plt.show()

if __name__ == "__main__":
    fileTypes = countFileTypes(sys.argv[1])
    plotHistogram(fileTypes, sys.argv[1])
    data_uri = base64.b64encode(open('Graph.png', 'rb').read()).decode('utf-8')
    img_tag = '<img src="data:image/png;base64,{0}">'.format(data_uri)