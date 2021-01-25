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








int main(void)
{
    
    //uint8_t frame[] = {255,255,255,255,255,190,4,255,59,8,255,57,8,255,56,10,255,55,10,255,55,10,255,55,10,255,56,8,255,57,8,255,59,4,255,255,255,25,4,255,59,8,255,57,8,255,56,10,255,55,10,255,55,10,255,55,10,255,56,8,255,57,8,255,59,4,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,20,4,255,59,8,255,56,9,255,55,11,255,53,12,255,53,12,255,52,13,255,51,13,255,51,14,84,4,217,14,83,8,214,15,83,8,213,14,84,10,211,15,84,10,210,15,85,10,209,15,86,10,208,15,87,10,207,15,88,10,206,15,89,11,204,15,90,11,204,14,91,12,202,14,93,12,200,14,94,12,199,14,96,12,197,14,97,13,195,14,99,13,193,14,100,14,191,15,101,14,189,15,103,14,187,15,104,16,184,15,106,16,182,15,108,16,180,15,110,16,179,14,112,16,177,14,114,17,174,14,116,17,172,14,118,17,170,14,121,16,168,14,123,17,165,14,125,17,163,15,126,17,161,15,128,18,158,15,131,17,156,15,133,17,154,15,135,18,152,14,137,18,150,14,140,17,148,14,142,18,145,14,144,18,143,14,147,18,140,14,149,18,138,14,151,19,135,15,153,18,133,15,155,19,130,15,157,19,128,15,160,19,124,16,162,20,121,16,165,19,119,16,167,20,116,16,170,20,113,16,172,21,109,17,175,21,106,17,177,22,102,18,180,22,99,17,184,23,94,18,186,25,90,18,189,25,86,19,192,26,81,20,195,26,77,20,199,27,73,20,202,28,67,21,206,29,61,23,209,30,56,23,213,32,49,25,217,34,41,26,222,36,32,28,226,43,18,31,231,88,234,84,239,79,244,73,250,67,255,1,62,255,7,55,255,15,47,255,23,38,255,36,24,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};
    //uint8_t len = sizeof(frame);
    
    char names[][10] = {"_______0", "_______1", "_______2", "_______3", "_______4", "_______5", "_______6", "_______7", "_______8", "_______9"};
    
    ti_var_t LLV_FILE;
    uint16_t LLV_SIZE;
    uint8_t frame[255];
    uint8_t line = 0;
    
    uint16_t i;
    int x = 0;
    uint8_t y = 0;
    uint8_t VERT_SCALE = 1;
    uint8_t key = 0;
    uint8_t color = 0;
    uint8_t tempColor = 0;
    char *var_name;
    uint8_t *search_pos = NULL;
    uint8_t numFound = 0;
    
    
    
    // Close any files that may be open already
    ti_CloseAll();
    
    
    
    
    
    
    
    
    gfx_Begin();            // Initalize graphics
    //gfx_SetDrawBuffer();    // Enable buffering (because the screen is fully redrawn each frame)
    
    gfx_SetColor(0xFF);
    gfx_FillScreen(74);
    
    // Pick file
    while((var_name = ti_DetectVar(&search_pos, "LLV", TI_APPVAR_TYPE)) != NULL) {
        strcpy(names[numFound], var_name);
        numFound++;
        if(numFound > 10) break;
    }
    
    
select:
    gfx_SetColor(0xFF);     // Setup color swapping
        
    gfx_FillScreen(74);
    for(i = 0; i < numFound; i++) {
        gfx_SetTextXY(10, i*8);
        gfx_PrintString(names[i]);
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
        
        while(!(key = os_GetCSC()));
    }
    
    if(key == sk_Clear) {
        gfx_End();              // End graphics drawing
        return 0;
    }
    

    // Open the selected file.
    LLV_FILE = ti_Open(names[i], "r");
    
    //ti_Read(&frame, 1, sizeof(frame), LLV_FILE);
    
    LLV_SIZE = ti_GetSize(LLV_FILE);
    
    
    while(key != sk_Clear) {
        
        // draw (320x240)
        x = 0;
        y = 0;
        color = 0;
        gfx_SetColor(0xFF);     // Setup color swapping
        
        gfx_FillScreen(74);
        
        
        ti_Seek(3, SEEK_SET, LLV_FILE);
        
        for (i = 0; i < LLV_SIZE; i++) {
            int remainingWidth = LCD_WIDTH - x;
            
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
                color = gfx_SetColor(color);    // To display a line of length 255, encode it as a 255 length line (0xFF) followed by a 0 length line (0xFF).
            }
            
            
            // DEBUG: debug var display
            /*tempColor = gfx_SetColor(74);
            gfx_FillRectangle(0, 200, LCD_WIDTH, 40);
            gfx_SetTextXY(0, 200);
            gfx_PrintUInt(x, 8);
            gfx_SetTextXY(0, 208);
            gfx_PrintUInt(y, 8);
            gfx_SetTextXY(0, 216);
            gfx_PrintUInt(line, 8);
            gfx_SetTextXY(0, 224);
            gfx_PrintUInt(i, 8);
            gfx_SetTextXY(0, 232);
            gfx_PrintInt(LLV_SIZE, 8);
            gfx_SetColor(tempColor);*/
            
            
        }
        
        while(!(key = os_GetCSC()));    // DEBUG: wait for keypress
        if(key == sk_Clear) break;      // DEBUG: quit on clear pressed
        //gfx_SwapDraw();
    }
    
    key = 0;
    goto select;
}