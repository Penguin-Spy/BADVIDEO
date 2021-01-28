/*
 *--------------------------------------
 * Program Name:    Bad Apple for the TI-84+ CE
 * Author:          Penguin_Spy
 * License:         Use, but don't steal my code thx; the original video is by あにら (https://www.nicovideo.jp/watch/sm8628149); original music is by colors (https://www.nicovideo.jp/watch/sm2077177)
 * Description:     Renders a series of frames stored as the length between each color flip, sorta like gen 1 pokemon sprites (i think).
 *--------------------------------------
*/

/* Includes */
#include <string.h>
#include <tice.h>
#include <graphx.h>
#include <fileioc.h>
#include <stdlib.h>

/*
|  Version |     0    |  1 Byte  | Version of this LLV file. For this specification, should be (`0b00001001`)
| Features |     1    |  1 Byte  | Bitmask - Which features this file uses: 0 - Captions; 1 - Sound; 2-7 Reserved, should be zeros.
|    FPS   |     2    |  1 Byte  | Frames per Second to play the video in.
|TitleLngth|     3    |  1 Byte  | Length of the title string
|   Title  |     4    |  strln+1 | String to display as the name of the video.
| #o Cptns |  depends |  2 Bytes | Number of caption strings in the following list. Not included if captions were not specified in the Features bitmask
| Captions |  depends | str+4*ln | List of null-terminated caption strings, followed by x & y postion to display string at. Not included if captions were not specified in the Features bitmask.
| #o Frames|  depends |  2 Bytes | Number of frames in the following list.
|  Frames  |  depends | #o Frames| List of frames. Frame format is described below.
*/
/*
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
*/

typedef struct
{
    char* text;
    uint16_t x;
    uint8_t  y;
    uint8_t  color;
} caption_t;

typedef struct
{
    uint8_t  header;
    uint8_t  displayTime;
    uint16_t captionIndex;
    uint8_t  sound;
    uint16_t lineCount;
} frame_header_t;

typedef struct
{
    char fileName[8];
    uint8_t    version;
    uint8_t    features;
    uint8_t    fps;
    uint8_t    titleLength;
    char* title;
    uint16_t   captionCount;
    caption_t* captions;
    uint16_t   frameTotal;
} llvh_header_t;

ti_var_t LLV_FILE;

int main(void)
{
    llvh_header_t LLVH_header;
    frame_header_t frame_header;

    //uint8_t frame[] = {255,255,255,255,255,190,4,255,59,8,255,57,8,255,56,10,255,55,10,255,55,10,255,55,10,255,56,8,255,57,8,255,59,4,255,255,255,25,4,255,59,8,255,57,8,255,56,10,255,55,10,255,55,10,255,55,10,255,56,8,255,57,8,255,59,4,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,20,4,255,59,8,255,56,9,255,55,11,255,53,12,255,53,12,255,52,13,255,51,13,255,51,14,84,4,217,14,83,8,214,15,83,8,213,14,84,10,211,15,84,10,210,15,85,10,209,15,86,10,208,15,87,10,207,15,88,10,206,15,89,11,204,15,90,11,204,14,91,12,202,14,93,12,200,14,94,12,199,14,96,12,197,14,97,13,195,14,99,13,193,14,100,14,191,15,101,14,189,15,103,14,187,15,104,16,184,15,106,16,182,15,108,16,180,15,110,16,179,14,112,16,177,14,114,17,174,14,116,17,172,14,118,17,170,14,121,16,168,14,123,17,165,14,125,17,163,15,126,17,161,15,128,18,158,15,131,17,156,15,133,17,154,15,135,18,152,14,137,18,150,14,140,17,148,14,142,18,145,14,144,18,143,14,147,18,140,14,149,18,138,14,151,19,135,15,153,18,133,15,155,19,130,15,157,19,128,15,160,19,124,16,162,20,121,16,165,19,119,16,167,20,116,16,170,20,113,16,172,21,109,17,175,21,106,17,177,22,102,18,180,22,99,17,184,23,94,18,186,25,90,18,189,25,86,19,192,26,81,20,195,26,77,20,199,27,73,20,202,28,67,21,206,29,61,23,209,30,56,23,213,32,49,25,217,34,41,26,222,36,32,28,226,43,18,31,231,88,234,84,239,79,244,73,250,67,255,1,62,255,7,55,255,15,47,255,23,38,255,36,24,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};
    //uint8_t len = sizeof(frame);

    char fileNames[][10] = { "_______0", "_______1", "_______2", "_______3", "_______4", "_______5", "_______6", "_______7", "_______8", "_______9" };

    uint16_t LLV_SIZE;

    uint8_t  key = 0;
    uint8_t  tempColor = 0;
    char* var_name;
    uint8_t* search_pos;
    uint8_t  numFound;
    uint16_t h;
    uint16_t i;
    uint16_t j;


    uint8_t  frameHeader;
    uint16_t lineCount;
    uint16_t x;
    uint8_t  y;
    uint8_t  VERT_SCALE = 1;
    uint8_t  FRAME_BYTE_BUFFER_SIZE = 1024;
    uint8_t  frame[1024]; // help how do i make the buffer size set from the var above, the one above doesn't change
    uint16_t remainingWidth;
    uint8_t  remainingFrameBytes;
    uint8_t  line;
    uint8_t  color = 0;

    uint16_t remainingFrames;
    uint8_t  frameCount;        // More than 255 frames cannot physically fit into 64ki (min frame size is 305, so 214.8 frames can fit)
                                //(this likely [hopefully] will change when compression is implemented)

    gfx_Begin(); // Initalize graphics

select:
    // Close any files that may be open already
    ti_CloseAll();

    gfx_SetColor(0xFF);
    gfx_FillScreen(74);

    search_pos = NULL;
    numFound = 0;

    // Pick file
    while ((var_name = ti_DetectVar(&search_pos, "LLVH", TI_APPVAR_TYPE)) != NULL) {
        LLV_FILE = ti_Open(var_name, "r");

        // Read data
        strcpy(LLVH_header.fileName, var_name);             // File name for printing
        strcpy(fileNames[numFound], var_name);             // File name saved for opening
        ti_Seek(4, SEEK_SET, LLV_FILE);                    // Seek past "LLVH" header
        ti_Read(&LLVH_header.fileName + 8, 4, 1, LLV_FILE); // Offsets 0-3
        LLVH_header.title = (char*)ti_MallocString(LLVH_header.titleLength);
        ti_Read(LLVH_header.title, 1, LLVH_header.titleLength, LLV_FILE); // Title String
        if ((LLVH_header.features) & 128)
        {                                                         // Captions exist
            ti_Read(&LLVH_header.captionCount, 2, 1, LLV_FILE);    // Number of captions
            ti_Seek(LLVH_header.captionCount, SEEK_CUR, LLV_FILE); // Seek pask the captions list
        }
        ti_Read(&LLVH_header.frameTotal, 1, 2, LLV_FILE); // Number of frames

        // Print in list
        gfx_SetTextXY(10, numFound * 8);
        gfx_PrintString(LLVH_header.fileName);
        /*gfx_SetTextXY(10+64+10, numFound*8);
        gfx_PrintUInt(LLVH_header.version, 8);
        gfx_SetTextXY(10+64+10+64+10, numFound*8);
        gfx_PrintUInt(LLVH_header.features, 8);
        gfx_SetTextXY(10+64+10+64+10+64+10, numFound*8);
        gfx_PrintUInt(LLVH_header.fps, 3);*/
        gfx_SetTextXY(10 + 64 + 10, numFound * 8);
        gfx_PrintString(LLVH_header.title);
        /*gfx_SetTextXY(10, 8);
        gfx_PrintUInt(LLVH_header.version, 8);
        gfx_SetTextXY(10, 16);
        gfx_PrintUInt(LLVH_header.features, 8);
        gfx_SetTextXY(10, 24);
        gfx_PrintUInt(LLVH_header.fps, 8);
        gfx_SetTextXY(10, 32);
        gfx_PrintUInt(LLVH_header.titleLength, 8);
        while (!(key = os_GetCSC()))*/

        // Close file.
        ti_CloseAll();

        numFound++;
        if (numFound > 10)  // We only have enough space for 10 names in the list (TODO: expand list to store a few more, and implement scrolling/whatever through files [search more when scrolling])
            break;
    }

    key = 0;
    if (numFound == 0) {
        gfx_SetTextXY(10, 0);
        gfx_PrintString("No videos found.");
        while (!(key = os_GetCSC()));
        key = sk_Clear;
    }

    h = 0;
    while (key != sk_Enter && key != sk_Clear)
    {
        switch (key)
        {
        case sk_Up:
            if (h > 0)
                h--;
            break;
        case sk_Down:
            if (h < numFound - 1)
                h++;
            break;
            // `mode` for more info?
        }

        // Draw selection carrot
        tempColor = gfx_SetColor(74);
        gfx_FillRectangle(0, 0, 10, 8 * numFound);
        gfx_SetColor(tempColor);
        gfx_SetTextXY(0, h * 8);
        gfx_PrintString(">");

        while (!(key = os_GetCSC()));
    }

    if (key == sk_Clear)
    {
        gfx_End(); // End graphics drawing
        return 0;
    }

    remainingFrames = LLVH_header.frameTotal;

    //TODO: read filenames properly
    LLV_FILE = ti_Open(fileNames[h], "r");

    // Seek to the beginning of the frame data
    //      LLVH, Header, Title string,       frameTotal     frameCount
    ti_Seek(4 + 4 + LLVH_header.titleLength + 2, SEEK_SET, LLV_FILE);

    goto skip_open;
open:
    // Open the selected file.

    //TODO: read filenames properly
    LLV_FILE = ti_Open(fileNames[h], "r");

    //LLV_SIZE = ti_GetSize(LLV_FILE);

skip_open:
    ti_Read(&frameCount, 1, 1, LLV_FILE);

    gfx_FillScreen(74);
    gfx_SetTextXY(0, 0);
    gfx_PrintUInt(LLVH_header.frameTotal, 8);
    gfx_SetTextXY(0, 8);
    gfx_PrintUInt(frameCount, 8);
    gfx_PrintStringXY(fileNames[h], 0, 16);
    fileNames[h][7]++;
    while (!(key = os_GetCSC()));

    /*while(key != sk_Clear) {

            ti_Read(&frame, 1, 1, LLV_FILE);

            //line = frame[i];
            line = frame[0];

        gfx_FillScreen(74);
        gfx_SetTextXY(0, 0);
        gfx_PrintUInt(line, 8);



        while(!(key = os_GetCSC()));
    }*/

    //gfx_SetDrawBuffer(); // Enable buffering (because the screen is fully redrawn each frame)

    for (i = 0; i < frameCount; i++) {

        ti_Read(&frameHeader, 1, 1, LLV_FILE);
        ti_Read(&lineCount, 1, 2, LLV_FILE);

        // draw (320x240)
        x = 0;
        y = 0;
        color = 0x00;
        gfx_SetColor(0xFF); // Setup color swapping
        if (frameHeader & 128)
        { // If 1st bit set, start with white
            color = gfx_SetColor(color);
        }

        //while(!(key = os_GetCSC()));

        // DEBUG: clear screen before draw.
        //gfx_FillScreen(74);
        // DEBUG: debug var display
        /*tempColor = gfx_SetColor(74);
            gfx_FillRectangle(0, 200, 64, 40);
            gfx_SetTextXY(0, 200);
            gfx_PrintUInt(lineCount, 16);
            gfx_SetTextXY(0, 208);
            gfx_PrintUInt(j, 16);
            gfx_SetTextXY(0, 216);
            gfx_PrintUInt(frameHeader, 8);
            /*gfx_SetTextXY(0, 224);
            gfx_PrintUInt(i, 8);
            gfx_SetTextXY(0, 232);
            gfx_PrintInt(LLV_SIZE, 8);*/
            //gfx_SetColor(tempColor);

        if (key == sk_Clear || lineCount > 1000)
        {
            return 0;
        }

        remainingFrameBytes = 0;
        ti_Read(&frame, 1, lineCount, LLV_FILE);

        for (j = 0; j < lineCount; j++) {

            // Load more data if nessecary
            /*if(remainingFrameBytes < 1) {

                // Calculate how many bytes to load: Frame sizes will not fit into multiples of the frame byte buffer size, and we don't want to read data from the next frame because that messes up the seek cursor pos
                uint16_t bytesToLoad = lineCount - j;
                if(bytesToLoad > FRAME_BYTE_BUFFER_SIZE) {  // Clamp to size of byte buffer
                    bytesToLoad = FRAME_BYTE_BUFFER_SIZE;
                }

                ti_Read(&frame, FRAME_BYTE_BUFFER_SIZE, 1, LLV_FILE);
                remainingFrameBytes = bytesToLoad;  // Keep track of how many bytes are loaded.
            }*/

            //line = frame[j];
            line = frame[j];

            remainingWidth = LCD_WIDTH - x;

            if (remainingWidth < line) {
                // line for remainingWidth
                //gfx_FillRectangle(x, y, remainingWidth, VERT_SCALE);
                gfx_HorizLine_NoClip(x, y, remainingWidth);

                // line for line - remainingWidth on next line
                //gfx_FillRectangle(0, y + VERT_SCALE, line - remainingWidth, VERT_SCALE);
                gfx_HorizLine_NoClip(0, y + VERT_SCALE, line - remainingWidth);

                // loop x and +1 to y
                x = line - remainingWidth;
                y += VERT_SCALE;
            }
            else {
                // line for line
                //gfx_FillRectangle(x, y, line, VERT_SCALE);
                gfx_HorizLine_NoClip(x, y, line);

                // increase x offset by length of line
                x += line;
            }

            if (line != 255) {                                // Swap color, unless line length was exactly 255 (0xFF). This is to allow contiguous sections of the same color longer than 255.
                color = gfx_SetColor(color); // To display a line of length 255, encode it as a 255 length line (0xFF) followed by a 0 length line (0x00).
            }

            // DEBUG: debug var display
            /*tempColor = gfx_SetColor(74);
            gfx_FillRectangle(0, 200, 64, 40);
            gfx_SetTextXY(0, 200);
            gfx_PrintUInt(lineCount, 16);
            gfx_SetTextXY(0, 208);
            gfx_PrintUInt(j, 16);
            gfx_SetTextXY(0, 216);
            gfx_PrintUInt(line, 8);
            gfx_SetTextXY(0, 224);
            gfx_PrintUInt(x, 8);
            gfx_SetTextXY(0, 232);
            gfx_PrintInt(y, 8);
            gfx_SetColor(tempColor);*/

            remainingFrameBytes--;
        }

        /*tempColor = gfx_SetColor(74);
        gfx_FillRectangle(0, 200, 64, 40);
        gfx_SetTextXY(0, 200);
        gfx_PrintString("for loop complete");
        gfx_SetColor(tempColor);*/
        //gfx_SwapDraw();

        //if(key == sk_Clear) break;      // DEBUG: quit on clear pressed
    }

    remainingFrames -= frameCount;  // Subtract the # of frames we just displayed from the total number of frames left

    if (remainingFrames > 0) {  // Frames left to read
        ti_CloseAll();
        goto open;
    }


    key = 0;
    //gfx_SetDrawScreen();
    goto select;
}
