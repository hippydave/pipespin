//
//  game.h
//  PipeSpin
//
//  Created by Dave on 29/05/2021.
//

#ifndef GAME_H
#define GAME_H

#define pipesPerLevel 10
#define pipesPerBlock 10
#define maxLevel 39
#define levelBonus 10
#define crossBonus 10
#define blockBonus 10

// libgba headers
#include <gba.h>
#include <fade.h>

// c headers

// local headers
#include "grid.h"

#ifdef DEBUGGERY
#include "mgba.h"
#endif

extern int keysPressed, keysPressedRepeat, keysReleased, keysHolded;
extern bool canRotate, canMove, puttingBack;
extern int rotating, doGameOver;

#ifdef DEBUGGERY
extern struct cursorCoords coords;
#endif

extern u32 score;
extern u16 pipesCleared, level, maxLoop, nextLevelPipes, nextBlockPipes, startLevel;

//public functions
void pipePlaced (int x, int y);
void gameScreen ();
void setLevel (int levelNumber);

#endif // GAME_H
