//
//  colour.h
//  PipeSpin
//
//  Created by Dave on 17/05/2021.
//

#ifndef COLOUR_H
#define COLOUR_H

// libgba headers
#include <gba.h>

// local headers
#include "gLUT.h"
#include "cc.h"

extern int correctCol, gammaLevel;

void processPalette(u16 *dst, const u16 *src, u32 nclrs);

#endif // COLOUR_H
