#
# --------------------------------------
# Program Name:    ConvertFrame
# Author:          Penguin_Spy
# License:         Use, but don't steal my code thx
# Description:     Turns a 2-color 320x240 .png into a list describing the image as the length between each color change.
# --------------------------------------
#


# Largely copy+pasted from dbr's answer to https://stackoverflow.com/questions/1109422/getting-list-of-pixel-values-from-pil
from PIL import Image
import cv2
import sys

inputPath = sys.argv[1]
outputPath = sys.argv[2]


# Read the video from specified path
inputVideo = cv2.VideoCapture(inputPath)

title = "Bad Apple"

frameByteArray = bytearray("LLV", "ascii")
frameByteArray.append(0b00000000)   # Version (0=debug)
frameByteArray.append(0b01000000)   # Features (0 - Captions; 1 - Sound; 2-7 Reserved, should be zeros.)
frameByteArray.append(0b00000101)   # FPS
frameByteArray += bytearray(len(title).to_bytes(1, 'big'))   # Title length
frameByteArray += bytearray(title, "ascii")  # Title
# captionCount
# *captions
frameCountIndex = len(frameByteArray)  # Frame Count Index (to insert into later)

frameCount = 0

while(True):

    # reading from frame
    ret, frame = inputVideo.read()

    if ret:  # Convert to PIL & encode as LLV
        #cv2.imwrite(''+str(frameCount) + "a.png", frame)
        color_coverted = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        img = Image.fromarray(color_coverted)

        #img.save(''+str(frameCount) + "b.png")

        img = img.convert('1', dither=False)  # convert image to 2 color black and white
        img = img.resize((320, 240))  # , box=(0, 0, width, height)
        #img.save(''+str(frameCount) + "c.png")

        pixels = img.load()  # this is not a list, nor is it list()'able
        color = pixels[0, 0]
        width, height = img.size

        frameByteArray.append(0b00000000 + ((color == 0) << 7))  # Frame Header
        lineCountIndex = len(frameByteArray)                    # Line Count Index (to insert into later)
        currentLineLength = -1  # Start at -1 so that we enter the first pixel at line length 0
        lineCount = 0

        for r in range(height):
            for c in range(width):
                cpixel = pixels[c, r]

                currentLineLength = currentLineLength + 1

                # By running both of these checks independently and in this order, we get the desired behavior for lines of exactly 255 (0xFF followed by 0x00).
                if currentLineLength > 254:     # If we're out of space in this byte, move onto the next one.
                    frameByteArray.append(currentLineLength)
                    lineCount += 1
                    currentLineLength = 0
                if not(cpixel == color):        # If the color changed from last pixel
                    frameByteArray.append(currentLineLength)
                    lineCount += 1
                    currentLineLength = 0
                    color = cpixel

        # Save the last line
        # currentLineLength is incremented at beginning of loop, so when we exit we're one pixel behind
        frameByteArray.append(currentLineLength + 1)
        lineCount += 1

        # This is how the TI-84+ CE stores 16 bit uints, so thats how we store it.
        lineCountBytes = lineCount.to_bytes(2, 'little')

        frameByteArray.insert(lineCountIndex, lineCountBytes[1])
        frameByteArray.insert(lineCountIndex, lineCountBytes[0])

        frameCount += 1

        print("Encoded frame", frameCount, "with", lineCount, "lines.")

    else:
        break

    if(frameCount > 120):
        break

frameCountBytes = frameCount.to_bytes(2, 'little')
frameByteArray.insert(frameCountIndex, frameCountBytes[1])
frameByteArray.insert(frameCountIndex, frameCountBytes[0])

with open(outputPath, "wb") as frame_bin:
    frame_bin.write(frameByteArray)
