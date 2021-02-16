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
#include <compression.h>
#include <C:/CEdev/include/stdint.h>
#include <C:/CEdev/BADVIDEO/src/decompression_test.c>

//#include <src/renderFrame.h>
//#include <src/renderFrame.asm>

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

uint8_t  color = 0x00;
uint16_t lineCount;
uint24_t hlSave;



void renderFrame(uint8_t frame[], uint16_t lineCount) {
    uint8_t line;
    uint16_t j;
    for (j = 0; j < lineCount; j++) {

        line = frame[j];

        // Inline asm code to draw a single line to vRam quickly.
        asm("xor   a, a\n"                  // clear A
            "sub   a, (IX-3)\n" // line     // Check if the line length is 0
            "jr    z, skip_ASM\n"           // If zero, jump past all this inline asm (we don't want to draw a line of 0, that underflows BC & fills past vRam)
            "ld    hl, (_hlSave)\n" // hlSave    // starting location (location of vram + x)
            //"LD  bc, (IX+-24)\n"//320 * 240 * 2 - 2\n" // number of pixels to copy
            "ld    bc, 0x000000\n"          // zero out BCU
            "ld    c, (IX-3)\n" // line     // copy line byte
            "push  hl\n"                    // Copy hl to de
            "pop   de\n"
            // Write initial 2-byte value
            "ld    a,(_color)\n"  // color  // Load color into screen pixel
            "ld    (hl),a\n"
            "inc   hl\n"
            /*"LD(hl), 69\n"                  // load 0x69 into the address pointed to by hl (vram)
            "INC   hl\n"                    // increment the address that hl points to
            "LD(hl), 0\n"                   // load 0x00 into the address pointed to by hl
            "INC   hl\n"                    // increment that adress*/
            "ex    de, hl\n"                // swap de & hl
            //Copy everything all at once. Interrupts may trigger while this instruction is processing.
            //"DI\n"
            "ldir\n"                        // fancy-shmancy instruction that loops until bc == 0x00, copying (hl) to (de) and incrementing both
            "ld    (_hlSave), hl\n" // x
            "skip_ASM:\n"
            //"EI\n"
            "ld  a, (IX-3)\n"               // Restore A register from before this code (is used b4 by getting the line, and after by the if statement)
        );

        if (line != 255) {               // Swap color, unless line length was exactly 255 (0xFF). This is to allow contiguous sections of the same color longer than 255.
            color = gfx_SetColor(color); // To display a line of length 255, encode it as a 255 length line (0xFF) followed by a 0 length line (0x00).
        }


        /*asm("decodeVertical:\n"
            // init (check line length & stuff)

            // load _line into b
            "   ld      b, (IX + -22)\n"  // line
                // if b == 0, cancel
            "   xor     a, a\n"
            "   or      a, b\n"
            "   jr      z, skip_inline\n"

            // load _color into a
            "   ld      a, (IX + -10)\n"   // color

            // load _hl_save into HL
            "   ld      hl, (IX + -38)\n"   // hl_save

            "   jr      start\n"

            "rewind:\n"
            "   ld      de, (IX + -27)\n"   // increment X
            "   inc     de\n"
            "   ld      (IX + -27), de\n"
            //"   inc     (IX + -10)\n"       // increment color
            "   ld      de, 76799\n"
            "   sbc     hl, de\n"

            "start:\n"
            // Calculate the "out of bounds address" that hl would encounter
            "   push    hl\n"               // pushed & popped because of instruction/register restrictions
            "   ld      hl, 0xD52C00\n"     // end of vRam
            "   ld      de, 0x000000\n"     // Clear DEU
            "   ld      d, (IX + -26)\n"    // load x
            "   ld      e, (IX + -27)\n"    // load x
            "   add     hl, de\n"           // add x
            "   ld      de, hl\n"
            "   pop     hl\n"

            "loop:\n"
            "   ld      (hl), a\n"
            "   push    bc\n"               // add 320 to hl
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
            "   ld      a, (IX + -22)"      // load line into a (used by below if statement)
        );*/
    }
}


int main(void) {
    llvh_header_t LLVH_header;
    frame_header_t frame_header;

    char fileNames[][10] = { "_______0", "_______1", "_______2", "_______3", "_______4", "_______5", "_______6", "_______7", "_______8", "_______9" };

    uint16_t LLV_SIZE;

    uint8_t  frameHeader;
    uint8_t  key = 0;
    uint8_t  tempColor = 0;
    char* var_name;
    uint8_t* search_pos;
    uint8_t  numFound;
    uint16_t h;
    uint16_t i;


    uint16_t y;
    uint8_t  VERT_SCALE = 1;
    uint8_t  HORIZ_SCALE = 1;
    uint8_t  chunkDelta = 3;
    uint8_t* chunkBuffer;       // Buffer for storing chunk data
    //uint8_t* decompBuffer;      // Buffer for storing uncompressed chunk data
    uint16_t CHUNK_BUFFER_SIZE = 2443;//5854 + chunkDelta;
    //uint16_t DECOMP_BUFFER_SIZE = 5854;
    uint16_t remainingLength;
    uint8_t  remainingFrameBytes;

    uint16_t remainingFrames;
    uint8_t  frameCount;        // More than 255 frames cannot physically fit into 64KiB (min frame size is 305, so 214.8 frames can fit)
                                //(this likely [hopefully] will change when compression is implemented)

    uint16_t chunkByteCount = 0;    // Compressed length of chunk
    uint16_t chunkHeadPointer;  // Where in the decompressed buffer we are at
    uint8_t compressionDelta = 2;

    gfx_Begin(); // Initalize graphics

    chunkBuffer = malloc(CHUNK_BUFFER_SIZE);

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
        while (!(os_GetCSC()));
        key = sk_Clear;
    }

    h = 0;
    while (key != sk_Enter && key != sk_Clear) {
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

    if (key == sk_Clear) {
        gfx_End(); // End graphics drawing
        return 0;
    }

    remainingFrames = LLVH_header.frameTotal;
    gfx_FillScreen(74);
    gfx_SetTextXY(0, 8);
    gfx_PrintUInt(remainingFrames, 8);
    while (!(key = os_GetCSC()));

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

    while (remainingFrames > 0) {

        // Read compressed length of chunk
        ti_Read(&chunkByteCount, 2, 1, LLV_FILE);
        gfx_FillScreen(74);
        gfx_SetTextXY(0, 8);
        gfx_PrintUInt(chunkByteCount, 8);
        while (!(key = os_GetCSC()));

        if (key == sk_Clear) {
            return 0;
        }

        // Read chunk into the buffer, aligned to the end
        ti_Read(&chunkBuffer[CHUNK_BUFFER_SIZE - chunkByteCount], chunkByteCount, 1, LLV_FILE);
        gfx_FillScreen(74);
        printBuffer(&chunkBuffer[CHUNK_BUFFER_SIZE - chunkByteCount], chunkByteCount);
        while (!(key = os_GetCSC()));

        if (key == sk_Clear) {
            return 0;
        }

        // Decode chunk into the beginning of the buffer
        zx7_Decompress(&chunkBuffer, &chunkBuffer[CHUNK_BUFFER_SIZE - chunkByteCount]);
        chunkHeadPointer = 0;
        gfx_FillScreen(74);
        printBuffer(chunkBuffer, CHUNK_BUFFER_SIZE);
        while (!(key = os_GetCSC()));

        if (key == sk_Clear) {
            return 0;
        }

        for (i = 0; i < 8; i++) {

            //gfx_FillScreen(74);

            frameHeader = chunkBuffer[chunkHeadPointer];
            lineCount = (chunkBuffer[chunkHeadPointer + 1] << 8) + chunkBuffer[chunkHeadPointer] + 1;
            chunkHeadPointer += 3;

            gfx_FillScreen(74);
            gfx_SetTextXY(0, 8);
            gfx_PrintUInt(frameHeader, 8);
            gfx_SetTextXY(0, 16);
            gfx_PrintUInt(lineCount, 8);
            gfx_SetTextXY(0, 24);
            gfx_PrintUInt(i, 8);
            while (!(key = os_GetCSC()));
            gfx_FillScreen(74);
            printBuffer((chunkBuffer + chunkHeadPointer), lineCount);
            while (!(key = os_GetCSC()));

            // draw (320x240)
            //x = 0xD40000;       // start of vRam, set here because it's used in later ASM and easier to just assign now
            hlSave = 0xD40000;
            y = 0;

            if (frameHeader & 128 && color != 0x00) { // If 1st bit set, start with white (check for black because color is opposite of actual set color)
                color = 0x00;
                gfx_SetColor(0xFF); // Setup color swapping
            }

            if (key == sk_Clear || lineCount > CHUNK_BUFFER_SIZE) { // outdated safety exit conditions
                return 0;
            }

            renderFrame((chunkBuffer + chunkHeadPointer), lineCount);
            while (!(key = os_GetCSC()));
            chunkHeadPointer += lineCount;

            remainingFrames--;
        }
    }

    key = 0;
    //gfx_SetDrawScreen();
    goto select;



    /*

    // read how many frames are in this file
    ti_Read(&frameCount, 1, 1, LLV_FILE);

    for (i = 0; i < frameCount; i++) {
        decodeFrame()
    }

    remainingFrames -= frameCount;  // Subtract the # of frames we just displayed from the total number of frames left

    if (remainingFrames > 0) {  // Frames left to read
        ti_CloseAll();
        goto open;
    }*/

}

