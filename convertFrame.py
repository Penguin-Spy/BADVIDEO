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


def save8xv(outFile, outBytes):
    outFile.write(outBytes)
    print("Wrote ", len(outBytes), " bytes to ", outFile.name)
    outFile.close()
    os.system('@make ' + str(outFile.name).replace('.bin', '.8xv'))
    os.remove(outFile.name)  # Remove intermediate .bin file


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

headerBytes = bytes("LLVH", "ascii")
headerBytes += bytes([0b00000000])        # Version (0=debug)
headerBytes += bytes([0b01000000])   # Features (0 - Captions; 1 - Sound; 2-7 Reserved, should be zeros.)
headerBytes += bytes([0b00000101])   # FPS
headerBytes += len(title).to_bytes(1, 'big')  # Title length
try:
    headerBytes += bytes(title, "ascii")  # Title
except Exception:
    print("[Warning]: Title could not be encoded via ascii. Setting to default: 'unknown'")
    title = "unknown"
# captionCount
# *captions


frameTotal = 0  # Overall total frames in the entire video
frameCount = 0  # How many frames are in this file
fileIndex = 0  # Which file is this
maxFrameSize = 0  # Size of the largest frame

outputBin = None
videoBytes = bytes()
outputBinFirst = open(outputPath + varName[0:8] + '.bin', 'wb')
videoBytesFirst = bytes()

while(True):

    # reading from frame
    ret, frame = inputVideo.read()

    if ret:  # Convert to PIL & encode as LLV
        # cv2.imwrite(''+str(frameTotal) + "a.png", frame)
        color_coverted = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        img = Image.fromarray(color_coverted)

        # img.save(''+str(frameTotal) + "b.png")

        img = img.convert('1', dither=False)  # convert image to 2 color black and white
        img = img.resize((320, 240))  # , box=(0, 0, width, height)
        # img.save(''+str(frameTotal) + "c.png")

        pixels = img.load()  # this is not a list, nor is it list()'able
        width, height = img.size
        color = pixels[0, 0]

        frameHeader = 0b10000000 * (color == 0)  # Starting color (1 = black)

        horizontalFrameBytes = bytes()
        verticalFrameBytes = bytes()
        currentLineLength = -1  # Start at -1 so that we enter the first pixel at line length 0
        horizontalLineCount = 0
        verticalLineCount = 0

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
        for c in range(width):
            for r in range(height):
                cpixel = pixels[c, r]

                currentLineLength = currentLineLength + 1

                # By running both of these checks independently and in this order, we get the desired behavior for lines of exactly 255 (0xFF followed by 0x00).
                if currentLineLength > 254:     # If we're out of space in this byte, move onto the next one.
                    verticalFrameBytes += bytes([currentLineLength])
                    verticalLineCount += 1
                    currentLineLength = 0
                if not(cpixel == color):        # If the color changed from last pixel
                    verticalFrameBytes += bytes([currentLineLength])
                    verticalLineCount += 1
                    currentLineLength = 0
                    color = cpixel

        # Save the last line
        # currentLineLength is incremented at beginning of loop, so when we exit we're one pixel behind
        verticalFrameBytes += bytes([currentLineLength + 1])
        verticalLineCount += 1

        # Comparison of file sizes with different directions:
        # Direction | #files | last size | max frame size
        # Horizontal    04        2,875      1063 bytes
        # Vertical      03       40,075      1492 bytes
        # Lowest of ↑   03       28,993      1063 bytes  -  this is the one that gets used
        # Highest lol   04       14,066      1492 bytes
        if (len(horizontalFrameBytes) < len(verticalFrameBytes)):
            frameBytes = horizontalFrameBytes
            lineCount = horizontalLineCount
        else:
            frameBytes = verticalFrameBytes
            lineCount = verticalLineCount
            frameHeader += 0b01000000   # Compression direction (1 = vertical)
        # assign to frameBytes
        # set bit in header
        # update calc program to read it
        # but not before seeing how bad it looks when we don't decode in the right direction

        # Frame Header & This is how the TI-84 stores 16 bit ints, so thats how we store it.
        frameBytes = bytes([frameHeader]) + lineCount.to_bytes(2, 'little') + frameBytes

        if (len(frameBytes) > maxFrameSize):
            maxFrameSize = len(frameBytes)

        frameTotal += 1
        frameCount += 1

        print("Encoded frame", frameTotal, "with", lineCount, "lines.")

        if (len(videoBytes) + len(frameBytes) > 65232):  # This frame wont fit in this file
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
            videoBytes += frameBytes

    else:
        print("End of video reached.")
        break

    if(frameTotal > 1000):
        break

if (len(videoBytes) > 0):  # There is still a file left to save
    if (outputBin == None):  # It's actually just the first file
        videoBytesFirst = frameCount.to_bytes(1, 'little') + bytes(videoBytes)
    else:
        save8xv(outputBin, frameCount.to_bytes(1, 'little') + videoBytes)
        # outputBin.write(frameCount.to_bytes(1, 'little') + videoBytes)
        # os.system('make ' + str(outputBin.name).replace('.bin', '.8xv'))
        # print("Wrote ", len(videoBytes), " bytes to ", outputBin.name)
        # outputBin.close()


# save header + total frame count + video bytes (this file's frameCount was saved into videoBytesFirst earlier)
videoBytesFirst = headerBytes + frameTotal.to_bytes(2, 'little') + videoBytesFirst

save8xv(outputBinFirst, videoBytesFirst)
# outputBinFirst.write(videoBytesFirst)
# os.system('make ' + str(outputBinFirst.name).replace('.bin', '.8xv'))

print(f"Converted {inputPath} to {varName}.8xp with {fileIndex} files.\nThe largest single frame was {maxFrameSize} bytes.")
