//
//  video.h
//  PipeSpin
//
//  Created by Dave on 15/04/2021.
//

#ifndef video_h
#define video_h

#define DMA_HDMA (DMA_ENABLE | DMA_REPEAT | DMA_HBLANK | DMA_DST_RELOAD)
#define OAM_HBL BIT(5)

#define btNumTiles 63
#define btTileOffset btNumTiles * 2
#define btTCount 15
#define btPause 60
#define btMult 4
#define logoX 8 - (8 >> 1)
#define logoY 5

#define gridPrio 1
#define gridXOffset 94
#define gridYOffset 2
#define rotateStep 0x0800
#define rotateAmount 0x4000
#define rotateSteps (rotateAmount / rotateStep)
#define flipSteps 16
#define flipPrio 1
#define pipeTileOffset 0
#define pipeNumTiles 30
#define pipeCharBlock 1
#define blockStartTile (pipeNumTiles << 1)
#define blockNumTiles 24
#define pointerStartTile (blockStartTile + (blockNumTiles << 1))
#define pointerNumTiles 14
#define pointerPrio 1
#define pointerHorizYpos 128
#define pointerVertXpos gridXOffset - 30
#define selectLabelX 222
#define selectLabelY 129
#define lrLabelX 64
#define lrLabelY 138
#define nextBarStartTile (pointerStartTile + (pointerNumTiles << 1))
#define nextBarNumTiles 50
#define nextBarX 224
#define nextBarY 128
#define fontStartTile (nextBarStartTile + (nextBarNumTiles << 0)) //nextBar and font are 16 colour (half size)
#define fontNumTiles 62
#define fontPalNum 15
#define fontAsciiOffset 32
#define fontNumberStart 16
#define floorPrio 3
#define floorPalNum 9
#define floorCharBlock 2
#define queuePrio 3
#define gameUICharBlock 2
#define gameUIPrio 2
#define frontPrio 0

#define scoreX 48
#define scoreY 18
#define pipeCountX scoreX
#define pipeCountY 44
#define levelX scoreX
#define levelY 70
#define maxLoopX scoreX
#define maxLoopY 96
#define startLevelX 49
#define startLevelY 111
#define hiScoreX scoreX
#define hiScoreY 128
#define hiLoopX scoreX
#define hiLoopY 146
#define scoreMaxDigits 7
#define mostLoopScores 4
#define loopScoreTime 120
#define loopScorePalDelay 3

// libgba headers
#include <gba_affine.h>

// c headers
//#include <stdio.h>

// local headers
#include "posprintf.h"
#include "hiscores.h"
#include "menu.h"
#include "game.h"
#include "sound.h"
#include "random.h"
#include "colour.h"
#include "pipes.h"
#include "block.h"
#include "pointer.h"
#include "pointer_nolabel.h"
#include "nextBar.h"
#include "font.h"
#include "font_yellow.h"
#include "font_orange.h"
#include "font_purple.h"
#include "font_green.h"
#include "gameInterface.h"
#include "gameOver.h"
#include "titleSpin.h"
#include "titlePipe.h"
#include "levelScore.h"
#include "clouds.h"
#include "logo.h"
#include "bittwyst.h"
#include "sLUT.h"

#ifdef DEBUGGERY
#include "mgba.h"
#endif

enum OAnumbers { saveIconOA, cursorhOA, cursorvOA, flipTileOA, pointerHorizOA, pointerVertOA, pipeHeldOA, pointerHorizBottomOA, pointerVertBottomOA, selectLabelOA, lrLabelOA, nextBarOA, pathOA, queueOA = pathOA + 16, loopScoreOA = queueOA + 9, addPipeOA = loopScoreOA + (5 * mostLoopScores), scoreCountOA, pipeCountOA = scoreCountOA + 5, levelCountOA = pipeCountOA + 5, maxLoopCountOA = levelCountOA + 2, startLevelOA = maxLoopCountOA + 2, hiScoreCountOA, hiLoopCountOA = hiScoreCountOA + 5, firstFadeSprite = hiLoopCountOA + 2 };
enum objAff { flipObjAff = 1, heldObjAff, queueObjAff };
enum mapNum { bgMap = 0, floorMap = 1, gameUIMap = 2, objMap = 30 };

struct fadeSprite {
    int x;
    int y;
    int gridX;
    int gridY;
    int oA;
};

struct loopScore {
    int rightX;
    int topY;
    int numDigits;
    int timer;
    char scoreString[6];
};

extern bool jamScreen, jamFade, inTitleScreen, lsScreen, titleRotate, inGame, playing, gameOverState, gameQuit, holding;
extern int buttonLabels;
extern int pipeAddCount;
extern int bgOffsetX, bgOffsetY, fgOffsetX, fgOffsetY;

extern OBJATTR addBlockMulti[8][8];
extern OBJAFFINE *ObjAffBuffer;
const extern OBJAFFINE flipOAs[];
const extern int flipYOff[];
extern BGAffineSource bgAS;
extern OBJAFFINE flipOA, pipeShrinkOA, pipeScaleOA[8], pipeQueueOA;
extern ObjAffineSource pipeShrink;

extern u16 rotation;
extern int blockFrame;

extern int logoObjAff;
extern OBJAFFINE logoOA;
extern s16 logoScale;
extern int titleTimer, blendCount, blendTimer;

extern int levelTimer, maxLoopTimer;
extern int saveTimer;

extern int scrollQueue, blockAdd, pipeRemove, pipeHold;
extern struct fadeSprite addPipeSprite, addBlockSprite[], removeBlockSprite[], removePipeSprite[], heldPipeSprite;
extern int nextFadeSprite;

//public functions
void vidInit ();
void VblankInterrupt ();
void doubleTileReg (int x, int y, int tNum, int tOffset, int map, int mapW);
void doubleTileBT (int x, int y, int tnum, int map);
void doubleTile (int x, int y, int tNum, int tOffset, int map, int mapW);
void printText (int x, int y, u8 text[], int pal);
void clearMenu (u32 xPos, u32 yPos, u32 w, u32 h);
void drawBorder (u32 xPos, u32 yPos, u32 w, u32 h);
void mosaicAllSprites ();
void disableAllSprites ();
void disableSprite (int oaNumber, OBJATTR buffer[]);
void setSprite (int oaNumber, int xPos, int yPos, int charNum, int palNum, int prio, bool blend, bool mosaic, int size);
void showLoopScore (int x, int y, int score);
void copyGraphics ();
void copyCannons ();
void setPalettes ();
void copyGameOver ();
void copyJam ();
void copyBittwyst ();
void copyTitle ();
void copyLevelScore ();

#endif /* video_h */
