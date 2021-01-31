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

//#include <src/decodeFrame.h>
//#include <src/decodeFrame.asm>

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

//extern void decodeVertical(uint8_t* frameBuffer, uint16_t lineCount);

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
    uint24_t hl_save;
    uint16_t x;
    uint16_t y;
    uint8_t  VERT_SCALE = 1;
    uint8_t  HORIZ_SCALE = 1;
    uint16_t FRAME_BYTE_BUFFER_SIZE = 2924;//2282;
    uint8_t  frame[2924]; // help how do i make the buffer size set from the var above, the one above doesn't change
    uint16_t remainingLength;
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
    fileNames[h][6] = '0';
    fileNames[h][7] = '0';

    // Seek to the beginning of the frame data
    //      LLVH, Header, Title string,       frameTotal     frameCount
    ti_Seek(4 + 4 + LLVH_header.titleLength + 2, SEEK_SET, LLV_FILE);

    //gfx_SetDrawBuffer(); // Enable buffering (because the screen is fully redrawn each frame)

    goto skip_open;
open:
    // Open the selected file.

    //TODO: read filenames properly
    LLV_FILE = ti_Open(fileNames[h], "r");
    fileNames[h][7]++;
    if (fileNames[h][7] == ':') {
        fileNames[h][6]++;
        fileNames[h][7] = '0';
    }

    //LLV_SIZE = ti_GetSize(LLV_FILE);

skip_open:
    ti_Read(&frameCount, 1, 1, LLV_FILE);

    /*gfx_FillScreen(74);
    gfx_SetTextXY(0, 0);
    gfx_PrintUInt(LLVH_header.frameTotal, 8);
    gfx_SetTextXY(0, 8);
    gfx_PrintUInt(frameCount, 8);
    gfx_PrintStringXY(fileNames[h], 0, 16);
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

    for (i = 0; i < frameCount; i++) {

        gfx_FillScreen(47);

        ti_Read(&frameHeader, 1, 1, LLV_FILE);
        ti_Read(&lineCount, 1, 2, LLV_FILE);

        // draw (320x240)
        //x = 0xD40000;       // start of vRam, set here because it's used in later ASM and easier to just assign now
        hl_save = 0xD40000;
        x = 0;
        y = 0;
        color = 0xFF;
        gfx_SetColor(0x00); // Setup color swapping

        if (frameHeader & 128) { // If 1st bit set, start with white
            color = gfx_SetColor(color);
        }


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
        //while (!(key = os_GetCSC()));

        if (key == sk_Clear || lineCount > FRAME_BYTE_BUFFER_SIZE)
        {
            return 0;
        }

        //remainingFrameBytes = 0;
        ti_Read(&frame, lineCount, 1, LLV_FILE);


        // We have the whole loop inside the for loop (which results in a lot of code duplication)
        // because that way we only check the compression direction once before we loop,
        // instead of every single line. If there's a better way to do this, I would like to know.

        //if (!(frameHeader & 64)) {

            //decodeVertical(&frame, lineCount);


            //for (j = 0; j < lineCount; j++) {
                //Initialize registers

        for (j = 0; j < lineCount; j++) {

            line = frame[j];

            asm("decodeVertical:\n"

                // init (check line length & stuff)

                // load _line into b
                "   ld      b, (IX + -24)\n"  // line
                    // if b == 0, cancel
                "   xor     a, a\n"
                "   or      a, b\n"
                "   jr      z, skip_inline\n"

                // load _color into a
                "   ld      a, (IX + -18)\n"   // color
                    // load 320 (offset to next pixel row) into c
                //"   ld      c, 320 / 2\n"     // UNUSED BECAUSE LATER CODE CHANGED

                // load _hl_save into HL
                "   ld      hl, (IX + -38)\n"  // hl_save

                "   jr      start\n"

                "rewind:\n"
                "   ld      de, 76799\n"
                "   sbc     hl, de\n"
                "   inc     (IX + -27)\n"       // increment X

                "start:\n"
                // Calculate the "out of bounds address" that hl would encounter
                "   push    hl\n"               // pushed & popped because of instruction/register restrictions
                "   ld      hl, 0xD65800\n"
                "   ld      de, 0x000000\n"     // Clear DEU
                "   ld      d, (IX + -26)\n"    // load x
                "   ld      e, (IX + -27)\n"    // load x
                "   add     hl, de\n"           // add x
                "   ld      de, hl\n"
                "   pop     hl\n"

                "loop:\n"
                "   ld      (hl), a\n"
                "   push    bc\n"           // add 320 to hl
                "   ld      bc, 320\n"
                "   add     hl, bc\n"
                "   pop     bc\n"
                // compare hl to previously calculated "out of bounds address"
                "   push    hl\n"
                "   sbc     hl, de\n"
                "   pop     hl\n"
                "   jr      z, rewind\n"    // If hl == de (oob address), jump back up to rewind
                "   djnz    loop\n"




                // cleanup
                    // save hl into hl_save
                "   ld      (IX + -38), hl\n"  // hl_save
                "skip_inline:\n"
            );

            /*asm(
                "LD      bc, 0x000000\n"        // zero out BCU
                "LD      C, (IX+-24)\n" // line // Load line length into C
                "loop:\n"
                // Check if line == 0, if so exit loop
                "AND     A, 0\n"                // Clear the Accumulator
                "SUB     A, C\n"                // Is line == 0 ?
                "JR      Z, exitLoop_V\n"       // Jump past this code

                // Copy color to screen & go to next line
                "LD      HL, (IX+-38)\n" // x   // Load the vRam offset into HL (saved in uint8_t x's memory location because simple)
                "LD      A, (IX+-18)\n" //color // Color
                "LD      (HL), A\n"
                "LD      DE, 320\n"             // Screen Width of one line(to go to the pixel below HL)
                "ADD     HL, DE\n"

                // If we've gone past vRam, rewind back to the top row, 1 column right
                "LD      DE, 0xD56801\n"        // End of vRam + 1 (so that we only need one conditional jump)
                "PUSH    HL\n"                  // Save HL b4 we subtract
                "SBC     HL, DE\n"
                "POP     HL\n"                  // Load HL back here (before jump because stack leak no good)
                "JR      M, skipRewind\n"

                // Rewind back to the top row, 1 column right
                "LD      DE, 76799\n"
                "SBC     HL, DE\n"              // vRam size - 1, to rewind to the top of the next column

                "skipRewind:\n"
                "LD    (IX+-38), HL\n" // x     // Save hl to x
                "DEC     BC\n"                  // Decrement the number of pixels left in this line
                "JR      loop\n"                // Loop

                "exitLoop_V:\n"
            );*/
        }


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
        /*line = frame[j];


        remainingLength = LCD_WIDTH - x;

        if (remainingLength < line) {
            // line for remainingLength
            //gfx_FillRectangle(x, y, remainingLength, VERT_SCALE);
            gfx_HorizLine_NoClip(x, y, remainingLength);

            // line for line - remainingLength on next line
            //gfx_FillRectangle(0, y + VERT_SCALE, line - remainingLength, VERT_SCALE);
            gfx_HorizLine_NoClip(0, y + VERT_SCALE, line - remainingLength);

            // loop x and +1 to y
            x = line - remainingLength;
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
        }*/

        // DEBUG: debug var display
        tempColor = gfx_SetColor(74);
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
        gfx_SetColor(tempColor);

        while (!os_GetCSC());

        //remainingFrameBytes--;
    //}
    /*}
    else {*/

    /*for (j = 0; j < lineCount; j++) {

        line = frame[j];

        // Inline asm code to draw a single line to vRam quickly.
        // org D1B32A (unless any C code above this gets modified) IT HAS BEEN NOW
        asm("AND   A, 0\n"                  // AND A with 0 to set A to 0, because you can't just load a value directly into it :/
            "SUB   A, (IX+-22)\n" // line   // Check if the line length is 0
            "JR    Z, skip_ASM\n"           // If zero, jump past all this inline asm (we don't want to draw a line of 0, that underflows BC & fills past vRam)
            "LD    HL, (IX+-38)\n" // x     // starting location (location of vram + x)
            //"LD  bc, (IX+-24)\n"//320 * 240 * 2 - 2\n" // number of pixels to copy
            "LD    bc, 0x000000\n"          // zero out BCU
            "LD    c, (IX+-22)\n" // line   // copy line byte
            "PUSH  hl\n"                    // Copy hl to de
            "POP   de\n"
            // Write initial 2-byte value
            "LD    A,(IX+-10)\n"  // color  // Load color into screen pixel
            "LD    (HL),A\n"
            "INC   hl\n"
            /*"LD(hl), 69\n"                // load 0x69 into the address pointed to by hl (vram)
            "INC   hl\n"                    // increment the address that hl points to
            "LD(hl), 0\n"                   // load 0x00 into the address pointed to by hl
            "INC   hl\n"                    // increment that adress*//*
            "EX    de, hl\n"                // swap de & hl
            //Copy everything all at once. Interrupts may trigger while this instruction is processing.
            //"DI\n"
            "LDIR\n"                        // fancy-shmancy instruction that loops until bc == 0x00, copying (hl) to (de)
            "LD    (IX+-38), HL\n" // x
            "skip_ASM:\n"
            //"EI\n"
            "LD  A, (IX + -22)\n"           // Restore A register from before this code (is set by other stuff idk but hope this works)
        );
        /*line = frame[j];

        remainingLength = LCD_HEIGHT - y;

        if (remainingLength < line) {
            gfx_VertLine_NoClip(x, y, remainingLength);
            if (line - remainingLength > LCD_HEIGHT) {
                gfx_VertLine_NoClip(x + HORIZ_SCALE, 0, LCD_HEIGHT);
                gfx_VertLine_NoClip(x + 2 * HORIZ_SCALE, 0, line - remainingLength - LCD_HEIGHT);
                y = line - remainingLength - LCD_HEIGHT;
                x += HORIZ_SCALE;
            }
            else {
                gfx_VertLine_NoClip(x + HORIZ_SCALE, 0, line - remainingLength);
                y = line - remainingLength;
            }

            // +1 to x
            x += HORIZ_SCALE;
        }
        else {
            gfx_VertLine_NoClip(x, y, line);

            // increase y offset by length of line
            y += line;
        }*//*

        if (line != 255) {              // Swap color unless 0xFF
            color = gfx_SetColor(color);
        }
    }*/
    //}

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

