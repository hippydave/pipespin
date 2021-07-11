//
//  hiscores.h
//  PipeSpin
//
//  Created by Dave on 03/07/2021.
//

#ifndef HISCORES_H
#define HISCORES_H

#define lsScoreX 192
#define lsScoreY 113
#define lsScoreYOff 11
#define lsNameX 52
#define lsMaxLoopX 160
#define lsMaxLoopY 99
#define lsLevelX 90
#define lsLevelXOff 13
#define lsLevelY1 42
#define lsLevelY2 56

#define scoreSramStart 0x100
#define sx 69

// local headers
#include "video.h"

#ifdef DEBUGGERY
#include "mgba.h"
#endif

struct levelHiScore {
    char name[3][8];
    u32 score[3];
    u16 maxLoop;
};

extern bool levelSelect, nameDone, gameEnded;
extern int lCursorX, lCursorY, currentHi, currentChar;
extern struct levelHiScore levelScores[];

extern int saveIndicatorSetting;

//public functions
void initHiScores ();
void updateSettings ();
bool levelScoreScreen ();
void checkSave ();

#endif // HISCORES_H
