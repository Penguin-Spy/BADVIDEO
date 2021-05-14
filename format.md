## Line Length Video (.LLV) Format Specification
Version 1.1 DEBUG
Copyright Penguin_Spy 2021

|   Name   |  Offset  |  Length  |  Description
|----------|----------|----------|--------------
|  Version |     0    |  1 Byte  | Version of this LLV file. For this specification, should be `0b1.001.0000`, 1st bit is set if debug
| Features |     1    |  1 Byte  | Bitmask - Which features this file uses: 0 - Sound; 1-7 Reserved, should be zeros.
|    FPS   |     2    |  1 Byte  | Frames per Second to play the video in.
|    OFF   |     3    |  1 Byte  | Frames per Second to play the video in.
|    ON    |     4    |  1 Byte  | Frames per Second to play the video in.
|   Title  |     5    |  1 + str | Length-prepended string to display as the name of the video.
|  Frames  |  depends | #o Frames| List of frames. Frame format is described below.

### .LLV Frame Format
|   Name   |  Offset  |  Length  |  Description
|----------|----------|----------|--------------
|  Header  |     0    |  1 Byte  | Bitmask - how to render frame:
|          |          |          |  0 - Starting color; 0 = black, 1 = white
|          |          |          |  1 - Compression direction; 0 - horizontal, 1 = vertical; UNUSED/RESERVED & ignored, should always be 0
|          |          |          |  2 - Display length; 0 - display frame for one frame, 1 - next bytes include how many frames to repeat this frame
|          |          |          |  3 - Sound; 0 - no sound change, 1 - next bytes include tone/note to play. UNUSED/RESERVED & ignored, should always be 0
|          |          |          |  4-7: Reserved, should be 0.
|  Display |     1    |  1 Byte  | If display length bit set, number of frames to display this frame for. Not present otherwise.
|   Sound  |  depends |  1 Byte? | If sound bit set, tone/note to play. Not present otherwise. UNUSED/RESERVED, NEVER PRESENT
| #o lines |  depends |  2 Bytes | Number of lines in this frame.
|Frame Data|  depends | #o lines | List of distances between each color change. A value of 0xFF (255) does not swap the color, allowing contiguous sections of the same color longer than 255. Line lengths of exactly 255 are encoded as a 255 length line, followed by a 0 length line.
