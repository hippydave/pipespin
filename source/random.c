//
//  random.c
//  spin
//
//  Created by Dave on 09/06/2020.
//  Copyright © 2020 Dave. All rights reserved.
//

// libgba headers
#include <gba.h>

// local headers
#include "random.h"
#include "video.h"

int newSeed = 0;
u8 byteVC;
unsigned short r256table[256];//, r256tableBackup[256];
unsigned char r256index;//, r256indexBackup;


//---------------------------------------------------------------------------------
void updateSeed (unsigned int frameNumber)
//---------------------------------------------------------------------------------
{
    byteVC = (u8) frameNumber;
    byteVC &= 63;
    newSeed <<= 6;
    newSeed |= byteVC;
}//void updateSeed

//---------------------------------------------------------------------------------
void r256init (int seed)
//---------------------------------------------------------------------------------
{
    int i,j,msb;
    j=seed;
    for(i=0;i<256;i++)
    {
        r256table[i]=(unsigned short)(j=j*65539);
    }
    
    msb=0x8000;
    
    j=0;
    
    for(i=0;i<16;i++)
    {
        j=i*5+3;
        r256table[j]|=msb;
        r256table[j+1]&=~msb;
        msb>>=1;
    }
}//void r256init

//---------------------------------------------------------------------------------
unsigned short r256 (void)
//---------------------------------------------------------------------------------
{
    int r;
    r=r256table[(r256index+103)&255]^r256table[r256index];
    r256table[r256index++]=r;
    return r;
}//unsigned short r256

//---------------------------------------------------------------------------------
unsigned short ranRange (int min, int max)
//---------------------------------------------------------------------------------
{
    unsigned short r = r256() & 0x7fff;
    return (r*(max-min)>>15)+min;
}//unsigned short ranRange

/*
//---------------------------------------------------------------------------------
void backupR256 ()
//---------------------------------------------------------------------------------
{
    r256indexBackup = r256index;
    for (int i = 0; i < 256; i++) {
        r256tableBackup[i] = r256table[i];
    }
}//void backupR256

//---------------------------------------------------------------------------------
void restoreR256 ()
//---------------------------------------------------------------------------------
{
    r256index = r256indexBackup;
    for (int i = 0; i < 256; i++) {
        r256table[i] = r256tableBackup[i];
    }
}//void restoreR256
*/
