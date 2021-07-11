//
//  random.h
//  spin
//
//  Created by Dave on 09/06/2020.
//  Copyright © 2020 Dave. All rights reserved.
//

#ifndef random_h
#define random_h

#include <stdio.h>

extern int newSeed;
extern unsigned short r256tableBackup[256];
extern unsigned char r256indexBackup;

void updateSeed (unsigned int frameNumber);

void r256init(int seed);

unsigned short r256(void);

unsigned short ranRange(int min, int max);

void backupR256();

void restoreR256();

#endif /* random_h */
