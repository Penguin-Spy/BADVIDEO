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
import sys

inputfile = sys.argv[1]
outputfile = sys.argv[2]

i = Image.open(inputfile)

pixels = i.load()  # this is not a list, nor is it list()'able
width, height = i.size

color = 0
currentLineLength = 0
frameByteArray = bytearray("LLV", "ascii")

for r in range(height):
    for c in range(width):
        cpixel = pixels[c, r]
        currentLineLength = currentLineLength + 1

        if not(cpixel == color):
            if color == 0:
                frameByteArray.append(currentLineLength)
                currentLineLength = 0
                color = cpixel
            else:
                frameByteArray.append(currentLineLength)
                currentLineLength = 0
                color = cpixel
        else:
            if currentLineLength > 254:
                frameByteArray.append(currentLineLength)
                currentLineLength = 0

with open(outputfile, "wb") as frame_bin:
    frame_bin.write(frameByteArray)

print(frameByteArray)
