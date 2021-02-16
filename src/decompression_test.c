
#include <string.h>
#include <tice.h>
#include <graphx.h>
#include <fileioc.h>
#include <compression.h>
#include <stdlib.h>
#include <C:/CEdev/include/stdint.h>


void printByte(uint8_t byte) {
    unsigned char hi = (byte >> 4) & 0xf;
    unsigned char lo = byte & 0xf;

    hi += hi < 10 ? '0' : 'A' - 10;
    lo += lo < 10 ? '0' : 'A' - 10;

    gfx_PrintChar(hi);
    gfx_PrintChar(lo);
    gfx_PrintChar(32);  // Space
    return;
}

void printAddr(uint24_t address) {
    printByte(address >> 16 & 0xff);
    printByte(address >> 8 & 0xff);
    printByte(address & 0xff);
}

void printBuffer(uint8_t* buffer, uint16_t size) {
    uint16_t i;
    gfx_SetTextXY(0, 0);
    for (i = 0; i < size; i++) {
        printByte(buffer[i]);
        if ((i % 17 == 0) && i > 0) {
            gfx_SetTextXY(0, gfx_GetTextY() + 8);
        }
        if (gfx_GetTextY() > 230) {
            while (!os_GetCSC());
            gfx_FillScreen(74);
            gfx_SetTextXY(0, 0);
        }
    }
}
/*
int mainb(void) {
    uint8_t compBuffer[] = { 0x54, 0x00, 0x45, 0x53, 0x54, 0x20, 0x46, 0x49, 0x4C, 0x45, 0x10, 0x3A, 0x20, 0x65, 0x6A, 0x00, 0x20, 0x00, 0x2D, 0x20, 0x6C, 0x6F, 0x6C, 0x20, 0x2F, 0x2E, 0x20, 0x5C, 0x00, 0x00, 0x10 };
    uint8_t decompBuffer[100];
    uint8_t delta = 2;           // Delta output by zx7.exe
    uint8_t numFound = 0;
    /*ti_var_t file;
    uint16_t fileSize;
    uint8_t* search_pos;
    char* var_name;


    search_pos = NULL;

    while ((var_name = ti_DetectVar(&search_pos, "LLVC", TI_APPVAR_TYPE)) != NULL) {
        file = ti_Open(var_name, "r");
        numFound++;
    }

    if (numFound == 0) {
        gfx_PrintString("file not found");
        while (!os_GetCSC());
        gfx_End();
        return -1;
    }

    //fileSize = ti_GetSize(file);
    fileSize = 34;
    fileSize -= 4;
    ti_Seek(4, SEEK_SET, file);

    while (!os_GetCSC());*//*

    gfx_Begin();
    gfx_FillScreen(74);
    //gfx_PrintUInt(fileSize, 8);
    gfx_SetTextXY(0, 8);
    printAddr((uint24_t)&compBuffer);

    //while (!os_GetCSC());

    //ti_Read(&compBuffer, fileSize, 1, file);

    while (!os_GetCSC());
    gfx_SetTextXY(0, 16);
    printAddr((uint24_t)&decompBuffer);

    while (!os_GetCSC());
    zx7_Decompress(&decompBuffer, &compBuffer);

    // do something with decompressed data

    //while (!os_GetCSC());
    gfx_FillScreen(74);
    gfx_SetTextXY(0, 0);

    printBuffer(compBuffer, sizeof(compBuffer));
    while (!os_GetCSC());

    gfx_FillScreen(74);
    gfx_SetTextXY(0, 0);

    printBuffer(decompBuffer, sizeof(compBuffer));
    while (!os_GetCSC());

    gfx_FillScreen(74);
    gfx_SetTextXY(0, 0);

    gfx_PrintString(decompBuffer);
    while (!os_GetCSC());

    gfx_End();

    return 0;
}*/