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
#include <keypadc.h>
#include <C:/CEdev/include/stdint.h>    // makes VS Code not whine about uint24_t
//#include <C:/CEdev/BADVIDEO/src/decompression_test.c>

#define DEBUG -1   // -1 = no debug, 0 = chunk header, 1 = chunk buffer, 2 = frame header, 3 = frame buffer (& pause on every frame)
#define ZX7_DELTA 13
#define ZX7_BUFFER_SIZE 41039 + ZX7_DELTA   // for bad apple
//#define ZX7_DELTA 13
//#define ZX7_BUFFER_SIZE 21454 + ZX7_DELTA // for lagtrain
#define FRAMES_PER_CHUNK 8
#define LLV_VERSION 145 // 0b1.001.0001; 1.1 debug

// is this dumb? probably but idc
#define KEYS_CONFIRM ((kb_Data[6] & kb_Enter) || (kb_Data[1] & kb_2nd)) // Pausing & confirm
#define KEYS_CANCEL (kb_Data[6] & kb_Clear)                             // Backing out/canceling
#define KEYS_SKIP (kb_Data[7] & kb_Right)                               // Skipping ahead in the video
#define KEYS_INFO (kb_Data[1] & kb_Mode)                                // Display more info for the video/frame

//#define COLOR_OFF 0x00
//#define COLOR_ON 0xFF
#define COLOR_BACKGROUND 0x1E
#define COLOR_TEXT 0x00
#define COLOR_ERROR 0x80

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
  uint8_t    color_off;
  uint8_t    color_on;
  uint8_t    titleLength;
  char* title;
  uint8_t    title_NULL; //fanstastic progamming, i know (this is the 0x00 byte that null-terminates the title screen)
  uint16_t   frameTotal;
} llvh_header_t;

ti_var_t LLV_FILE;

uint8_t  color = 0x00;
uint16_t lineCount;
uint24_t hlSave;



void renderFrame(uint8_t frame[], uint16_t lineCount) {
  uint8_t line;
  uint16_t j;
  for(j = 0; j < lineCount; j++) {

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
      "skip_ASM:\n"//"EI\n"
      "ld  a, (IX-3)\n"               // Restore A register from before this code (is used b4 by getting the line, and after by the if statement)
    );

    if(line != 255) {               // Swap color, unless line length was exactly 255 (0xFF). This is to allow contiguous sections of the same color longer than 255.
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

  // file select menu, loading, & reading
  llvh_header_t LLVH_header;
  frame_header_t frame_header;

  char fileNames[][10] = { "_______0", "_______1", "_______2", "_______3", "_______4", "_______5", "_______6", "_______7", "_______8", "_______9" };

  uint8_t  frameHeader;
  char* var_name;
  uint8_t* search_pos;
  uint8_t  numFound;
  uint16_t h;
  uint16_t i;
  uint8_t pause = 0;

  // data decoding
  uint16_t y;
  uint8_t* chunkBuffer;       // Buffer for storing chunk data
  uint16_t remainingLength;
  uint8_t  remainingFrameBytes;
  uint16_t readFileSize;
  uint8_t  headerSize;

  uint16_t remainingFrames;
  uint8_t  repeatFrames;      // how many frames to leave this frame onscreen

  uint16_t chunkByteCount = 0;    // Compressed length of chunk
  uint16_t chunkHeadPointer;  // Where in the decompressed buffer we are at
  uint8_t compressionDelta = 2;

  // timekeeping
  double msAhead = 0;   // running total of how many miliseconds we're ahead (may be [usually] negative, indicating we're behind)
  double msToRender;    // how long this frame took to render (+ time it took to compute logic & whatnot)
  double targetMs = 0;  // how long we want to spend rendering a frame (in miliseconds).

  //uint32_t timerSave = 0;

  //start up a timer for FPS monitoring, do not move:
  timer_Control = TIMER1_ENABLE | TIMER1_32K | TIMER1_UP;

  // angery lettuce man says this is how to use keypad fastly:
  kb_SetMode(MODE_3_CONTINUOUS);

  gfx_Begin(); // Initalize graphics

  chunkBuffer = malloc(ZX7_BUFFER_SIZE);
  if(chunkBuffer == NULL) {
    gfx_PrintString("malloc fail");
    while(!os_GetCSC());   // we can use this here because we're about to exit anyways
    gfx_End();
    return -1;
  }



select:
  // Close any files that may be open already
  ti_CloseAll();

  kb_Scan();  // update keypad? idk if this needs to be done but jetpack joyride does it so

  gfx_FillScreen(COLOR_BACKGROUND);

  search_pos = NULL;
  numFound = 0;

  // Pick file
  while((var_name = ti_DetectVar(&search_pos, "LLVH", TI_APPVAR_TYPE)) != NULL) {
    LLV_FILE = ti_Open(var_name, "r");

    // Read data
    strcpy(LLVH_header.fileName, var_name);            // File name for printing
    strcpy(fileNames[numFound], var_name);             // File name saved for opening
    ti_Seek(4, SEEK_SET, LLV_FILE);                    // Seek past "LLVH" header
    ti_Read(&LLVH_header.fileName + 8, 6, 1, LLV_FILE); // Offsets 0-5 (version, features, fps, off, on)
    LLVH_header.title = (char*)ti_MallocString(LLVH_header.titleLength);
    ti_Read(LLVH_header.title, 1, LLVH_header.titleLength, LLV_FILE); // Title String
    /*if ((LLVH_header.features) & 128) {                        // Captions exist
        ti_Read(&LLVH_header.captionCount, 2, 1, LLV_FILE);    // Number of captions
        ti_Seek(LLVH_header.captionCount, SEEK_CUR, LLV_FILE); // Seek pask the captions list
    }*/
    ti_Read(&LLVH_header.frameTotal, 1, 2, LLV_FILE); // Number of frames

    // Print in list
    if(LLVH_header.version != LLV_VERSION) {
      gfx_SetTextFGColor(COLOR_ERROR);
    } else {
      gfx_SetTextFGColor(COLOR_TEXT);
    }
    gfx_SetTextXY(10, numFound * 8);
    gfx_PrintString(LLVH_header.fileName);
    gfx_SetTextXY(10 + 64 + 10, numFound * 8);
    gfx_PrintString(LLVH_header.title);
    gfx_SetTextXY(10 + 64 + 10 + 64 + 10, numFound * 8);
    gfx_PrintUInt(LLVH_header.fps, 3);
    /*gfx_SetTextXY(10 + 64 + 10, numFound * 8);
    gfx_PrintUInt(LLVH_header.version, 8);
    gfx_SetTextXY(10 + 64 + 10 + 64 + 10, numFound * 8);
    gfx_PrintUInt(LLVH_header.features, 8);*/
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
    if(numFound > 10)  // We only have enough space for 10 names in the list (TODO: expand list to store a few more, and implement scrolling/whatever through files [search more when scrolling])
      break;
  }

  //           LLVH, Header, Title string,       frameTotal
  headerSize = 4 + 6 + LLVH_header.titleLength + 2;

  if(numFound == 0) {
    gfx_SetTextXY(10, 0);
    gfx_PrintString("No videos found.");
    while(!kb_AnyKey());
    gfx_End();
    return 0;
  }

  h = 0;
  while(!KEYS_CONFIRM && !KEYS_CANCEL) {
    switch(kb_Data[7]) {   //?
      case kb_Up:
        if(h > 0)
          h--;
        break;
      case kb_Down:
        if(h < numFound - 1)
          h++;
        break;
        // `mode` for more info?
    }

    // Draw selection carrot
    gfx_SetColor(COLOR_BACKGROUND);
    gfx_FillRectangle(0, 0, 10, 8 * numFound);
    gfx_SetTextXY(0, h * 8);
    gfx_PrintString(">");

    while(!kb_AnyKey());
  }

  if(KEYS_CANCEL) {
    gfx_End(); // End graphics drawing
    return 0;
  }

  remainingFrames = LLVH_header.frameTotal;
  targetMs = 1000.0 / LLVH_header.fps;


  if(LLVH_header.version != LLV_VERSION) {
#if DEBUG > 0
    gfx_SetTextXY(0, 8);
    gfx_PrintUInt(remainingFrames, 8);
#endif
    gfx_FillScreen(COLOR_BACKGROUND);
    if(LLVH_header.version < LLV_VERSION) {
      gfx_SetTextFGColor(COLOR_ERROR);
      gfx_PrintStringXY("File outdated:", 0, 0);
      gfx_SetTextXY(15 * 8, 0);
      gfx_PrintUInt(LLVH_header.version, 8);
    }
    if(LLVH_header.version > LLV_VERSION) {
      gfx_SetTextFGColor(COLOR_ERROR);
      gfx_PrintStringXY("Program outdated:", 0, 0);
      gfx_SetTextXY(18 * 8, 0);
      gfx_PrintUInt(LLVH_header.version, 8);
    }
    while(kb_AnyKey());
    while(!kb_AnyKey());
    if(KEYS_CANCEL) {
      gfx_End(); // End graphics drawing
      return 0;
    }
  }
  while(kb_AnyKey());    // wait for all keys to be unpressed (so that the enter in the menu doesn't immediatley pause)

  //TODO: read filenames properly
  LLV_FILE = ti_Open(fileNames[h], "r");
  fileNames[h][6] = '0';
  fileNames[h][7] = '0';
  //remainingFileSize = ti_GetSize(LLV_FILE) - headerSize;

  // Seek to the beginning of the frame data
  ti_Seek(headerSize, SEEK_SET, LLV_FILE);

  //gfx_SetDrawBuffer(); // Enable buffering (because the screen is fully redrawn each frame)

  // reset the timer before entering the main loop
  timer_1_Counter = 0;

  while(remainingFrames > 0) {

    // Read compressed length of chunk
    ti_Read(&chunkByteCount, 2, 1, LLV_FILE);
    //remainingFileSize -= 2;
    //chunkByteCount = 2048;
#if DEBUG == 0
    gfx_FillScreen(COLOR_BACKGROUND);
    gfx_SetTextXY(0, 0);
    gfx_PrintUInt(chunkByteCount, 8);
    /*gfx_SetTextXY(0, 8);
    gfx_PrintUInt(remainingFileSize, 8);*/
    while(!(key = os_GetCSC()));

    if(key == sk_Clear) {
      return 0;
    }
#endif
    memset(chunkBuffer, 0x00, ZX7_BUFFER_SIZE);

    // Read chunk into the buffer, aligned to the end
    //Read as much as we can from this file
    readFileSize = ti_Read(&chunkBuffer[ZX7_BUFFER_SIZE - chunkByteCount], 1, chunkByteCount, LLV_FILE);

    if(readFileSize < chunkByteCount) { // If we didn't read the entire chunk from that file
      ti_Close(LLV_FILE);     // Close it & open the next
      LLV_FILE = ti_Open(fileNames[h], "r");
      fileNames[h][7]++;
      if(fileNames[h][7] == ':') {
        fileNames[h][6]++;
        fileNames[h][7] = '0';
      }
      // Read however much we still need to from the next file
      ti_Read(&chunkBuffer[ZX7_BUFFER_SIZE - chunkByteCount + readFileSize], chunkByteCount - readFileSize, 1, LLV_FILE);
    }

#if DEBUG == 1
    gfx_FillScreen(COLOR_BACKGROUND);
    printBuffer(&chunkBuffer[ZX7_BUFFER_SIZE - chunkByteCount], chunkByteCount);
    while(!(key = os_GetCSC()));

    if(key == sk_Clear) {
      return 0;
    }
#endif

    // Decode chunk into the beginning of the buffer
    zx7_Decompress(chunkBuffer, &chunkBuffer[ZX7_BUFFER_SIZE - chunkByteCount]);
    chunkHeadPointer = 0;
#if DEBUG == 1
    gfx_FillScreen(COLOR_BACKGROUND);
    printBuffer(chunkBuffer, ZX7_BUFFER_SIZE);
    while(!(key = os_GetCSC()));

    if(key == sk_Clear) {
      return 0;
    }
#endif

    for(i = 0; i < FRAMES_PER_CHUNK; i++) {
      // fun fact: the order you decode the header in actually matters! i totally didn't spend 2 days trying to figure out why it wouldn't decode properly while having this code out of order...
      frameHeader = chunkBuffer[chunkHeadPointer];
      chunkHeadPointer += 1;

      repeatFrames = 0;
      if(frameHeader & 32) { // If 3rd bit set, read how many times to play this frame
        repeatFrames = chunkBuffer[chunkHeadPointer];
        chunkHeadPointer += 1;
      }

      lineCount = (chunkBuffer[chunkHeadPointer + 1] << 8) + chunkBuffer[chunkHeadPointer];
      chunkHeadPointer += 2;

      hlSave = 0xD40000;   // start of vRam, set here because it's used in later ASM and easier to just assign now
      y = 0;


#if DEBUG == 2
      gfx_SetColor(COLOR_BACKGROUND);
      gfx_FillRectangle_NoClip(0, 0, 64, 24);
      gfx_SetTextXY(0, 0);
      gfx_PrintUInt(frameHeader, 3);
      gfx_SetTextXY(0, 8);
      gfx_PrintUInt(repeatFrames, 3);
      gfx_SetTextXY(0, 16);
      gfx_PrintUInt(lineCount, 5);
      gfx_SetTextXY(0, 24);
      gfx_PrintUInt(remainingFrames, 5);
      while(kb_AnyKey());
      while(!kb_AnyKey());
      while(kb_AnyKey());
#endif
#if DEBUG == 3
      gfx_FillScreen(COLOR_BACKGROUND);
      printBuffer((chunkBuffer + chunkHeadPointer), lineCount);
      while(!(key = os_GetCSC()));

      if(key == sk_Clear || lineCount > ZX7_BUFFER_SIZE) { // outdated safety exit conditions
        gfx_End();
        return 0;
      }
#endif
      color = LLVH_header.color_on;           // Setup color swapping
      gfx_SetColor(LLVH_header.color_off);
      if(frameHeader & 128) { // If 1st bit set, swap color to start with white
        color = gfx_SetColor(color);
      }

#if 0
      if(/*!KEYS_SKIP &&*/ msBehind <= (1000.0 / LLVH_header.fps)) {  // Calculate if we have the time to render this frame
        renderFrame((chunkBuffer + chunkHeadPointer), lineCount);     // If we're behind by less than 1 frame, render the next one
      }
#endif

      if(msAhead > -targetMs) { // if we're not more than 1 frame behind
        renderFrame((chunkBuffer + chunkHeadPointer), lineCount);
      }

      /*else { //if (msBehind > (1000 / LLVH_header.fps)) { // If not, skip ahead without spending the time to render
         /*if(msBehind >= (1000.0 / LLVH_header.fps)) {  // don't underflow?
          }*/
          //}

      do {
        //FPS counter data collection, time "stops" (being relevant) after this point; insert Jo-Jo reference here
        //FPS = (32768.0 / timer_1_Counter); // Cycles/Second / Cycles = x/Second

        // calculate how long this frame (& logic) took to compute. time "stops" (being relevant) after this point; insert Jo-Jo reference here
        msToRender = (1000.0 / (32768 / timer_1_Counter));

        timer_1_Counter = 0;  // time is actually relavent again here

        // menu stuff
        kb_Scan();
        if(KEYS_CONFIRM || KEYS_SKIP || pause == 2) {
          // Pause Bar
          gfx_SetColor(COLOR_BACKGROUND);
          gfx_FillRectangle(0, 230, LCD_WIDTH, 10);   // Background
          gfx_SetColor(LLVH_header.color_off);
          gfx_SetTextXY(1, 232);      // Play button & current time
          gfx_PrintString("\x10 ");
          gfx_PrintUInt(LLVH_header.frameTotal - remainingFrames, 3);
          gfx_SetTextXY(282, 232);
          gfx_PrintUInt(LLVH_header.frameTotal, 4);   // Total length
          gfx_FillRectangle(38, 235, 243, 1);  // Playbar
          gfx_FillRectangle(38, 234, 243 / (LLVH_header.frameTotal / ((float)LLVH_header.frameTotal - remainingFrames)), 3);  // yeet math (played video progress)

          // Janky keypad unjanking
          if(!KEYS_SKIP) {
            pause = 1;
            while(kb_AnyKey());    // wait for the pause key to be unpressed
            while(pause == 1) {
              while(!kb_AnyKey());
              if(KEYS_INFO) {
                gfx_SetColor(COLOR_BACKGROUND);
                gfx_FillRectangle_NoClip(0, 0, 64, 56);
                gfx_SetTextXY(0, 0);
                gfx_PrintUInt(frameHeader, 3);
                gfx_SetTextXY(0, 8);
                gfx_PrintUInt(repeatFrames, 3);
                gfx_SetTextXY(0, 16);
                gfx_PrintUInt((unsigned)msToRender, 5);  // FPS would be (unsigned)(1000 / msToRender)
                gfx_SetTextXY(0, 24);
                gfx_PrintInt((int)msAhead, 5);
                gfx_SetTextXY(0, 32);
                gfx_PrintUInt(remainingFrames, 5);
                gfx_SetTextXY(0, 40);
                gfx_PrintUInt((int)targetMs, 5);
              }
              if(KEYS_SKIP) { pause = 2; }
              if(KEYS_CONFIRM) { pause = 0; }
              if(KEYS_CANCEL) {
                remainingFrames = 1;    // Trick the exit code to think we're done playing the vid (because we are i guess)
                pause = 0;
              }
              while(kb_AnyKey());    // require the key to be unpressed b5 we continue*/
            }
          }

          // reset the timer again after menu stuff is done because we don't want to count the time spent in the menu towards rendering the next frame
          timer_1_Counter = 0;
        }

        /*gfx_SetColor(COLOR_BACKGROUND);
        gfx_FillRectangle(0, 0, 4 * 8, 16);
        gfx_SetTextXY(0, 0);
        gfx_PrintInt((int) FPS, 4);*/


        /*
      if (msAhead > -targetMs): // if we're not more than 1 frame behind
          render the frame

      --msToRender = time it took to render or skip [in ms] (calculated using system timer)

        if(msToRender > targetMs) // time spent is > target ms per frame
          add it to how far we're behind

        if(msToRender < targetMs) { // time spent is < target ms per frame

          subtract it from how far we're behind
            if we're not behind, must delay the remaining ms


        }

          msAhead += targetMs - msToRender;

          if(msAhead > targetMs) {  // if we're ahead by over a frame, delay to get Back on Track - DJVI
            delay(msAhead);         // we allow getting ahead by a bit in case the next frame suddenly takes longer
            msAhead = 0; // msAhead + -msAhead;
          }





        */

        msAhead += targetMs - msToRender;

        if(msAhead > targetMs) {  // if we're ahead by over a frame, delay to get Back on Track - DJVI
          /*gfx_SetTextXY(0, 48);
          gfx_SetTextFGColor(COLOR_ERROR);
          gfx_PrintInt(msAhead, 0);
          gfx_SetTextFGColor(COLOR_TEXT);*/

          //timerSave = timer_1_Counter;
          delay((uint16_t)msAhead);         // we allow getting ahead by a bit in case the next frame suddenly takes longer
          //timer_1_Counter = timerSave;


          msAhead = 0;//msAhead - (int)msAhead;
          timer_1_Counter = 0;    // disregard the time it took to delay, don't count it towards next frame's msToRender


          /*if(pause == 1) {
            while(!kb_AnyKey());
            while(kb_AnyKey());    // require the key to be unpressed b5 we continue
          }*/
        }

        if(repeatFrames > 0) {
          repeatFrames--;
          delay(1);   // gross but works i guess (makes sure timer 1 isn't 0 when we calc the FPS while repeating frames, so that we delay properly)
        }
      } while(repeatFrames > 0);

#if DEBUG == 3
      while(!os_GetCSC());
#endif

      chunkHeadPointer += lineCount;
      remainingFrames--;
      if(remainingFrames == 0) { // remainingFrames is unsigned, so it will never be below 0 (the cancel code used to set it to 0, not 1. this caused many problems...)
        break;
      }
    }
  }

  //gfx_SetDrawScreen();
  goto select;
}