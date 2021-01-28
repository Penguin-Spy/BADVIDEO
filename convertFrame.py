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

inputPath = sys.argv[1]
outputPath = sys.argv[2]

# Read the video from specified path
inputVideo = cv2.VideoCapture(inputPath)
inputVideoTag = TinyTag.get(inputPath)

outputBin = open(outputPath, "wb")

# title = "Bad Apple"
title = inputVideoTag.title
if (title == None):
    print("[Warning]: Title is empty. Setting to default: 'unknown'")
    title = "unknown"

headerBytes = bytes("LLVH", "ascii")
headerBytes += bytes([0b00000000])        # Version (0=debug)
headerBytes += bytes([0b01000000])   # Features (0 - Captions; 1 - Sound; 2-7 Reserved, should be zeros.)
headerBytes += bytes([0b00000101])   # FPS
headerBytes += len(title).to_bytes(1, 'big')   # Title length
headerBytes += bytes(title, "ascii")  # Title
# captionCount
# *captions

videoBytes = bytes()

frameTotal = 0

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
        color = pixels[0, 0]
        width, height = img.size

        frameBytes = bytes()
        currentLineLength = -1  # Start at -1 so that we enter the first pixel at line length 0
        lineCount = 0

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

        # Frame Header & This is how the TI-84 stores 16 bit ints, so thats how we store it.
        frameBytes = bytes([(pixels[0, 0] == 0) * 0x80]) + lineCount.to_bytes(2, 'little') + frameBytes

        frameTotal += 1

        videoBytes += frameBytes

        print("Encoded frame", frameTotal, "with", lineCount, "lines.")

    else:
        break

    if(frameTotal > 120):
        break

# frameTotal twice because we dont generate more files yet
videoBytes = headerBytes + frameTotal.to_bytes(2, 'little') + frameTotal.to_bytes(1, 'little') + videoBytes

outputBin.write(videoBytes)
