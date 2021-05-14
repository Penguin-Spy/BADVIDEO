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

maxFrameSize = 0  # Size of the largest frame
maxUncompressedChunk = 0  # Size of the largest uncompressed chunk
maxDelta = 0  # Maximum delta (distance between uncompressing data into it's own buffer)


def encodeFrame(frame):
    # Convert to PIL & encode as LLV
    color_coverted = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    img = Image.fromarray(color_coverted)

    img = img.convert('1', dither=False)  # convert image to 2 color black and white
    img = img.resize((320, 240))  # , box=(0, 0, width, height)

    pixels = img.load()  # this is not a list, nor is it list()'able
    width, height = img.size
    color = pixels[0, 0]

    # frameHeader = 0b10000000 * (color == 0)  # Starting color (1 = black)
    startColor = (color == 0)

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

    # Compress vertically
    # currentLineLength = -1  # Start at -1 so that we enter the first pixel at line length 0
    # color = pixels[0, 0]
    #
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
    # Horizontal    04        2,875      1063 bytes  -  this is the one that gets used because decoding vertical takes 3 years
    # Vertical      03       40,075      1492 bytes
    # Lowest of ↑   03       28,993      1063 bytes
    # Highest lol   04       14,066      1492 bytes
    # if (len(horizontalFrameBytes) < len(verticalFrameBytes)):
    frameBytes = horizontalFrameBytes
    lineCount = horizontalLineCount
    # else:
    #    frameBytes = verticalFrameBytes
    #    lineCount = verticalLineCount
    #    frameHeader += 0b01000000   # Compression direction (1 = vertical)

    # Frame Header & This is how the TI-84 stores 16 bit ints, so thats how we store it.
    # frameBytes = bytes([frameHeader]) + lineCount.to_bytes(2, 'little') + frameBytes

    print("Encoded frame", frameTotal, "with", lineCount, "lines.")

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
        0   # whoo python is amazing right?
    uncompressedFile = open("temp.bin", 'wb')
    uncompressedFile.write(uncompressedBytes)
    uncompressedFile.close()
    out = os.popen('@"tools/zx7.exe" temp.bin').read().split(" ")
    if (maxUncompressedChunk < int(out[9])):
        maxUncompressedChunk = int(out[9])
    # out[12]
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

    #header = 0b00100000 * (repeatedFrames > 0)
    #header = int(header) + (0b10000000 * color)
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
    print(f'[Note]: # of frames to encode was unspecified. Setting to default: "{framesToEncode}"')

# Read the video from specified path
inputVideo = cv2.VideoCapture(inputPath)
inputVideoTag = TinyTag.get(inputPath)

# title = "Bad Apple"
title = inputVideoTag.title
if (title == None):
    title = "unknown"
    print(f'[Warning]: Title is empty. Setting to default: "{title}"')
try:
    titleBytes = bytes(title, "ascii")  # Title
except Exception:
    title = "unknown"
    print(f'[Warning]: Title could not be encoded via ascii. Setting to default: "{title}"')

headerBytes = bytes("LLVH", "ascii")
headerBytes += bytes([0b10010001])   # Version (1.1 DEBUG)
headerBytes += bytes([0b00000000])   # Features (0 - Sound; 1-7 Reserved, should be zeros.)
headerBytes += bytes([int(inputVideo.get(cv2.CAP_PROP_FPS))])   # FPS
headerBytes += bytes([0x00])   # Off color (set manually)
headerBytes += bytes([0xB5])   # On color (set manually)
titleBytes = bytes(title, "ascii")   # get title
headerBytes += len(title).to_bytes(1, 'big')  # Title length
headerBytes += titleBytes            # Title

frameTotal = 0  # Overall total frames in the entire video
repeatedFrames = 0  # How many frames in a row were identical
fileIndex = 0  # Which file is this
oldFrameBytes = bytes()
oldColor = 0  # gross but eh

unCVideoBytes = bytes()  # Uncompressed video bytes
cVideoBytes = bytes()   # Compressed video bytes

while(True):
    # reading from frame
    ret, frame = inputVideo.read()

    if ret:
        color, frameBytes = encodeFrame(frame)  # Encode this frame

        if (frameBytes == oldFrameBytes and color == oldColor):
            repeatedFrames += 1
        elif (len(oldFrameBytes) != 0):
            unCVideoBytes += makeHeader(oldColor, 0, repeatedFrames, 0) + oldFrameBytes
            repeatedFrames = 0
            frameTotal += 1

        oldFrameBytes = frameBytes
        oldColor = color

        # no repeatedFrames counting
        #unCVideoBytes += makeHeader(color, 0, 0, 0) + frameBytes
        #frameTotal += 1

        if ((frameTotal % 8) == 0 and frameTotal != 0):  # Compress in batches of 8 frames
            cVideoBytes += compressBytes(unCVideoBytes)
            unCVideoBytes = bytes()

        if (len(frameBytes) > maxFrameSize):
            maxFrameSize = len(frameBytes)

        # previousFrameBytes = frameBytes  # Save last frame
        # frameHeader, frameBytes = encodeFrame(frame)  # Encode this frame

        # if (frameBytes == previousFrameBytes):
        #    repeatedFrames += 1
        # else:
        #    unCVideoBytes += bytes([frameHeader | 0b00100000]) + bytes([repeatedFrames]) + frameBytes
        #    repeatedFrames = 0
        #    frameTotal += 1

        # if ((frameTotal % 8) == 0):  # Compress in batches of 8 frames
        #    cVideoBytes += compressBytes(unCVideoBytes)
        #    unCVideoBytes = bytes()

        # if (len(frameBytes) > maxFrameSize):
        #    maxFrameSize = len(frameBytes)

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
    if(frameTotal > framesToEncode and not framesToEncode == 0):  # 0 == encode the whole video
        break

# Finish up loop operations that normally would wait for a condition to be true
# if (repeatedFrames > 0):
#    lastFrameHeader = (lastFrameBytes[0] & 0b00100000) + repeatedFrames.to_bytes(2, 'little')
#    uncompressedVideoBytes += lastFrameHeader + lastFrameBytes[1:]
#    frameTotal += 1
#    frameCount += 1
# if (len(uncompressedVideoBytes) > 0):
#    compressedVideoBytes += compressBytes(uncompressedVideoBytes)

headerBytes += (frameTotal - 1).to_bytes(2, 'little')

# TODO: parse output text of zx7.exe & save delta & max buffer size to headers/stdout

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

print(f"Converted {inputPath} to {outputPath}{varName}.8xp with {fileIndex + 1} files ({finalSize} bytes in total).\n" +
      f"The largest single frame was {maxFrameSize} lines ({maxFrameSize + 3} bytes).\n" +
      f"The largest uncompressed chunk was {maxUncompressedChunk} bytes, and the largest decompression delta is {maxDelta} bytes.")
