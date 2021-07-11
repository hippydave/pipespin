//
//  colour.c
//  PipeSpin
//
//  Created by Dave on 17/05/2021.
//

// local headers
#include "colour.h"

int correctCol = 1;
int gammaLevel = 4;

//---------------------------------------------------------------------------------
void processPalette(u16 *dst, const u16 *src, u32 nclrs)
//---------------------------------------------------------------------------------
{
    u32 ii, clr;
    int r, g, b;
    for(ii=0; ii<nclrs; ii++)
    {
        clr= src[ii];
        if (correctCol == 1)
            clr = cc[clr];
        r = clr & 0x1f;
        g = (clr >> 5) & 0x1f;
        b = (clr >> 10) & 0x1f;
        r = gLUT[(32 * gammaLevel) + r];
        g = gLUT[(32 * gammaLevel) + g];
        b = gLUT[(32 * gammaLevel) + b];


        dst[ii] = (r&31) + ((g&31)<<5) + ((b&31)<<10);
    }
}//void processPalette

