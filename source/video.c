//
//  video.c
//  PipeSpin
//
//  Created by Dave on 15/04/2021.
//

// local headers
#include "video.h"

const u16 btpalette[] = {
    RGB8(0x00,0x00,0x64),
    RGB8(0xFF,0xFF,0x00),
    RGB8(0xFF,0xFF,0x80)
};

unsigned int frame = 0;
u16 keysOld = 0x03FF, keysNew = 0x03FF;
bool jamScreen = false, jamFade = false, inTitleScreen = false, lsScreen = false, titleRotate = false, inGame = false, playing = false, gameOverState = false, gameQuit = false, holding = false;
int buttonLabels = 1, oldbuttonLabels = 1;
int textPal = 0;
int pipeAddCount, barNum;
int bgOffsetX = 0, bgOffsetY = 0, fgOffsetX = 0, fgOffsetY = 0;
OBJATTR OAMBuffer[128];
OBJAFFINE *ObjAffBuffer = (OBJAFFINE*)OAMBuffer;

const int scoreMultiLine[8] = { 155, 27, 53, 79, 105, 120, 138 }; //only need 7

const OBJAFFINE flipOAs[8] = { { { 0, 0, 0}, 0x100, { 0, 0, 0}, 0, { 0, 0, 0}, -0x000, { 0, 0, 0}, 0x100 },
                         { { 0, 0, 0}, 0x130, { 0, 0, 0}, 0, { 0, 0, 0}, -0x080, { 0, 0, 0}, 0x100 },
                         { { 0, 0, 0}, 0x180, { 0, 0, 0}, 0, { 0, 0, 0}, -0x100, { 0, 0, 0}, 0x100 },
                         { { 0, 0, 0}, 0x1a0, { 0, 0, 0}, 0, { 0, 0, 0}, -0x180, { 0, 0, 0}, 0x100 },
                         { { 0, 0, 0}, 0x280, { 0, 0, 0}, 0, { 0, 0, 0}, -0x200, { 0, 0, 0}, 0x100 },
                         { { 0, 0, 0}, 0x300, { 0, 0, 0}, 0, { 0, 0, 0}, -0x280, { 0, 0, 0}, 0x100 },
                         { { 0, 0, 0}, 0x500, { 0, 0, 0}, 0, { 0, 0, 0}, -0x300, { 0, 0, 0}, 0x100 },
                         { { 0, 0, 0}, 0x801, { 0, 0, 0}, 0, { 0, 0, 0}, -0x380, { 0, 0, 0}, 0x100 } };
const int flipYOff[8] = { 0, -3, -5, -6, -6, -6, -6, -6 };

BGAffineSource bgAS;

OBJAFFINE flipOA, pipeShrinkOA, pipeScaleOA[8], pipeQueueOA;
ObjAffineSource pipeShrink = { 0b101000000, 0b101000000, 0 };
ObjAffineSource pipeQueue = { 0b100100000, 0b100100000, 0 };
ObjAffineSource pipeScale[8];
u16 rotation;
u16 mosaic = 0, floorMosaic = 1, floorMosaicDir = 1;
int mosaicWait = 0, mosaicDelay = 10, toggle = 0, blockFrame = 0;

u16 waveX[SCREEN_HEIGHT + 1];
int fCount = 0;
int scaleX = 2;
s16 logoScale;
s16 logoScaleStep = 2;
bool logoScaled = false;
int titleTimer, blendCount, blendTimer;

char scoreString[scoreMaxDigits + 1];
char pipesString[6];
char levelString[6];
char maxLoopString[3];
char hiScoreString[scoreMaxDigits + 1];
char hiMaxLoopString[3];
char nameString[scoreMaxDigits + 1];
const char scorePrintf[] = { '%', scoreMaxDigits + 48, 'l', 0 };
int scorePal, hiScorePal = -2, loopScorePal, loopScorePalTimer = 0, namePal, levelTimer, maxLoopTimer;
int saveTimer;

struct loopScore lScore[mostLoopScores];

int scrollQueue = 0, blockAdd = 0, pipeRemove = 0, pipeHold = 0;
struct fadeSprite addPipeSprite, addBlockSprite[maxBlocks], removeBlockSprite[maxBlocks], removePipeSprite[gridSquares], heldPipeSprite;
int nextFadeSprite;

//---------------------------------------------------------------------------------
void vidInit ()
//---------------------------------------------------------------------------------
{
    //pipe scaling for queue/held
    ObjAffineSet(&pipeShrink, &pipeShrinkOA.pa, 1, 8);
    ObjAffBuffer[heldObjAff].pa = pipeShrinkOA.pa;
    ObjAffBuffer[heldObjAff].pb = pipeShrinkOA.pb;
    ObjAffBuffer[heldObjAff].pc = pipeShrinkOA.pc;
    ObjAffBuffer[heldObjAff].pd = pipeShrinkOA.pd;
    ObjAffineSet(&pipeQueue, &pipeQueueOA.pa, 1, 8);
    ObjAffBuffer[queueObjAff].pa = pipeQueueOA.pa;
    ObjAffBuffer[queueObjAff].pb = pipeQueueOA.pb;
    ObjAffBuffer[queueObjAff].pc = pipeQueueOA.pc;
    ObjAffBuffer[queueObjAff].pd = pipeQueueOA.pd;
    for (int i = 0; i < 8; i++) {
        pipeScale[i].sX = pipeScale[i].sY = 256 + (32 * i);
        pipeScale[i].theta = 0;
        ObjAffineSet(&pipeScale[i], &pipeScaleOA[i].pa, 1, 8);
    }

    for (int i = 0; i < mostLoopScores; i++) {
        lScore[i].timer = -1;
    }
    loopScorePal = -1;
    for (int i = 0; i < maxBlocks; i++) {
        addBlockSprite[i].x = -1;
        removeBlockSprite[i].x = -1;
    }
    for (int i = 0; i < gridSquares; i++) {
        removePipeSprite[i].x = -1;
    }
    nextFadeSprite = firstFadeSprite;
#ifdef DEBUGGERY
    //mgba_printf(MGBA_LOG_INFO, "scoreCountOA: %i, addPipeOA: %i, addBlockOA: %i, removeBlockOA: %i, removePipeOA: %i, whatevercomesnext: %i", scoreCountOA, addPipeOA, addBlockOA, removeBlockOA, removePipeOA, whatevercomesnext);
#endif
}//void vidInit

//---------------------------------------------------------------------------------
void drawJamScreen ()
//---------------------------------------------------------------------------------
{
    //horizontal wave
    if (jamFade) scaleX = 1;
    fCount++;
    if (fCount == 100) fCount = 0;

    for (int i = 0; i < SCREEN_HEIGHT + 1; i++) {
        waveX[i] = (sLUT[fCount + i] / scaleX) - ((2 - scaleX) * 8);
    }

    //logo
    if (!jamFade) {
        logoScale += logoScaleStep;
        if (logoScale > (1 << 8)) {
            logoScale = 1 << 8;
            logoScaled = true;
        }
        if (bgAS.theta != 0) {
            if (!logoScaled)
                bgAS.theta = 0xffff - (frame << 11);
            else
                bgAS.theta = 0;
        }
    } else {
        logoScale += logoScaleStep << 6;
    }
    bgAS.sX = bgAS.sY = logoScale;
    BgAffineSet(&bgAS, (BGAffineDest*)&REG_BG2PA, (s32) 1);

    REG_DMA3CNT = 0;
    DMA3COPY(&waveX[1], &REG_BG1HOFS, 1 | DMA_HDMA);
}//void drawJamScreen

//---------------------------------------------------------------------------------
void drawTitleScreen ()
//---------------------------------------------------------------------------------
{
    if (blendCount > 8) {
        if (blendTimer == 8) {
            blendTimer = 0;
            blendCount--;
            REG_BLDY = blendCount;
        } else {
            blendTimer++;
        }
    }
    if (titleRotate) {
        titleTimer += 1;
        bgAS.theta -= 837; //spin in time with music
        if (bgAS.theta == 250) {
            bgAS.theta = 0;
            titleRotate = false;
        }
    } else {
        titleTimer -= 1;
        if (titleTimer == 0) {
            titleRotate = true;
        }
    }
    BgAffineSet(&bgAS, (BGAffineDest*)&REG_BG2PA, (s32) 1);
}//void drawTitleScreen

//---------------------------------------------------------------------------------
void drawLsScreen ()
//---------------------------------------------------------------------------------
{
    if (blendCount > 8) {
        if (blendTimer == 8) {
            blendTimer = 0;
            blendCount--;
            REG_BLDY = blendCount;
        } else {
            blendTimer += 4;
        }
    }
    if (levelSelect) {
        //highlight selected number
        setSprite(1, lsLevelX + (lCursorX * lsLevelXOff), lCursorY == 0 ? lsLevelY1 : lsLevelY2, (fontStartTile << 2) + startLevel + fontNumberStart, fontPalNum + loopScorePal, gameUIPrio, false, false, 4);
    }
    //print scores
    posprintf(maxLoopString, "%2d", levelScores[startLevel].maxLoop);
    for (int j = 0; j < 3; j++) {
        if (levelScores[startLevel].score[j] > 0) {
            posprintf(scoreString, scorePrintf, levelScores[startLevel].score[j]);
            posprintf(nameString, "%s", levelScores[startLevel].name[j]);
        } else {
            for (int i = 0; i < scoreMaxDigits; i++) {
                scoreString[i] = '.';
                nameString[i] = '.';
            }
            scoreString[scoreMaxDigits] = 0;
            nameString[scoreMaxDigits] = 0;
        }

        bool stringEnded = false;
        for (int i = 0; i < scoreMaxDigits; i++) {
            //name
            if (currentHi == j) {
                if (!levelSelect && !nameDone) {
                    if (currentChar == i)
                        namePal = loopScorePal;
                    else
                        namePal = 0;
                } else if (levelTimer > 0) {
                    namePal = loopScorePal;
                } else {
                    namePal = 0;
                }
            } else {
                namePal = 0;
            }
            if ((nameString[i] == 0) || (stringEnded)) {
                stringEnded = true;
                disableSprite(23 + i + (j * scoreMaxDigits), NULL);
            } else {
                setSprite(23 + i + (j * scoreMaxDigits), lsNameX + (i * 10), lsScoreY + (j * lsScoreYOff), (fontStartTile << 2) + nameString[i] - fontAsciiOffset, fontPalNum + namePal, gameUIPrio, false, false, 4);
            }
            //score
            if (scoreString[scoreMaxDigits - 1 - i] == ' ') {
                disableSprite(2 + i + (j * scoreMaxDigits), NULL);
            } else {
                setSprite(2 + i + (j * scoreMaxDigits), lsScoreX - (i * 10), lsScoreY + (j * lsScoreYOff), (fontStartTile << 2) + scoreString[scoreMaxDigits - 1 - i] - fontAsciiOffset, fontPalNum + ((currentHi == j) && (levelTimer > 0) ? loopScorePal : 0), gameUIPrio, false, false, 4);
            }
            //maxLoop
            if ((j == 0) && (i < 2)) {
                if (maxLoopString[1 - i] == ' ') {
                    disableSprite(44 + i, NULL);
                } else {
                    setSprite(44 + i, lsMaxLoopX - (i * 10), lsMaxLoopY, (fontStartTile << 2) + maxLoopString[1 - i] - fontAsciiOffset, fontPalNum + (maxLoopTimer > 0 ? loopScorePal : 0), gameUIPrio, false, false, 4);
                }
            }
        }
    }
}//void drawLsScreen

//---------------------------------------------------------------------------------
void drawScore ()
//---------------------------------------------------------------------------------
{
    //score display
    posprintf(scoreString, scorePrintf, score);
    posprintf(pipesString, "%5d", pipesCleared);
    posprintf(levelString, "%2d", level);
    posprintf(maxLoopString, "%2d", maxLoop);
    posprintf(hiScoreString, scorePrintf, levelScores[startLevel].score[0]);
    posprintf(hiMaxLoopString, "%2d", levelScores[startLevel].maxLoop);
    if (scoreString[scoreMaxDigits - 5 - 1] == ' ') {
        scorePal = 0;
    } else {
        scorePal = -1;
    }
    for (int i = 0; i < 5; i++) {
        if (scoreString[scoreMaxDigits - 1 - i] == ' ') {
            disableSprite(scoreCountOA + i, NULL);
        } else {
            setSprite(scoreCountOA + i, scoreX - (i * 10), scoreY, (fontStartTile << 2) + scoreString[scoreMaxDigits - 1 - i] - fontAsciiOffset, fontPalNum + scorePal, gameUIPrio, false, false, 4);
        }
        if (pipesString[4 - i] == ' ') {
            disableSprite(pipeCountOA + i, NULL);
        } else {
            setSprite(pipeCountOA + i, pipeCountX - (i * 10), pipeCountY, (fontStartTile << 2) + pipesString[4 - i] - fontAsciiOffset, fontPalNum, gameUIPrio, false, false, 4);
        }
        if (i < 2) { //level and maxLoop/hiMaxLoop are 2 digits
            if (levelString[1 - i] == ' ') {
                disableSprite(levelCountOA + i, NULL);
            } else {
                setSprite(levelCountOA + i, levelX - (i * 10), levelY, (fontStartTile << 2) + levelString[1 - i] - fontAsciiOffset, fontPalNum + (levelTimer > 0 ? loopScorePal : 0), gameUIPrio, false, false, 4);
            }
            if (maxLoopString[1 - i] == ' ') {
                disableSprite(maxLoopCountOA + i, NULL);
            } else {
                setSprite(maxLoopCountOA + i, maxLoopX - (i * 10), maxLoopY, (fontStartTile << 2) + maxLoopString[1 - i] - fontAsciiOffset, fontPalNum + (maxLoopTimer > 0 ? loopScorePal : 0), gameUIPrio, false, false, 4);
            }
            if (hiMaxLoopString[1 - i] == ' ') {
                disableSprite(hiLoopCountOA + i, NULL);
            } else {
                setSprite(hiLoopCountOA + i, hiLoopX - (i * 10), hiLoopY, (fontStartTile << 2) + hiMaxLoopString[1 - i] - fontAsciiOffset, fontPalNum + hiScorePal, gameUIPrio, false, false, 4);
            }
        }
        if (hiScoreString[scoreMaxDigits - 1 - i] == ' ') {
            disableSprite(hiScoreCountOA + i, NULL);
        } else {
            setSprite(hiScoreCountOA + i, hiScoreX - (i * 10), hiScoreY, (fontStartTile << 2) + hiScoreString[scoreMaxDigits - 1 - i] - fontAsciiOffset, fontPalNum + hiScorePal, gameUIPrio, false, false, 4);
        }
        for (int j = 0; j < mostLoopScores; j++) {//loop score (in playgrid)
            if (lScore[j].timer != -1) {
                if (i == 0) lScore[j].timer--; //only decrement once, not once per digit
                if (lScore[j].scoreString[4 - i] == ' ') {
                    disableSprite(loopScoreOA + (5 * j) + i, NULL);
                } else {
                    setSprite(loopScoreOA + (5 * j) + i, lScore[j].rightX - (i * 8) + 8, lScore[j].topY + 8, (fontStartTile << 2) + lScore[j].scoreString[4 - i] - fontAsciiOffset, fontPalNum + loopScorePal, frontPrio, false, false, 4);
                }
            } else {
                disableSprite(loopScoreOA + (5 * j) + i, NULL);
            }
        }
    }
    //start level
    setSprite(startLevelOA, startLevelX, startLevelY, (fontStartTile << 2) + startLevel + fontNumberStart, fontPalNum, gameUIPrio, false, false, 4);
}//void drawScore

//---------------------------------------------------------------------------------
void drawAnimations ()
//---------------------------------------------------------------------------------
{
    //animations
    if (pipeHold > 0) { //pipe being picked up/put back
        setSprite(pipeHeldOA, heldPipeSprite.x, heldPipeSprite.y + ((8 - pipeHold) * heldPipeSprite.gridY), pipeTileOffset + (holdingPipe.tile << 1), -heldObjAff, pointerPrio, false, false, 0);
        ObjAffineSet(&pipeShrink, &pipeShrinkOA.pa, 1, 8);
        ObjAffBuffer[heldObjAff].pa = pipeScaleOA[8 - pipeHold].pa;
        ObjAffBuffer[heldObjAff].pb = pipeScaleOA[8 - pipeHold].pb;
        ObjAffBuffer[heldObjAff].pc = pipeScaleOA[8 - pipeHold].pc;
        ObjAffBuffer[heldObjAff].pd = pipeScaleOA[8 - pipeHold].pd;
        pipeHold--;
        if (pipeHold == 0) {
            canMove = true;
            canRotate = true;
        }
    } else if (scrollQueue > 0) { //pipe adding to playfield, queue scrolling
        REG_MOSAIC = (scrollQueue >> 2) << 8 | (scrollQueue >> 2) << 12;
        REG_BLDCNT = (0b000010 << 8) | (0b01 << 6) | (0b010000); //BOT (BG1) | normal blend | TOP (Obj)
        REG_BLDALPHA = (0b11111 << 8) | ((16 - (scrollQueue >> 0)) << 1); //BOT weight | TOP weight
        drawQueue();
        scrollQueue--;
        if (scrollQueue == 0) { //finish up
            disableSprite(addPipeOA, NULL);
            doubleTile(addPipeSprite.gridX, addPipeSprite.gridY, grid[addPipeSprite.gridX][addPipeSprite.gridY].tile, pipeTileOffset, bgMap, 16);
            nextPipe ();
            canRotate = true;
            //blending for beams
            REG_BLDCNT = (0b000011 << 8) | (0b01 << 6) | (0b010000); //BOT (BG1 BG0) | normal blend | TOP (Obj)
            REG_BLDALPHA = (0b11111 << 8) | (0b00100); //BOT weight | TOP weight
        }
    } else if (pipeRemove > 0) { //pipes & blocks removing from playfield
        REG_MOSAIC = ((16 - pipeRemove) >> 2) << 8 | ((16 - pipeRemove) >> 2) << 12;
        REG_BLDCNT = (0b000010 << 8) | (0b01 << 6) | (0b010000); //BOT (BG1) | normal blend | TOP (Obj)
        REG_BLDALPHA = (0b11111 << 8) | ((16 - ((16 - pipeRemove) >> 0)) << 1); //BOT weight | TOP weight
        pipeRemove--;
        if (pipeRemove == 0) { //finish up
            //do pipes
            for (int i = 0; i < gridSquares; i++) {
                if (removePipeSprite[i].x != -1) {
                    disableSprite(removePipeSprite[i].oA, NULL);
                    removePipeSprite[i].x = -1;
                }
            }
            //do blocks
            for (int i = 0; i < maxBlocks; i++) {
                if (removeBlockSprite[i].x != -1) {
                    disableSprite(removeBlockSprite[i].oA, NULL);
                    removeBlockSprite[i].x = -1;
                }
            }
            canRotate = true;
            //blending for beams
            REG_BLDCNT = (0b000011 << 8) | (0b01 << 6) | (0b010000); //BOT (BG1 BG0) | normal blend | TOP (Obj)
            REG_BLDALPHA = (0b11111 << 8) | (0b00100); //BOT weight | TOP weight
        } else {
            //update block sprites
            for (int i = 0; i < maxBlocks; i++) {
                if (removeBlockSprite[i].x != -1) {
                    setSprite(removeBlockSprite[i].oA, removeBlockSprite[i].x, removeBlockSprite[i].y, (blockTile + blockFrame) << 1, -255, flipPrio, true, true, 0);
                }
            }
        }
    } else if (blockAdd > 0) { //block adding to playfield
        REG_MOSAIC = (blockAdd >> 2) << 8 | (blockAdd >> 2) << 12;
        REG_BLDCNT = (0b000010 << 8) | (0b01 << 6) | (0b010000); //BOT (BG1) | normal blend | TOP (Obj)
        REG_BLDALPHA = (0b11111 << 8) | ((16 - (blockAdd >> 0)) << 1); //BOT weight | TOP weight
        blockAdd--;
        if (blockAdd == 0) { //finish up
            for (int i = 0; i < maxBlocks; i++) {
                if (addBlockSprite[i].x != -1) {
                    disableSprite(addBlockSprite[i].oA, NULL);
                    doubleTile(addBlockSprite[i].gridX, addBlockSprite[i].gridY, blockTile + blockFrame, pipeTileOffset, bgMap, 16);
                    blocks[i].colour = 0;
                    addBlockSprite[i].x = -1;
                }
            }
            canRotate = true;
            //blending for beams
            REG_BLDCNT = (0b000011 << 8) | (0b01 << 6) | (0b010000); //BOT (BG1 BG0) | normal blend | TOP (Obj)
            REG_BLDALPHA = (0b11111 << 8) | (0b00100); //BOT weight | TOP weight
        } else {
            //update sprites
            for (int i = 0; i < maxBlocks; i++) {
                if (addBlockSprite[i].x != -1) {
                    setSprite(addBlockSprite[i].oA, addBlockSprite[i].x, addBlockSprite[i].y, (blockTile + blockFrame) << 1, -255, flipPrio, true, true, 0);
                }
            }
        }
    }//end of animations
}//void drawAnimations

//---------------------------------------------------------------------------------
void drawGame ()
//---------------------------------------------------------------------------------
{
    toggle = 1 - toggle;

    if (buttonLabels != oldbuttonLabels) {
        copyCannons();
        oldbuttonLabels = buttonLabels;
    }

    if (doGameOver == 2) {
        rotation += rotateStep >> 1;
        bgAS.sX += logoScale;
        bgAS.sY += logoScale;
        logoScale += 5;
        if (bgAS.sX > 15000) doGameOver = 3;
    } else if (doGameOver == 4) {
        bgAS.sX -= logoScale;
        bgAS.sY -= logoScale;
        if (bgAS.sX < 1 << 8) {
            bgAS.sX = 1 << 8;
            bgAS.sY = 1 << 8;
            doGameOver = 5;
        }
    }
    //set play grid rotation
    bgAS.theta = rotation;
    if (doGameOver == 0) {
        switch (rotation) { //tweak source centre offset to fix position after rotating
            case 0x4000:
                bgAS.x = (s32) 63 << 8;
                bgAS.y = (s32) 64 << 8;
                break;
            case 0x8000:
                bgAS.x = (s32) 63 << 8;
                bgAS.y = (s32) 63 << 8;
                break;
            case 0xc000:
                bgAS.x = (s32) 64 << 8;
                bgAS.y = (s32) 63 << 8;
                break;
            default:
                bgAS.x = (s32) 64 << 8;
                bgAS.y = (s32) 64 << 8;
                break;
        }
    }
    BgAffineSet(&bgAS, (BGAffineDest*)&REG_BG2PA, (s32) 1);

    //set "flip" transformation
    ObjAffBuffer[flipObjAff].pa = flipOA.pa;
    ObjAffBuffer[flipObjAff].pb = flipOA.pb;
    ObjAffBuffer[flipObjAff].pc = flipOA.pc;
    ObjAffBuffer[flipObjAff].pd = flipOA.pd;

    mosaicWait++;
    if (mosaicWait == mosaicDelay) {
        mosaicWait = 0;
        mosaic++;
        //*
        floorMosaic += floorMosaicDir;
        if (floorMosaic == 2) floorMosaic += floorMosaicDir;
        if ((floorMosaic == 3) || (floorMosaic == 0)) floorMosaicDir = 0 - floorMosaicDir;
        //*/
        //adjust mosaic
        if (mosaic == 0b10) mosaic = 0;
        if (scrollQueue == 0) {
            REG_MOSAIC = mosaic << 8 | mosaic << 12 | (3 - floorMosaic) << 4 | floorMosaic;
        }
        if (doGameOver == 0) {
            //animate blocks
            blockFrame = (blockFrame == 15? 0: blockFrame + 1);
            for (int i = 0; i < maxBlocks; i++) {
                if ((blocks[i].x != -1) && (blocks[i].colour != -1)) {
                    doubleTile(blocks[i].x, blocks[i].y, blockTile + blockFrame, pipeTileOffset, bgMap, 16);
                }
            }
        }
    }

    //button labels
    if (buttonLabels == 1) {
        setSprite(selectLabelOA, selectLabelX, selectLabelY, pointerStartTile + 20, -255, pointerPrio, false, false, 1);
        setSprite(lrLabelOA, lrLabelX, lrLabelY, pointerStartTile + 24, -255, pointerPrio, false, false, 1);
    } else {
        disableSprite(selectLabelOA, NULL);
        disableSprite(lrLabelOA, NULL);
    }

    if (doGameOver == 0) {
        mmSetModuleTempo(1024 + (5 * squaresFilled));//speed up music as grid fills up
        drawScore();
    }

    if (playing) {
        //new pipe piece timer
        if ((scrollQueue == 0) && (pipeAddCount != pipeAddDelay))
            pipeAddCount++;
        barNum = (int) (double)(pipeAddCount) / (double)(pipeAddDelay) * (double)25;
        if (barNum == 25) barNum = 24;
        setSprite(nextBarOA, nextBarX, nextBarY, nextBarStartTile + (barNum << 1), fontPalNum, pointerPrio, true, false, 2);

        if ((pipeAddCount == pipeAddDelay) && (rotating == 0)) { //timer finished
            pipeAddCount = 0;
            if (!addPipe()) {
                //fail
                playing = false;
                gameOverState = true;
                doGameOver = 1;
#ifdef DEBUGGERY
                mgba_printf(MGBA_LOG_INFO, "Game over!");
#endif
            }
        }
        drawAnimations();
    }//if (playing)
}//void drawgame

//---------------------------------------------------------------------------------
void drawThing ()
//---------------------------------------------------------------------------------
{
}//void drawThing

//---------------------------------------------------------------------------------
void VblankInterrupt ()
//---------------------------------------------------------------------------------
{
#ifdef DEBUGGERY
    //mgba_printf(MGBA_LOG_INFO, "inGame: %s, playing: %s, gameOverState: %s", inGame ? "true" : "false", playing ? "true" : "false", gameOverState ? "true" : "false");
#endif
    mmFrame();
    frame += 1;

    //save indicator
    if (saveTimer > 0) {
        setSprite(saveIconOA, 0, 0, pipeTileOffset + (104 << 1), -255, frontPrio, false, false, 0);
        saveTimer--;
    } else {
        disableSprite(saveIconOA, NULL);
    }

    //timers
    loopScorePalTimer += 1;
    if (loopScorePalTimer == loopScorePalDelay) {
        loopScorePalTimer = 0;
        loopScorePal -= 1;
        if (loopScorePal == -5) loopScorePal = -1;
    }
    if (levelTimer > 0) levelTimer--;
    if (maxLoopTimer > 0) maxLoopTimer--;

    //input
    scanKeys();
    keysNew = REG_KEYINPUT;
    if (keysNew != keysOld) updateSeed(frame);
    keysOld = keysNew;

    //screen specific stuff
    if (jamScreen) {
        drawJamScreen();
    } else if (inTitleScreen){
        drawTitleScreen();
    } else if (lsScreen){
        drawLsScreen();
    } else if (inGame){
        drawGame();
    }

    //update sprite OAM from buffer
    CpuFastSet(&OAMBuffer, (u16*)OAM,(sizeof(OAMBuffer) >> 2) | COPY32);
}//void VblankInterrupt


//---------------------------------------------------------------------------------
void doubleTileReg (int x, int y, int tNum, int tOffset, int map, int mapW)
//---------------------------------------------------------------------------------
{
    x <<= 1;
    y <<= 1;
    tNum <<= 1;
    int offset = x + (mapW * y);
    *((u16 *)MAP_BASE_ADR(map) + offset) = ((tNum + tOffset)* 2) + 0;
    *((u16 *)MAP_BASE_ADR(map) + offset + 1) = ((tNum + tOffset) * 2) + 1;
    *((u16 *)MAP_BASE_ADR(map) + offset + 32) = ((tNum + tOffset) * 2) + 2;
    *((u16 *)MAP_BASE_ADR(map) + offset + 33) = ((tNum + tOffset) * 2) + 3;

}//void doubleTileReg

//---------------------------------------------------------------------------------
void doubleTileBT (int x, int y, int tnum, int map)
//---------------------------------------------------------------------------------
{
    x <<= 1;
    y <<= 1;
    int offset = x + (32 * y);
    *((u16 *)MAP_BASE_ADR(map) + offset) = tnum * 2;
    *((u16 *)MAP_BASE_ADR(map) + offset + 1) = (tnum * 2) + 1;
    *((u16 *)MAP_BASE_ADR(map) + offset + 32) = (tnum * 2) + btTileOffset;
    *((u16 *)MAP_BASE_ADR(map) + offset + 33) = (tnum * 2) + btTileOffset + 1;
}//void doubleTileBT

//---------------------------------------------------------------------------------
void doubleTile (int x, int y, int tNum, int tOffset, int map, int mapW)
//---------------------------------------------------------------------------------
{
    int offset = x + (mapW * y);
    *((u16 *)MAP_BASE_ADR(map) + offset + 0 + 0) = (u16)((u8)((tNum * 4) + 0 + tOffset) | (u16)((u8)((tNum * 4) + 1 + tOffset)) << 8);
    *((u16 *)MAP_BASE_ADR(map) + offset + 0 + (mapW >> 1)) = (u16)((u8)((tNum * 4) + 2 + tOffset) | (u16)((u8)((tNum * 4) + 3 + tOffset)) << 8);
}//void doubleTile

//---------------------------------------------------------------------------------
void printText (int x, int y, u8 text[], int pal)
//---------------------------------------------------------------------------------
{
    textPal = 0;
    int offset = x + (32 * y);
    int c = 0, xc = 0;
    while (text[c] != 0) {
        if (text[c] > 'Z') { //lowercase a-e = colours
            textPal = 97 - text[c];
        } else {
            *((u16 *)MAP_BASE_ADR(floorMap) + offset + xc) = ((pointerStartTile << 2) - fontAsciiOffset + text[c]) | CHAR_PALETTE(pal + textPal);
            xc++;
        }
        c++;
    }
}//void printText

//---------------------------------------------------------------------------------
void clearMenu (u32 xPos, u32 yPos, u32 w, u32 h)
//---------------------------------------------------------------------------------
{
    u32 x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            *((u16 *)MAP_BASE_ADR(floorMap) + (x + xPos) + (32 * (y + yPos))) = pointerStartTile << 2;
        }
    }
}//void clearMenu

//---------------------------------------------------------------------------------
void drawBorder (u32 xPos, u32 yPos, u32 w, u32 h)
//---------------------------------------------------------------------------------
{
    u32 x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (x == 0) {
                if (y == 0) {
                    *((u16 *)MAP_BASE_ADR(floorMap) + (x + xPos) + (32 * (y + yPos))) = ((pointerStartTile << 2) + cornerBorder) | CHAR_PALETTE(15);
                } else if (y == h - 1) {
                    *((u16 *)MAP_BASE_ADR(floorMap) + (x + xPos) + (32 * (y + yPos))) = ((pointerStartTile << 2) + cornerBorder) | CHAR_PALETTE(15) | (1 << 11);
                } else {
                    *((u16 *)MAP_BASE_ADR(floorMap) + (x + xPos) + (32 * (y + yPos))) = ((pointerStartTile << 2) + vertBorder) | CHAR_PALETTE(15);
                }
            } else if (x == w - 1) {
                if (y == 0) {
                    *((u16 *)MAP_BASE_ADR(floorMap) + (x + xPos) + (32 * (y + yPos))) = ((pointerStartTile << 2) + cornerBorder) | CHAR_PALETTE(15) | (1 << 10);
                } else if (y == h - 1) {
                    *((u16 *)MAP_BASE_ADR(floorMap) + (x + xPos) + (32 * (y + yPos))) = ((pointerStartTile << 2) + cornerBorder) | CHAR_PALETTE(15) | (1 << 11) | (1 << 10);
                } else {
                    *((u16 *)MAP_BASE_ADR(floorMap) + (x + xPos) + (32 * (y + yPos))) = ((pointerStartTile << 2) + vertBorder) | CHAR_PALETTE(15) | (1 << 10);
                }
            } else {
                if (y == 0) {
                    *((u16 *)MAP_BASE_ADR(floorMap) + (x + xPos) + (32 * (y + yPos))) = ((pointerStartTile << 2) + horizBorder) | CHAR_PALETTE(15);
                } else if (y == h - 1) {
                    *((u16 *)MAP_BASE_ADR(floorMap) + (x + xPos) + (32 * (y + yPos))) = ((pointerStartTile << 2) + horizBorder) | CHAR_PALETTE(15) | (1 << 11);
                }
            }
        }
    }
}//void drawBorder

//---------------------------------------------------------------------------------
void mosaicAllSprites ()
//---------------------------------------------------------------------------------
{
    int i;
    for (i = 0; i < 128; i++) {
        OAMBuffer[i].attr0 |=  ATTR0_MOSAIC;
    }
}//void mosaicAllSprites

//---------------------------------------------------------------------------------
void disableAllSprites ()
//---------------------------------------------------------------------------------
{
    int i;
    for (i = 0; i < 128; i++) {
        OAMBuffer[i].attr0 = OBJ_DISABLE;
    }
}//void disableAllSprites

//---------------------------------------------------------------------------------
void disableSprite (int oaNumber, OBJATTR buffer[])
//---------------------------------------------------------------------------------
{
    if (buffer == NULL) buffer = OAMBuffer;
    buffer[oaNumber].attr0 = OBJ_DISABLE;
}//void disableSprite

//---------------------------------------------------------------------------------
void setSprite (int oaNumber, int xPos, int yPos, int charNum, int palNum, int prio, bool blend, bool mosaic, int size)
//---------------------------------------------------------------------------------
{
    int size0, size1;
    switch (size) {
        case 0:
            size0 = ATTR0_SQUARE;
            size1 = ATTR1_SIZE_16;
            break;
        case 1:
            size0 = ATTR0_WIDE;
            size1 = ATTR1_SIZE_32;
            break;
        case 2:
            size0 = ATTR0_TALL;
            size1 = ATTR1_SIZE_32;
            break;
        case 3:
            size0 = ATTR0_SQUARE;
            size1 = ATTR1_SIZE_64;
            break;
        case 4:
            size0 = ATTR0_SQUARE;
            size1 = ATTR1_SIZE_8;
            break;
        default: //gotta shut up those warnings;
            size0 = 69;
            size1 = 69;
            break;
    }

    if (palNum == -255) { //regular sprite 256col
        if (blend)
            if (mosaic)
                OAMBuffer[oaNumber].attr0 = (OBJ_Y(yPos) | ATTR0_MOSAIC | ATTR0_COLOR_256 | ATTR0_TYPE_BLENDED | size0);
            else
                OAMBuffer[oaNumber].attr0 = (OBJ_Y(yPos) | ATTR0_COLOR_256 | ATTR0_TYPE_BLENDED | size0);
        else
            OAMBuffer[oaNumber].attr0 = (OBJ_Y(yPos) | ATTR0_COLOR_256 | size0);
        OAMBuffer[oaNumber].attr1 = (OBJ_X(xPos) | size1);
        OAMBuffer[oaNumber].attr2 = (OBJ_CHAR(charNum << 2) | OBJ_PRIORITY(prio));
    } else if (palNum < 0) { //affine sprite 256col
        OAMBuffer[oaNumber].attr0 = (OBJ_Y(yPos) | ATTR0_ROTSCALE | ATTR0_ROTSCALE_DOUBLE | ATTR0_COLOR_256 | size0);
        OAMBuffer[oaNumber].attr1 = (OBJ_X(xPos) | ATTR1_ROTDATA(-palNum) | size1);
        OAMBuffer[oaNumber].attr2 = (OBJ_CHAR(charNum << 2) | OBJ_PRIORITY(prio));
    } else { //regular sprite 16col
        if (blend)
            if (mosaic)
                OAMBuffer[oaNumber].attr0 = (OBJ_Y(yPos) | ATTR0_MOSAIC | ATTR0_COLOR_16 | ATTR0_TYPE_BLENDED | size0);
            else
                OAMBuffer[oaNumber].attr0 = (OBJ_Y(yPos) | ATTR0_COLOR_16 | ATTR0_TYPE_BLENDED | size0);
        else
            OAMBuffer[oaNumber].attr0 = (OBJ_Y(yPos) | ATTR0_COLOR_16 | size0);
        OAMBuffer[oaNumber].attr1 = (OBJ_X(xPos) | size1);
        if (size == 4) { //8x8 sprite
            OAMBuffer[oaNumber].attr2 = (OBJ_CHAR(charNum << 0) | OBJ_PRIORITY(prio) | OBJ_PALETTE(palNum));
        } else {
            OAMBuffer[oaNumber].attr2 = (OBJ_CHAR(charNum << 2) | OBJ_PRIORITY(prio) | OBJ_PALETTE(palNum));
        }
    }
}//void setSprite

//---------------------------------------------------------------------------------
void showLoopScore (int x, int y, int score)
//---------------------------------------------------------------------------------
{
    int ls = 0, minTime = loopScoreTime + 1;
    for (int i = 0; i < mostLoopScores; i++) {
        if (lScore[i].timer < minTime) {
            minTime = lScore[i].timer;
            ls = i;
        }
    }
    posprintf(lScore[ls].scoreString, "%5d", score);
    lScore[ls].numDigits = 5;
    int i = 0;
    while (lScore[ls].scoreString[i] == ' ') {
        i++;
        lScore[ls].numDigits--;
    }
    lScore[ls].rightX = x + (lScore[ls].numDigits / 2);
    lScore[ls].topY = y - 4;
    lScore[ls].timer = loopScoreTime;
}//void showLoopScore

//---------------------------------------------------------------------------------
void copyGraphics ()
//---------------------------------------------------------------------------------
{
    CpuFastSet(pipesTiles, CHAR_BASE_ADR(pipeCharBlock), (pipesTilesLen >>2) | COPY32);
    CpuFastSet(blockTiles, (CHAR_BASE_ADR(pipeCharBlock) + ((blockStartTile << 3) << 4)), (blockTilesLen >>2) | COPY32);
    CpuFastSet(fontTiles, (CHAR_BASE_ADR(pipeCharBlock) + ((pointerStartTile << 3) << 4)), (fontTilesLen >> 2) | COPY32);
    CpuFastSet(gameInterfaceTiles, CHAR_BASE_ADR(gameUICharBlock), (gameInterfaceTilesLen >>2) | COPY32);
    CpuFastSet(gameInterfaceMap, MAP_BASE_ADR(gameUIMap), (gameInterfaceMapLen >>2) | COPY32);
    CpuFastSet(pipesTiles, (u16*)SPRITE_GFX, (pipesTilesLen >>2) | COPY32);
    CpuFastSet(blockTiles, ((u16*)SPRITE_GFX + ((blockStartTile << 2) << 4)), (blockTilesLen >>2) | COPY32);
    if (buttonLabels == 1)
        CpuFastSet(pointerTiles, (u16*)(SPRITE_GFX + ((pointerStartTile << 2) << 4)), (pointerTilesLen >> 2) | COPY32);
    else
        CpuFastSet(pointer_nolabelTiles, (u16*)(SPRITE_GFX + ((pointerStartTile << 2) << 4)), (pointer_nolabelTilesLen >> 2) | COPY32);
    CpuFastSet(nextBarTiles, (u16*)(SPRITE_GFX + ((nextBarStartTile << 2) << 4)), (nextBarTilesLen >> 2) | COPY32);
    CpuFastSet(fontTiles, (u16*)(SPRITE_GFX + ((fontStartTile << 2) << 4)), (fontTilesLen >> 2) | COPY32);

}//void copyGraphics

//---------------------------------------------------------------------------------
void copyCannons ()
//---------------------------------------------------------------------------------
{
    if (buttonLabels == 1)
        CpuFastSet(pointerTiles, (u16*)(SPRITE_GFX + ((pointerStartTile << 2) << 4)), (pointerTilesLen >> 2) | COPY32);
    else
        CpuFastSet(pointer_nolabelTiles, (u16*)(SPRITE_GFX + ((pointerStartTile << 2) << 4)), (pointer_nolabelTilesLen >> 2) | COPY32);
}//void copyGraphics

//---------------------------------------------------------------------------------
void setPalettes ()
//---------------------------------------------------------------------------------
{
    processPalette((u16*)BG_PALETTE, (u16*) pipesPal, pipesPalLen >> 1);
    processPalette((u16*)BG_PALETTE + 240, (u16*) fontPal, fontPalLen >> 1);
    processPalette((u16*)BG_PALETTE + 224, (u16*) font_yellowPal, font_yellowPalLen >> 1);
    processPalette((u16*)BG_PALETTE + 208, (u16*) font_orangePal, font_orangePalLen >> 1);
    processPalette((u16*)BG_PALETTE + 192, (u16*) font_purplePal, font_purplePalLen >> 1);
    processPalette((u16*)BG_PALETTE + 176, (u16*) font_greenPal, font_greenPalLen >> 1);
    processPalette((u16*)SPRITE_PALETTE, (u16*) pipesPal, pipesPalLen >> 1);
    processPalette((u16*)SPRITE_PALETTE + 240, (u16*) fontPal, fontPalLen >> 1);
    processPalette((u16*)SPRITE_PALETTE + 224, (u16*) font_yellowPal, font_yellowPalLen >> 1);
    processPalette((u16*)SPRITE_PALETTE + 208, (u16*) font_orangePal, font_orangePalLen >> 1);
    processPalette((u16*)SPRITE_PALETTE + 192, (u16*) font_purplePal, font_purplePalLen >> 1);
    processPalette((u16*)SPRITE_PALETTE + 176, (u16*) font_greenPal, font_greenPalLen >> 1);
}//void setPalettes

//---------------------------------------------------------------------------------
void copyGameOver ()
//---------------------------------------------------------------------------------
{
    CpuFastSet(gameOverTiles, CHAR_BASE_ADR(pipeCharBlock), (gameOverTilesLen >>2) | COPY32);
    CpuFastSet(gameOverMap, MAP_BASE_ADR(bgMap), (gameOverMapLen >>2) | COPY32);
}//void copyGameOver

//---------------------------------------------------------------------------------
void copyJam ()
//---------------------------------------------------------------------------------
{
    CpuFastSet(cloudsTiles, CHAR_BASE_ADR(0), (cloudsTilesLen >>2) | COPY32);
    CpuFastSet(cloudsMap, MAP_BASE_ADR(31), (cloudsMapLen >>2) | COPY32);
    CpuFastSet(cloudsPal, (u16*)BG_PALETTE, (cloudsPalLen >> 2) | COPY32);
    CpuFastSet(logoTiles, CHAR_BASE_ADR(3), (logoTilesLen >>2) | COPY32);
    CpuFastSet(logoMap, MAP_BASE_ADR(22), (logoMapLen >>2) | COPY32);
    CpuFastSet(pipesPal, (u16*)SPRITE_PALETTE, (pipesPalLen >> 2) | COPY32);
    CpuFastSet(pipesTiles, (u16*)SPRITE_GFX + 0x6580, (pipesTilesLen >>2) | COPY32); //extra copy in high sprite block for save indicator to work in mode 4
}//void copyJam

//---------------------------------------------------------------------------------
void copyBittwyst ()
//---------------------------------------------------------------------------------
{
    CpuFastSet(bittwystTiles, CHAR_BASE_ADR(0),(bittwystTilesLen >> 2) | COPY32);
    CpuFastSet(&btpalette, (u16*)BG_PALETTE, (sizeof(btpalette) >> 2) | COPY32);
    CpuFastSet(pipesPal, (u16*)SPRITE_PALETTE, (pipesPalLen >> 2) | COPY32);
}//void copyBittwyst

//---------------------------------------------------------------------------------
void copyTitle ()
//---------------------------------------------------------------------------------
{
    CpuFastSet(pipesTiles, CHAR_BASE_ADR(pipeCharBlock), (pipesTilesLen >>2) | COPY32);
    CpuFastSet(fontTiles, (CHAR_BASE_ADR(pipeCharBlock) + ((pointerStartTile << 3) << 4)), (fontTilesLen >> 2) | COPY32);
    CpuFastSet(titleSpinTiles, CHAR_BASE_ADR(floorCharBlock), (titleSpinTilesLen >>2) | COPY32);
    CpuFastSet(titleSpinMap, MAP_BASE_ADR(gameUIMap), (titleSpinMapLen >>2) | COPY32);
    CpuFastSet(titlePipeTiles, ((u16*)SPRITE_GFX + ((blockStartTile << 2) << 4)), (titlePipeTilesLen >>2) | COPY32);
}//void copyTitle

//---------------------------------------------------------------------------------
void copyLevelScore ()
//---------------------------------------------------------------------------------
{
    CpuFastSet(pipesTiles, CHAR_BASE_ADR(pipeCharBlock), (pipesTilesLen >>2) | COPY32);
    CpuFastSet(levelScoreTiles, CHAR_BASE_ADR(gameUICharBlock), (levelScoreTilesLen >>2) | COPY32);
    CpuFastSet(levelScoreMap, MAP_BASE_ADR(gameUIMap), (levelScoreMapLen >>2) | COPY32);
    CpuFastSet(fontTiles, (u16*)(SPRITE_GFX + ((fontStartTile << 2) << 4)), (fontTilesLen >> 2) | COPY32);
}//void copyLevelScore
