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

    
typedef struct {
    char    *text;
    uint16_t x;
    uint8_t  y;
    uint8_t  color;
} caption_t;

typedef struct {
    uint8_t  header;
    uint8_t  displayTime;
    uint16_t captionIndex;
    uint8_t  sound;
    uint16_t lineCount;
    uint8_t  frameDataBuffer[255];
} frame_header_t;

typedef struct {
    char        fileName[8];
    uint8_t     version;
    uint8_t     features;
    uint8_t     fps;
    uint8_t     titleLength;
    char        *title;
    uint8_t     titleEnd;
    uint16_t    captionCount;
    caption_t   *captions;
    uint16_t    frameCount;
    frame_header_t   *frameBuffer;
} llv_header_t;


ti_var_t LLV_FILE;


int loadFrame() {
    uint8_t  frameHeader;
    uint16_t lineCount;
    uint16_t x;
    uint8_t  y;
    uint8_t  VERT_SCALE = 1;
    uint8_t  remainingWidth;
    uint8_t  frame[255];
    uint8_t  line;
    uint8_t  color = 0;
    uint8_t  tempColor;
    uint8_t  key = 0;
    uint16_t i;
    
    ti_Read(&frameHeader, 1, 1, LLV_FILE);
    ti_Read(&lineCount, 1, 2, LLV_FILE);
        
    // draw (320x240)
    x = 0;
    y = 0;
    color = 0x00;
    gfx_SetColor(0xFF);     // Setup color swapping
    if(frameHeader & 128) {  // If 1st bit set, start with white
        color = gfx_SetColor(color);
    }
    
    // DEBUG: clear screen before draw.
    gfx_FillScreen(74);
            // DEBUG: debug var display
            tempColor = gfx_SetColor(74);
            gfx_FillRectangle(0, 200, LCD_WIDTH, 40);
            gfx_SetTextXY(0, 200);
            gfx_PrintUInt(lineCount, 16);
            gfx_SetTextXY(0, 208);
            gfx_PrintUInt(i, 16);
            gfx_SetTextXY(0, 216);
            gfx_PrintUInt(frameHeader, 8);
            /*gfx_SetTextXY(0, 224);
            gfx_PrintUInt(i, 8);
            gfx_SetTextXY(0, 232);
            gfx_PrintInt(LLV_SIZE, 8);*/
            gfx_SetColor(tempColor);
    
    while(!(key = os_GetCSC()));
    
    if(key == sk_Clear || lineCount == 65535) {
        return 0;
    }
    
    for (i = 0; i < lineCount; i++) {
        remainingWidth = LCD_WIDTH - x;
        
        ti_Read(&frame, 1, 1, LLV_FILE);
        
        //line = frame[i];
        line = frame[0];
        
        if(remainingWidth < line) {
            // line for remainingWidth
            gfx_FillRectangle(x, y, remainingWidth, VERT_SCALE);
            
            // line for line - remainingWidth on next line
            gfx_FillRectangle(0, y + VERT_SCALE, line - remainingWidth, VERT_SCALE);
            
            // loop x and +1 to y
            x = line - remainingWidth;
            y += VERT_SCALE;
        } else {
            // line for line
            gfx_FillRectangle(x, y, line, VERT_SCALE);
            // increase x offset by length of line
            x += line;
        }
        
        if(line != 255) {   // Swap color, unless line length was exactly 255 (0xFF). This is to allow contiguous sections of the same color longer than 255.
            color = gfx_SetColor(color);    // To display a line of length 255, encode it as a 255 length line (0xFF) followed by a 0 length line (0x00).
        }
        
            
            // DEBUG: debug var display
            tempColor = gfx_SetColor(74);
            gfx_FillRectangle(0, 200, LCD_WIDTH, 40);
            gfx_SetTextXY(0, 200);
            gfx_PrintUInt(lineCount, 16);
            gfx_SetTextXY(0, 208);
            gfx_PrintUInt(i, 16);
            /*gfx_SetTextXY(0, 216);
            gfx_PrintUInt(line, 8);
            gfx_SetTextXY(0, 224);
            gfx_PrintUInt(i, 8);
            gfx_SetTextXY(0, 232);
            gfx_PrintInt(LLV_SIZE, 8);*/
            gfx_SetColor(tempColor);
            
            
    }
    
    return 0;
}


int main(void)
{
    llv_header_t LLV_header;
    frame_header_t frame_header;
    
    
    
    //uint8_t frame[] = {255,255,255,255,255,190,4,255,59,8,255,57,8,255,56,10,255,55,10,255,55,10,255,55,10,255,56,8,255,57,8,255,59,4,255,255,255,25,4,255,59,8,255,57,8,255,56,10,255,55,10,255,55,10,255,55,10,255,56,8,255,57,8,255,59,4,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,20,4,255,59,8,255,56,9,255,55,11,255,53,12,255,53,12,255,52,13,255,51,13,255,51,14,84,4,217,14,83,8,214,15,83,8,213,14,84,10,211,15,84,10,210,15,85,10,209,15,86,10,208,15,87,10,207,15,88,10,206,15,89,11,204,15,90,11,204,14,91,12,202,14,93,12,200,14,94,12,199,14,96,12,197,14,97,13,195,14,99,13,193,14,100,14,191,15,101,14,189,15,103,14,187,15,104,16,184,15,106,16,182,15,108,16,180,15,110,16,179,14,112,16,177,14,114,17,174,14,116,17,172,14,118,17,170,14,121,16,168,14,123,17,165,14,125,17,163,15,126,17,161,15,128,18,158,15,131,17,156,15,133,17,154,15,135,18,152,14,137,18,150,14,140,17,148,14,142,18,145,14,144,18,143,14,147,18,140,14,149,18,138,14,151,19,135,15,153,18,133,15,155,19,130,15,157,19,128,15,160,19,124,16,162,20,121,16,165,19,119,16,167,20,116,16,170,20,113,16,172,21,109,17,175,21,106,17,177,22,102,18,180,22,99,17,184,23,94,18,186,25,90,18,189,25,86,19,192,26,81,20,195,26,77,20,199,27,73,20,202,28,67,21,206,29,61,23,209,30,56,23,213,32,49,25,217,34,41,26,222,36,32,28,226,43,18,31,231,88,234,84,239,79,244,73,250,67,255,1,62,255,7,55,255,15,47,255,23,38,255,36,24,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};
    //uint8_t len = sizeof(frame);
    
    char fileNames[][10] = {"_______0", "_______1", "_______2", "_______3", "_______4", "_______5", "_______6", "_______7", "_______8", "_______9"};
    
    uint16_t LLV_SIZE;
    
    uint8_t key = 0;
    uint8_t tempColor = 0;
    char *var_name;
    uint8_t *search_pos;
    uint8_t numFound;
    uint8_t i;
    
    
    
    gfx_Begin();            // Initalize graphics
    //gfx_SetDrawBuffer();    // Enable buffering (because the screen is fully redrawn each frame)
    
    
    
    
    
select:
    // Close any files that may be open already
    ti_CloseAll();
    
    gfx_SetColor(0xFF);
    gfx_FillScreen(74);
    
    search_pos = NULL;
    numFound = 0;
    
    // Pick file
    while((var_name = ti_DetectVar(&search_pos, "LLV", TI_APPVAR_TYPE)) != NULL) {
        LLV_FILE = ti_Open(var_name, "r");
        
        // Read data
        strcpy(LLV_header.fileName, var_name);  // File name for printing
        strcpy(fileNames[numFound], var_name);      // File name saved for opening
        ti_Seek(3, SEEK_SET, LLV_FILE);         // Seek past "LLV" header
        ti_Read(&LLV_header.fileName+9, 4, 1, LLV_FILE);    // Offsets 0-3
        ti_Read(&LLV_header.title, 1, LLV_header.titleLength, LLV_FILE);    // Title String
        LLV_header.titleEnd = 0x00;             // Not the best way to do this, but whatever.
        if((LLV_header.features) & 128) {                   // Captions exist
            ti_Read(&LLV_header.captionCount, 2, 1, LLV_FILE);      // Number of captions
            ti_Seek(LLV_header.captionCount, SEEK_CUR, LLV_FILE);   // Seek pask the captions list
        }
        ti_Read(&LLV_header.frameCount, 2, 1, LLV_FILE);            // Number of frames
        
        // Print in list
        gfx_SetTextXY(10, numFound*8);
        gfx_PrintString(LLV_header.fileName);
        /*gfx_SetTextXY(10+64+10, numFound*8);
        gfx_PrintUInt(LLV_header.version, 8);
        gfx_SetTextXY(10+64+10+64+10, numFound*8);
        gfx_PrintUInt(LLV_header.features, 8);
        gfx_SetTextXY(10+64+10+64+10+64+10, numFound*8);
        gfx_PrintUInt(LLV_header.fps, 3);*/
        gfx_SetTextXY(10+64+10, numFound*8);
        //gfx_PrintString(LLV_header.title);
        
        // Close file.
        ti_CloseAll();
        
        numFound++;
        if(numFound > 10) break;
    }
    //gfx_SwapDraw();
    
    for(i = 0; i < numFound; i++) {
    }
    
    key = 0;
    i = 0;
    while(key != sk_Enter && key != sk_Clear) {
        switch(key) {
            case sk_Up:
                if(i > 0) i--;
                break;
            case sk_Down:
                if(i < numFound-1) i++;
                break;
            // mode for more info?
        }
        
        // Draw selection carrot
        tempColor = gfx_SetColor(74);
        gfx_FillRectangle(0, 0, 10, 8*numFound);
        gfx_SetColor(tempColor);
        gfx_SetTextXY(0, i*8);
        gfx_PrintString(">");
        //gfx_SwapDraw();
        
        while(!(key = os_GetCSC()));
    }
    
    if(key == sk_Clear) {
        gfx_End();              // End graphics drawing
        return 0;
    }
    

    // Open the selected file.
    LLV_FILE = ti_Open("BADAPPLE", "r");
    
    //ti_Read(&frame, 1, sizeof(frame), LLV_FILE);
    
    LLV_SIZE = ti_GetSize(LLV_FILE);
    
    // Seek to the beginning of the frame data
    ti_Seek(3 + 7 + LLV_header.titleLength + 2, SEEK_SET, LLV_FILE);
    gfx_SetTextXY(0, 0);
    gfx_PrintUInt(LLV_header.frameCount, 8);
    //gfx_SwapDraw();
    
    while(!(key = os_GetCSC()));
    
    for(i = 0; i < LLV_header.frameCount; i++) {
        
        loadFrame();
        //gfx_SwapDraw();
        
        //if(key == sk_Clear) break;      // DEBUG: quit on clear pressed
    }
    
    key = 0;
    goto select;
}
