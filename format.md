## Line Length Video (.LLV) Format Specification
Version 0.9 PREVIEW
Copyright Penguin_Spy 2021

|   Name   |  Offset  |  Length  |  Description
|----------|----------|----------|--------------
|  Version |     0    |  1 Byte  | Version of this LLV file. For this specification, should be (`0b00010000`)
| Features |     1    |  1 Byte  | Bitmask - Which features this file uses: 0 - Captions; 1 - Sound; 2-7 Reserved, should be zeros.
|  Runtime |     2    |  2 Bytes | Play time of the video in seconds. Not used in any render logic, purely for displaying in the menu.
|    FPS   |     4    |  1 Byte  | Frames per Second to play the video in.
|   Title  |     5    |  str + 1 | Null-terminated string to display as the name of the video.
| #o Cptns |  depends |  2 Bytes | Number of caption strings in the following list. Not included if captions were not specified in the Features bitmask
| Captions |  depends | str+1*ln | List of null-terminated caption strings, followed by x & y postion to display string at. Not included if captions were not specified in the Features bitmask.
|  Frames  |  depends | #o Frames| List of frames. Frame format is described below.

### .LLV Frame Format
|   Name   |  Offset  |  Length  |  Description
|----------|----------|----------|--------------
|  Header  |     0    |  1 Byte  | Bitmask - how to render frame:
|          |          |          |  0 - Starting color; 0 = black, 1 = white
|          |          |          |  1 - Compression direction; 0 - horizontal, 1 = vertical
|          |          |          |  2 - Display length; 0 - display frame for normal time (depends on FPS), 1 - next bytes include length of time to display this frame.
|          |          |          |  3 - Caption; 0 - no caption change, 1 - next bytes include caption string index to show.
|          |          |          |  4 - Sound; 0 - no sound change, 1 - next bytes include tone/note to play.
|          |          |          |  5-7: Reserved, should be 0.
|  Display |     1    |  1 Byte  | If display length bit set, number of frames to display this frame for. Not present otherwise.
|  Caption |  depends |  2 Bytes | If caption bit set, caption index to show. If value is 0xFFFF, clear any displayed captions. Not present otherwise.
|   Sound  |  depends |  1 Byte? | If sound bit set, tone/note to play. Not present otherwise.
| #o lines |  depends |  2 Bytes | Number of lines in this frame.
|Frame Data|  depends | #o lines | List of distances between each color change. A value of 0xFF (255) does not swap the color, allowing contiguous sections of the same color longer than 255. Line lengths of exactly 255 are encoded as a 255 length line, followed by a 0 length line.  
