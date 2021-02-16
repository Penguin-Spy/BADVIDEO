#
# --------------------------------------
# Program Name:    ConvertFrame
# Author:          Penguin_Spy
# License:         Use, but don't steal my code thx
# Description:     Turns a 2-color 320x240 video into a .bin file describing the image as the length between each color change. Format can be found in format.md
# --------------------------------------
#


# Largely copy+pasted from dbr's answer to https://stackoverflow.com/questions/1109422/getting-list-of-pixel-values-from-pil
from PIL import Image
from tinytag import TinyTag
import cv2
import sys
import os


def encodeFrame(frame):
    # Convert to PIL & encode as LLV
    color_coverted = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    img = Image.fromarray(color_coverted)

    img = img.convert('1', dither=False)  # convert image to 2 color black and white
    img = img.resize((320, 240))  # , box=(0, 0, width, height)

    pixels = img.load()  # this is not a list, nor is it list()'able
    width, height = img.size
    color = pixels[0, 0]

    frameHeader = 0b10000000 * (color == 0)  # Starting color (1 = black)

    horizontalFrameBytes = bytes()
    # verticalFrameBytes = bytes()
    currentLineLength = -1  # Start at -1 so that we enter the first pixel at line length 0
    horizontalLineCount = 0
    # verticalLineCount = 0

    # Compress horizontally
    for r in range(height):
        for c in range(width):
            cpixel = pixels[c, r]

            currentLineLength = currentLineLength + 1

            # By running both of these checks independently and in this order, we get the desired behavior for lines of exactly 255 (0xFF followed by 0x00).
            if currentLineLength > 254:     # If we're out of space in this byte, move onto the next one.
                horizontalFrameBytes += bytes([currentLineLength])
                horizontalLineCount += 1
                currentLineLength = 0
            if not(cpixel == color):        # If the color changed from last pixel
                horizontalFrameBytes += bytes([currentLineLength])
                horizontalLineCount += 1
                currentLineLength = 0
                color = cpixel

        # Save the last line
        # currentLineLength is incremented at beginning of loop, so when we exit we're one pixel behind
    horizontalFrameBytes += bytes([currentLineLength + 1])
    horizontalLineCount += 1

    currentLineLength = -1  # Start at -1 so that we enter the first pixel at line length 0
    color = pixels[0, 0]

    # Compress vertically
    # for c in range(width):
    #    for r in range(height):
    #        cpixel = pixels[c, r]
    #
    #        currentLineLength = currentLineLength + 1
    #
    #        # By running both of these checks independently and in this order, we get the desired behavior for lines of exactly 255 (0xFF followed by 0x00).
    #        if currentLineLength > 254:     # If we're out of space in this byte, move onto the next one.
    #            verticalFrameBytes += bytes([currentLineLength])
    #            verticalLineCount += 1
    #            currentLineLength = 0
    #        if not(cpixel == color):        # If the color changed from last pixel
    #            verticalFrameBytes += bytes([currentLineLength])
    #            verticalLineCount += 1
    #            currentLineLength = 0
    #            color = cpixel

    # Save the last line
    # currentLineLength is incremented at beginning of loop, so when we exit we're one pixel behind
    # verticalFrameBytes += bytes([currentLineLength + 1])
    # verticalLineCount += 1

    # Comparison of file sizes with different directions:
    # Direction | #files | last size | max frame size
    # Horizontal    04        2,875      1063 bytes
    # Vertical      03       40,075      1492 bytes
    # Lowest of ↑   03       28,993      1063 bytes  -  this is the one that gets used
    # Highest lol   04       14,066      1492 bytes
    # if (len(horizontalFrameBytes) < len(verticalFrameBytes)):
    frameBytes = horizontalFrameBytes
    lineCount = horizontalLineCount
    # else:
    #    frameBytes = verticalFrameBytes
    #    lineCount = verticalLineCount
    #    frameHeader += 0b01000000   # Compression direction (1 = vertical)

    # Frame Header & This is how the TI-84 stores 16 bit ints, so thats how we store it.
    frameBytes = bytes([frameHeader]) + lineCount.to_bytes(2, 'little') + frameBytes

    print("Encoded frame", frameTotal, "with", lineCount, "lines.")

    return frameBytes


def compressBytes(uncompressedBytes):
    try:
        os.remove("temp.bin.zx7")
    except FileNotFoundError:
        0
    uncompressedFile = open("temp.bin", 'wb')
    uncompressedFile.write(uncompressedBytes)
    uncompressedFile.close()
    os.system('@"tools/zx7.exe" temp.bin')
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


inputPath = sys.argv[1]
outputPath = sys.argv[2]
if (not outputPath.endswith('/')):
    outputPath += '/'
varName = sys.argv[3][0:8].upper()

# Read the video from specified path
inputVideo = cv2.VideoCapture(inputPath)
inputVideoTag = TinyTag.get(inputPath)


# title = "Bad Apple"
title = inputVideoTag.title
if (title == None):
    print("[Warning]: Title is empty. Setting to default: 'unknown'")
    title = "unknown"
try:
    titleBytes = bytes(title, "ascii")  # Title
except Exception:
    print("[Warning]: Title could not be encoded via ascii. Setting to default: 'unknown'")
    title = "unknown"

headerBytes = bytes("LLVH", "ascii")
headerBytes += bytes([0b00000000])   # Version (0=debug)
headerBytes += bytes([0b01000000])   # Features (0 - Captions; 1 - Sound; 2-7 Reserved, should be zeros.)
headerBytes += bytes([0b00000101])   # FPS
titleBytes = bytes(title, "ascii")   # Title
headerBytes += len(title).to_bytes(1, 'big')  # Title length
headerBytes += titleBytes
# captionCount
# *captions


frameTotal = 0  # Overall total frames in the entire video
repeatedFrames = 0  # How many frames in a row were identical
fileIndex = 0  # Which file is this
maxFrameSize = 0  # Size of the largest frame

unCVideoBytes = bytes()  # Uncompressed video bytes
cVideoBytes = bytes()   # Compressed video bytes

while(True):
    # reading from frame
    ret, frame = inputVideo.read()

    if ret:
        frameBytes = encodeFrame(frame)

        unCVideoBytes += frameBytes

        frameTotal += 1

        if ((frameTotal % 8) == 0):  # Compress in batches of 8 frames
            cVideoBytes += compressBytes(unCVideoBytes)
            unCVideoBytes = bytes()

        if (len(frameBytes) > maxFrameSize):
            maxFrameSize = len(frameBytes)

        # if (frameTotal > 1):  # Don't run this on the first loop
        # if (frameBytes == lastFrameBytes):
        #    repeatedFrames += 1
        #    print("repeated")
        # else:
        #    print("diff")
        #    if(frameTotal > 0):
        #        print("appending")
        #        if (repeatedFrames > 0):  # encode lastFrameBytes
        #            lastFrameBytes = bytes(lastFrameBytes[0] & 0b00100000) + \
        #                repeatedFrames.to_bytes(2, 'little') + lastFrameBytes[1:]
        #        uncompressedVideoBytes += lastFrameBytes
        #frameTotal += 1
        #frameCount += 1

        #lastFrameBytes = frameBytes

        # if (lastFrameBytes == frameBytes):  # This frame is identical to the last one
        #    repeatedFrames += 1
        # elif (repeatedFrames > 0):          # Not the same, but previously it was (we need to save that frame)
        #    lastFrameHeader = (lastFrameBytes[0] & 0b00100000) + repeatedFrames.to_bytes(2, 'little')
        #    uncompressedVideoBytes += lastFrameHeader + lastFrameBytes[1:]
        #    frameTotal += 1
        #    frameCount += 1
        # else:                               # Normal frame, no shenanagans
        #    frameTotal += 1
        #    frameCount += 1
        #    lastFrameBytes = frameBytes
        #    uncompressedVideoBytes += frameBytes

        # if ((frameTotal % 8) == 0):    # compress batches of 8 frames
        #    compressedVideoBytes += compressBytes(uncompressedVideoBytes)
        #    uncompressedVideoBytes = bytes()

        '''
        if (len(videoBytes) + len(frameBytes) > 65232):  # This frame wont fit in the current file
            if (outputBin == None):  # We're on the first file
                # Save the first bytes for later (writing the file header)
                videoBytesFirst = frameCount.to_bytes(1, 'little') + bytes(videoBytes)
            else:
                save8xv(outputBin, frameCount.to_bytes(1, 'little') + videoBytes)
                # outputBin.write(frameCount.to_bytes(1, 'little') + videoBytes)
                # print("Wrote ", len(videoBytes), " bytes to ", outputBin.name)
                # outputBin.close()
                # os.system('make ' + str(outputBin.name).replace('.bin', '.8xv'))
            outputBin = open(f'{outputPath}{varName[0:6]}{fileIndex:02d}.bin', 'wb')
            # outputPath + varName[0:6] + str(int(fileIndex)) + '.bin', 'wb')
            fileIndex += 1

            frameCount = 0  # Reset count of frames in this file to 0 (because it's a new file)
            videoBytes = bytes()

        else:
            videoBytes += frameBytes'''

    else:
        print("End of video reached.")
        break
    if(frameTotal > 191):
        break

# Finish up loop operations that normally would wait for a condition to be true
# if (repeatedFrames > 0):
#    lastFrameHeader = (lastFrameBytes[0] & 0b00100000) + repeatedFrames.to_bytes(2, 'little')
#    uncompressedVideoBytes += lastFrameHeader + lastFrameBytes[1:]
#    frameTotal += 1
#    frameCount += 1
# if (len(uncompressedVideoBytes) > 0):
#    compressedVideoBytes += compressBytes(uncompressedVideoBytes)


print(unCVideoBytes)
# print(cVideoBytes)

headerBytes += frameTotal.to_bytes(2, 'little')

# TODO: parse output text of zx7.exe & save delta & max buffer size to headers

finalSize = len(headerBytes) + len(cVideoBytes)

# Write the first file with the file header
save8xv(f"{outputPath}{varName}.bin", headerBytes + cVideoBytes[0:65232])
cVideoBytes = cVideoBytes[65232:]  # Remove what we just wrote

# If there's more to write
while (len(cVideoBytes) > 0):
    save8xv(f"{outputPath}{varName[0:6]}{fileIndex:02d}.bin", cVideoBytes[0:65232])
    cVideoBytes = cVideoBytes[65232:]
    fileIndex += 1

# if (len(compressedVideoBytes) > 0):  # There is still a file left to save
#    if (outputBin == None):  # It's actually just the first file
#        videoBytesFirst = frameCount.to_bytes(1, 'little') + bytes(compressedVideoBytes)
#    else:
#        save8xv(outputBin, frameCount.to_bytes(1, 'little') + compressedVideoBytes)
# outputBin.write(frameCount.to_bytes(1, 'little') + videoBytes)
# os.system('make ' + str(outputBin.name).replace('.bin', '.8xv'))
# print("Wrote ", len(videoBytes), " bytes to ", outputBin.name)
# outputBin.close()


# save header + total frame count + video bytes (this file's frameCount was saved into videoBytesFirst earlier)
# videoBytesFirst = headerBytes + frameTotal.to_bytes(2, 'little') + videoBytesFirst

# save8xv(outputBinFirst, videoBytesFirst)
# outputBinFirst.write(videoBytesFirst)
# os.system('make ' + str(outputBinFirst.name).replace('.bin', '.8xv'))

print(f"Converted {inputPath} to {outputPath}{varName}.8xp with {fileIndex + 1} files ({finalSize} bytes in total).\
      \nThe largest single frame was {maxFrameSize} lines ({maxFrameSize + 3} bytes).")
