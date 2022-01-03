#
# --------------------------------------
# Program Name:    ConvertFrame
# Author:          Penguin_Spy
# Description:     Turns a 2-color 320x240 video into a set of .8xv files describing each frame as the length between each color change. Format can be found in format.md
# License:
#    Copyright (C) 2022  Penguin_Spy
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
# --------------------------------------
#

# Largely copy+pasted from dbr's answer to https://stackoverflow.com/questions/1109422/getting-list-of-pixel-values-from-pil
from PIL import Image
from tinytag import TinyTag
import cv2
import sys
import os

# Conversion constants, must equal main.c's values
FRAMES_PER_CHUNK = 8
LLV_VERSION = 0b00010001 # 1.1 release

# LLV file format defines
#see format.md for descriptions
LLV_FILE_FEATURES_SOUND    = 0b10000000 # UNUSED/RESERVED
LLV_FILE_FEATURES_CAPTIONS = 0b01000000 # UNUSED/RESERVED

LLV_FRAME_HEADER_COLOR          = 0b10000000
LLV_FRAME_HEADER_COMPRESSIONDIR = 0b01000000 # UNUSED/RESERVED
LLV_FRAME_HEADER_REPEAT         = 0b00100000
LLV_FRAME_HEADER_SOUND          = 0b00010000 # UNUSED/RESERVED

# Statistic variables (not encoded)
maxFrameSize = 0    # Size of the largest frame
trueFrameTotal = 0  # Duplicate frames still increase this counter

# Compression statistics (BADVIDEO.8xp's values must equal or excede these)
#TODO: encode this into the .llv file (only need to encode the value of (maxUncompressedChunk + maxDelta) as 1 value)
maxUncompressedChunk = 0  # Size of the largest uncompressed chunk
maxDelta = 0              # Maximum delta (distance between uncompressing data into it's own buffer)


def encodeFrame(frame):
    # Convert to PIL & encode as LLV
    color_coverted = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    img = Image.fromarray(color_coverted)

    img = img.convert('1', dither=False)  # convert image to 2 color black and white
    img = img.resize((320, 240))  # , box=(0, 0, width, height)

    pixels = img.load()  # this is not a list, nor is it list()'able
    width, height = img.size
    color = pixels[0, 0]

    startColor = (color == 0)

    frameBytes = bytes()
    currentLineLength = -1  # Start at -1 so that we enter the first pixel at line length 0
    lineCount = 0

    # Compress frame
    for r in range(height):
        for c in range(width):
            cpixel = pixels[c, r]

            currentLineLength = currentLineLength + 1

            # By running both of these checks independently and in this order, we get the desired behavior for lines of exactly 255 (0xFF followed by 0x00).
            if currentLineLength > 254:     # If we're out of space in this byte, move onto the next one.
                frameBytes += bytes([currentLineLength])
                lineCount += 1
                currentLineLength = 0
            if not(cpixel == color):        # If the color changed from last pixel
                frameBytes += bytes([currentLineLength])
                lineCount += 1
                currentLineLength = 0
                color = cpixel

    # Save the last line
    # currentLineLength is incremented at beginning of loop, so when we exit we're one pixel behind
    frameBytes += bytes([currentLineLength + 1])
    lineCount += 1

    if(repeatedFrames > 0):
      print(f"Encoded frame {frameTotal}.{repeatedFrames}")
    else:
      print(f"Encoded frame {frameTotal} with {lineCount} lines.")

    return startColor, lineCount.to_bytes(2, 'little') + frameBytes


def compressBytes(uncompressedBytes):
    if (len(uncompressedBytes) == 0):
        print("[ERROR]: uncompressedBytes is empty")
        return bytes()
    global maxUncompressedChunk
    global maxDelta
    try:
        os.remove("temp.bin.zx7")
    except FileNotFoundError:
        0   # whoo python is amazing right? i love how indentation matters!
    uncompressedFile = open("temp.bin", 'wb')
    uncompressedFile.write(uncompressedBytes)
    uncompressedFile.close()
    out = os.popen('@"tools/zx7.exe" temp.bin').read().split(" ")
    if (maxUncompressedChunk < int(out[9])):
        maxUncompressedChunk = int(out[9])
    if (maxDelta < int(out[14].replace(")", ""))):
        maxDelta = int(out[14].replace(")", ""))
    compressedFile = open("temp.bin.zx7", 'rb')
    compressedBytes = compressedFile.read()
    compressedBytes = len(compressedBytes).to_bytes(2, 'little') + compressedBytes
    compressedFile.close()
    return compressedBytes


def save8xv(outPath, outBytes):
    outFile = open(outPath, 'wb')
    outFile.write(outBytes)
    print("Wrote ", len(outBytes), " bytes to ", outFile.name)
    outFile.close()
    os.system('@make ' + str(outFile.name).replace('.bin', '.8xv'))


def makeHeader(color, compDir, repeatedFrames, sound):
    header = 0
    if (color):
        header += 0b10000000
    if (repeatedFrames > 0):
        header += 0b00100000

    header = bytes([header])
    if (repeatedFrames > 0):
        header += bytes([repeatedFrames])
    return header


inputPath = sys.argv[1]
outputPath = sys.argv[2]
if (not outputPath.endswith('/')):
    outputPath += '/'
varName = sys.argv[3][0:8].upper()
try:
    framesToEncode = int(sys.argv[4])
except:
    framesToEncode = 0
    print(f'[Note]: # of frames to encode was unspecified, encoding whole video!')

# Read the video from specified path
inputVideo = cv2.VideoCapture(inputPath)
inputVideoTag = TinyTag.get(inputPath)

title = inputVideoTag.title
if (title == None):
    title = "unknown"
    print(f'[Warning]: Title is empty. Setting to default: "{title}"')
try:
    titleBytes = bytes(title, "ascii")  # Title
except Exception:
    title = varName
    print(f'[Warning]: Title could not be encoded via ascii. Setting to varName: "{title}"')

headerBytes = bytes("LLVH", "ascii") # File "extension", used by ti_DetectVar
headerBytes += bytes([LLV_VERSION])  # Version
headerBytes += bytes([0b00000000])   # Features (0 - Sound; 1 - Captions; 2-7 Reserved, should be zeros.)
headerBytes += bytes([int(inputVideo.get(cv2.CAP_PROP_FPS))])   # FPS
headerBytes += bytes([0x00])   # Off color (set manually)
headerBytes += bytes([0xff])   # On color (set manually) badapple: 0xff, lagtrain: 0xb5
titleBytes = bytes(title, "ascii")   # get title
headerBytes += len(title).to_bytes(1, 'big')  # Title length
headerBytes += titleBytes            # Title

frameTotal = 0  # Overall total frames in the entire video
repeatedFrames = 0  # How many frames in a row were identical
fileIndex = 0  # Which file is this

#used for detecting repeated frames
oldFrameBytes = bytes()
oldColor = 0

unCVideoBytes = bytes()  # Uncompressed video bytes
cVideoBytes = bytes()   # Compressed video bytes

while(True):
    # reading from frame
    ret, frame = inputVideo.read()

    if ret:
        color, frameBytes = encodeFrame(frame)  # Encode this frame

        # repeat frame detection
        if (frameBytes == oldFrameBytes and color == oldColor):
            repeatedFrames += 1
        elif (len(oldFrameBytes) != 0):
            unCVideoBytes += makeHeader(oldColor, 0, repeatedFrames, 0) + oldFrameBytes
            repeatedFrames = 0
            frameTotal += 1
        oldFrameBytes = frameBytes
        oldColor = color

        # Compress in batches of FRAMES_PER_CHUNK frames
        if ((frameTotal % FRAMES_PER_CHUNK) == 0 and frameTotal != 0):
            cVideoBytes += compressBytes(unCVideoBytes)
            unCVideoBytes = bytes()

        # Statistics
        if (len(frameBytes) > maxFrameSize):
            maxFrameSize = len(frameBytes)
        trueFrameTotal += 1

    else:
        print("End of video reached.")
        break
    if(frameTotal > framesToEncode and not framesToEncode == 0):  # 0 == encode the whole video
        break

headerBytes += (frameTotal - 1).to_bytes(2, 'little')
finalSize = len(headerBytes) + len(cVideoBytes)

# Write the first file with the file header
cVideoBytes = headerBytes + cVideoBytes
save8xv(f"{outputPath}{varName}.bin", cVideoBytes[0:65232])
cVideoBytes = cVideoBytes[65232:]  # Remove what we just wrote

# If there's more to write
while (len(cVideoBytes) > 0):
    save8xv(f"{outputPath}{varName[0:6]}{fileIndex:02d}.bin", cVideoBytes[0:65232])
    cVideoBytes = cVideoBytes[65232:]
    fileIndex += 1

print(f"Converted {inputPath} to {outputPath}{varName}.8xv with {fileIndex + 1} files ({finalSize} bytes in total).\n" +
      f"The largest single frame was {maxFrameSize} lines ({maxFrameSize + 3} bytes).\n" +
      f"The largest uncompressed chunk was {maxUncompressedChunk} bytes, and the largest decompression delta is {maxDelta} bytes.\n" +
      f"A total of {frameTotal} frames were encoded, and {trueFrameTotal} frames will be displayed.")
