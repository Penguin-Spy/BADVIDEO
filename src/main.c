/*
 *--------------------------------------
 * Program Name:    BADVIDEO
 * Author:          Penguin_Spy
 * Description:     "Bad" 2-tone video player for the TI-84+ CE
 *                  Renders a series of frames stored as the length between each color flip, sorta like gen 1 pokemon sprites (i think).
 * License:
    Copyright (C) 2022  Penguin_Spy

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

/* ZX7 Compression values
 * These must be at least equal to or larger than the values output by convertFrame.py
 * Values for the two preconverted videos are included below.
 * TODO: encode this into the LLV header & allocate after selecting the video & before playing
*/
#define ZX7_DELTA 13
//#define ZX7_BUFFER_SIZE 21454 + ZX7_DELTA // for bad apple
//#define ZX7_DELTA 13
#define ZX7_BUFFER_SIZE 41039 + ZX7_DELTA   // for lagtrain

/* FEATURE SWITCHES
 * Enable these if testing extra features. 0 = disabled, 1 = enabled.
 * DO NOT TOUCH IF YOU AREN'T TESTING!
*/
// WARNING: Debug code may be outdated & fail to compile, or prevent the keypad from working (softlocking the program)
#define DEBUG -1   // -1 = no debug, 0 = chunk header, 1 = chunk buffer, 2 = frame header, 3 = frame buffer (& pause on every frame)
#define FEATURE_CAPTIONS 0
#define FEATURE_DRAW_BUFFERING 0

// must equal convertFrame.py's values
#define FRAMES_PER_CHUNK 8
#define LLV_VERSION 17 // 0b0.001.0001; 1.1 release

// keypad controls
#define KEYS_CONFIRM ((kb_Data[6] & kb_Enter) || (kb_Data[1] & kb_2nd)) // Pausing & confirm
#define KEYS_CANCEL (kb_Data[6] & kb_Clear)                             // Backing out/canceling
#define KEYS_SKIP (kb_Data[7] & kb_Right)                               // Skipping ahead in the video
#define KEYS_INFO (kb_Data[1] & kb_Mode)                                // Display more info for the video/frame

// Colors
#define COLOR_BACKGROUND 0x1E
#define COLOR_TEXT 0x00
#define COLOR_ERROR 0x80

// LLV file format defines
//see format.md for descriptions
#define LLV_FILE_FEATURES_SOUND (1<<7)    // UNUSED/RESERVED
#define LLV_FILE_FEATURES_CAPTIONS (1<<6) // UNUSED/RESERVED

#define LLV_FRAME_HEADER_COLOR (1<<7)
#define LLV_FRAME_HEADER_COMPRESSIONDIR (1<<6)  // UNUSED/RESERVED
#define LLV_FRAME_HEADER_REPEAT (1<<5)
#define LLV_FRAME_HEADER_SOUND (1<<4)           // UNUSED/RESERVED

// Type defines
#if FEATURE_CAPTIONS == 1
typedef struct {
  char* text;
  uint16_t x;
  uint8_t  y;
  uint8_t  color;
} caption_t;
#endif

typedef struct {
  uint8_t    version;
  uint8_t    features;
  uint8_t    fps;
  uint8_t    color_off;
  uint8_t    color_on;
  uint8_t    titleLength;
  char* title;
  uint16_t   frameTotal;
} llvh_header_t;

// Open file's handle
ti_var_t LLV_FILE;

// these variables are defined outside of main() so they may be accessed safely from the asm routine.
// the assembler accesses them by name instead of by offset from the IX register, meaning they won't change position when C code changes.
uint8_t  color = 0x00;    // current color of screen drawing
uint16_t lineCount;       // # of lines in this frame
uint24_t hlSave;          // variable used to save drawing position between asm routine executions


// Render a frame that's encoded as a set of line lengths between color changes.
void renderFrame(uint8_t frame[], uint16_t lineCount) {
  uint8_t line;
  uint16_t j;
  for(j = 0; j < lineCount; j++) {

    line = frame[j];

    // Inline asm code to draw a single line to vRam quickly.
    asm(
      "xor   a, a\n"                  // clear A
      "sub   a, (IX-3)\n" // line     // Check if the line length is 0
      "jr    z, skip_ASM\n"           // If zero, jump past all this inline asm (we don't want to draw a line of 0, it would underflow BC & fill past vRam)
      "ld    hl, (_hlSave)\n" // hlSave    // starting location (location of vram + x)
      "ld    bc, 0x000000\n"          // zero out BCU
      "ld    c, (IX-3)\n" // line     // copy line length byte
      "push  hl\n"                    // Copy hl to de
      "pop   de\n"
      // Write initial 2-byte value
      "ld    a,(_color)\n"  // color  // Load color into screen pixel
      "ld    (hl),a\n"
      "inc   hl\n"
      "ex    de, hl\n"                // swap de & hl (above instructions only work with hl)
      //Copy everything all at once. Interrupts may trigger while this instruction is processing.
      "ldir\n"                        // fancy-shmancy instruction that loops until bc == 0x00, copying (hl) to (de) and incrementing both
      "ld    (_hlSave), hl\n" // x
      "skip_ASM:\n"
      "ld  a, (IX-3)\n"               // Restore A register from before this code (the C compiler uses it b4 for getting the line, and after by the if statement, and expects it to not change)
    );

    if(line != 255) {               // Swap color, unless line length was exactly 255 (0xFF). This is to allow contiguous sections of the same color longer than 255.
      color = gfx_SetColor(color);  // To display a line of length 255, encode it as a 255 length line (0xFF) followed by a 0 length line (0x00).
    }
  }
}


int main(void) {
  /* File select menu, loading, & reading */
  llvh_header_t LLVH_header;  // header for current file (reused during playback)
  //holds found filenames
  char fileNames[][10] = { "_______0", "_______1", "_______2", "_______3", "_______4", "_______5", "_______6", "_______7", "_______8", "_______9" };
  char* var_name;     // name of file currently being read in file select menu
  uint8_t* search_pos;   // save the search position for ti_DetectVar
  uint8_t numFound;   // how many matching files have been found
  uint16_t fileIndex; // file select cursor pos

  /* Data decoding */
  //file
  uint16_t readFileSize;        // keeps track of how much of a file has been read
  uint16_t remainingFrames;     // Frames remaining in this file to be played
  //chunks
  uint16_t chunkByteCount = 0;  // Compressed length of chunk
  uint16_t chunkHeadPointer;    // Where in the decompressed buffer we are at
  uint8_t* chunkBuffer;         // Buffer for storing chunk data
  uint16_t i;                   // used for iterating through each frame in a chunk
  //individual frames
  uint8_t  frameHeader;         // bitfield describing this frame's attributes. See format.md
  uint8_t  repeatFrames;        // how many frames to leave this frame onscreen

  // State of pause menu, used for frame-by-frame skipping
  uint8_t pause = 0;

  /* Timekeeping */
  double msAhead = 0;   // running total of how many miliseconds we're ahead (may be [usually] negative, indicating we're behind)
  double msToRender;    // how long this frame took to render (+ time it took to compute logic & whatnot)
  double targetMs = 0;  // how long we want to spend rendering a frame (in miliseconds).


  // start up a timer for frame skip/delay, do not move:
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

file_select:
  // Close any files that may be open already
  ti_CloseAll();

  kb_Scan();  // update keypad? idk if this needs to be done but jetpack joyride does it so

  gfx_FillScreen(COLOR_BACKGROUND);

  search_pos = NULL;
  numFound = 0;

  // Load & display video files
  while((var_name = ti_DetectVar(&search_pos, "LLVH", TI_APPVAR_TYPE)) != NULL) {
    LLV_FILE = ti_Open(var_name, "r");

    // Read data
    strcpy(fileNames[numFound], var_name);             // File name saved for opening
    ti_Seek(4, SEEK_SET, LLV_FILE);                    // Seek past "LLVH" header
    ti_Read(&LLVH_header.version, 6, 1, LLV_FILE); // Offsets 0-5 (version, features, fps, off_color, on_color)

    //allocate space for & null terminate the title string
    LLVH_header.title = malloc(LLVH_header.titleLength + 1);  // don't cast the return of malloc :troll:
    LLVH_header.title[LLVH_header.titleLength] = '\0';
    ti_Read(LLVH_header.title, 1, LLVH_header.titleLength, LLV_FILE); // read title string

#if FEATURE_CAPTIONS == 1
    if((LLVH_header.features) & LLV_FILE_FEATURES_CAPTIONS) { // Captions exist
      ti_Read(&LLVH_header.captionCount, 2, 1, LLV_FILE);     // Number of captions
      ti_Seek(LLVH_header.captionCount, SEEK_CUR, LLV_FILE);  // Seek pask the captions list
    }
#endif

    ti_Read(&LLVH_header.frameTotal, 1, 2, LLV_FILE); // Number of frames

    // Print in list
    if(LLVH_header.version != LLV_VERSION) {
      gfx_SetTextFGColor(COLOR_ERROR);
    } else {
      gfx_SetTextFGColor(COLOR_TEXT);
    }
    gfx_SetTextXY(10, numFound * 8);
    gfx_PrintString(var_name);
    gfx_SetTextXY(10 + 64 + 10, numFound * 8);
    gfx_PrintString(LLVH_header.title);
    gfx_SetTextXY(10 + 64 + 10 + LLVH_header.titleLength * 8 + 10, numFound * 8);
    gfx_PrintUInt(LLVH_header.fps, 3);
    gfx_SetTextXY(10 + 64 + 10 + LLVH_header.titleLength * 8 + 10 + 24 + 10, numFound * 8);
    gfx_PrintUInt(LLVH_header.frameTotal, 4);

    // Close file.
    ti_CloseAll();

    numFound++;
    if(numFound > 10)  // We only have enough space for 10 names in the list (TODO: expand list to store a few more, and implement scrolling/whatever through files [search more when scrolling])
      break;
    }

  if(numFound == 0) {
    gfx_SetTextXY(10, 0);
    gfx_PrintString("No videos found.");
    while(!kb_AnyKey());
    gfx_End();
    return 0;
  }

  // Choosing which video to play
  fileIndex = 0;
  while(!KEYS_CONFIRM && !KEYS_CANCEL) {
    switch(kb_Data[7]) {   //?
      case kb_Up:
        if(fileIndex > 0)
          fileIndex--;
        break;
      case kb_Down:
        if(fileIndex < numFound - 1)
          fileIndex++;
        break;
        // `mode` for more info?
    }

    // Draw selection carrot
    gfx_SetColor(COLOR_BACKGROUND);
    gfx_FillRectangle(0, 0, 10, 8 * numFound);
    gfx_SetTextXY(0, fileIndex * 8);
    gfx_PrintString(">");

    while(!kb_AnyKey());
  }

  if(KEYS_CANCEL) {
    gfx_End(); // End graphics drawing
    return 0;
  }

  remainingFrames = LLVH_header.frameTotal;
  targetMs = 1000.0 / LLVH_header.fps;

  // Confirm file is safe to play
  if(LLVH_header.version != LLV_VERSION) {
#if DEBUG > 0
    gfx_SetTextXY(0, 8);
    gfx_PrintUInt(remainingFrames, 8);
    gfx_SetTextXY(0, 16);
    gfx_PrintUInt(targetMs, 8);
#endif
    gfx_FillScreen(COLOR_BACKGROUND);
    if(LLVH_header.version < LLV_VERSION) {
      gfx_SetTextFGColor(COLOR_ERROR);
      gfx_PrintStringXY("File outdated: ", 0, 0);
      gfx_SetTextXY(16 * 8, 0);
      gfx_PrintUInt(LLVH_header.version, 8);
    }
    if(LLVH_header.version > LLV_VERSION) {
      gfx_SetTextFGColor(COLOR_ERROR);
      gfx_PrintStringXY("Program outdated: ", 0, 0);
      gfx_SetTextXY(19 * 8, 0);
      gfx_PrintUInt(LLVH_header.version, 8);
    }
    gfx_PrintStringXY("Clear to exit, any key to unsafely continue.", 0, 8);
    while(kb_AnyKey());
    while(!kb_AnyKey());
    if(KEYS_CANCEL) {
      gfx_End(); // End graphics drawing
      return 0;
    }
    // text FG color is intentionally left as COLOR_ERROR if version mismatch warning was disregarded
  }

  while(kb_AnyKey());    // wait for all keys to be unpressed (so that the enter in the menu doesn't immediatley pause)

  // Read first file & setup filename counting
  LLV_FILE = ti_Open(fileNames[fileIndex], "r");
  fileNames[fileIndex][6] = '0';
  fileNames[fileIndex][7] = '0';

  // Seek to the beginning of the frame data
  //      "LLVH" + Header, Title string,             frameTotal
  ti_Seek(4 +      6 +     LLVH_header.titleLength + 2, SEEK_SET, LLV_FILE);

#if FEATURE_DRAW_BUFFERING == 1
  gfx_SetDrawBuffer(); // Enable buffering (because the screen is fully redrawn each frame)
#endif

  // reset the timer before entering the main loop
  timer_1_Counter = 0;

  // Main frame loop
  while(remainingFrames > 0) {

    // Read compressed length of chunk
    ti_Read(&chunkByteCount, 2, 1, LLV_FILE);
#if DEBUG == 0
    gfx_FillScreen(COLOR_BACKGROUND);
    gfx_SetTextXY(0, 0);
    gfx_PrintUInt(chunkByteCount, 8);
    while(!(key = os_GetCSC()));

    if(key == sk_Clear) {
      return 0;
    }
#endif
    memset(chunkBuffer, 0x00, ZX7_BUFFER_SIZE);

    // Read chunk into the buffer, aligned to the end

    // Read as much as we can from this file
    readFileSize = ti_Read(&chunkBuffer[ZX7_BUFFER_SIZE - chunkByteCount], 1, chunkByteCount, LLV_FILE);

    // If we didn't read the entire chunk from that file
    if(readFileSize < chunkByteCount) {
      ti_Close(LLV_FILE);     // Close it & open the next
      LLV_FILE = ti_Open(fileNames[fileIndex], "r");
      fileNames[fileIndex][7]++;
      if(fileNames[fileIndex][7] == ':') {
        fileNames[fileIndex][6]++;
        fileNames[fileIndex][7] = '0';
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
      if(frameHeader & LLV_FRAME_HEADER_REPEAT) { // If 3rd bit set, read how many times to play this frame
        repeatFrames = chunkBuffer[chunkHeadPointer];
        chunkHeadPointer += 1;
      }

      lineCount = (chunkBuffer[chunkHeadPointer + 1] << 8) + chunkBuffer[chunkHeadPointer];
      chunkHeadPointer += 2;

      hlSave = 0xD40000;   // start of vRam, set here because it's used in later ASM and easier to just assign now

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
      // Setup color swapping
      color = LLVH_header.color_on;
      gfx_SetColor(LLVH_header.color_off);
      if(frameHeader & LLV_FRAME_HEADER_COLOR) { // If 1st bit set, swap starting color
        color = gfx_SetColor(color);
      }

      if(msAhead > -targetMs && !KEYS_SKIP) { // if we're not more than 1 frame behind
        renderFrame((chunkBuffer + chunkHeadPointer), lineCount);
      }
      // A frame has either been rendered or skipped, so seek through the chunk to the next frame
      chunkHeadPointer += lineCount;

      // do {} while(repeatFrames > 0);
      do {
        // calculate how long this frame (& logic) took to compute. time "stops" (being relevant) after this point; insert Jo-Jo reference here
        msToRender = (1000.0 / (32768 / timer_1_Counter));

        timer_1_Counter = 0;  // time is actually relavent again here

        // pause menu stuff
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
                gfx_PrintInt((int)targetMs, 5);
              }
              if(KEYS_SKIP) { pause = 2; }
              if(KEYS_CONFIRM) { pause = 0; }
              if(KEYS_CANCEL) {
                remainingFrames = 1;    // Trick the exit code to think we're done playing the vid (because we are i guess)
                pause = 3;  // exit the pause menu (and tell exit code we canceled playback)
              }
              while(kb_AnyKey());    // require the key to be unpressed b5 we continue
            }
          }

          // reset the timer again after menu stuff is done because we don't want to count the time spent in the menu towards rendering the next frame
          timer_1_Counter = 0;
        }

        // Calculate frame timing
        if(!KEYS_SKIP) {
          msAhead += targetMs - msToRender;

          // if we're ahead by over a frame, delay to get Back on Track - DJVI
          if(msAhead > targetMs) {
            delay((uint16_t)msAhead);         // we allow getting ahead by a bit in case the next frame suddenly takes longer
            msAhead = 0;  //msAhead - (int)msAhead;
            timer_1_Counter = 0;    // disregard the time it took to delay, don't count it towards next frame's msToRender
          }
        }

        if(repeatFrames > 0) {
          repeatFrames--;
          delay(1);   // gross but works i guess (makes sure timer 1 isn't 0 when we calc the FPS while repeating frames, so that we delay properly)
        }
      } while(repeatFrames > 0);

#if DEBUG == 3
      while(!os_GetCSC());
#endif

      remainingFrames--;
      if(remainingFrames == 0) { // remainingFrames is unsigned, so it will never be below 0 (the cancel code used to set it to 0, not 1. this caused many problems...)
        break;
      }
    }
  }

  // fake pause menu to show playback is complete.
  if(pause != 3) {
    gfx_SetColor(COLOR_BACKGROUND);
    gfx_FillRectangle(0, 230, LCD_WIDTH, 10);   // Background
    gfx_PrintStringXY("Playback complete.", 1, 232);

    // wait for keypress before returing to file select
    while(!kb_AnyKey());
    while(kb_AnyKey());
  }

#if FEATURE_DRAW_BUFFERING == 1
  gfx_SetDrawScreen();
#endif

  goto file_select;
  }