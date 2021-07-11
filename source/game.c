//
//  game.c
//  PipeSpin
//
//  Created by Dave on 29/05/2021.
//

// local headers
#include "game.h"

// globals
int keysPressed, keysPressedRepeat, keysReleased, keysHolded;
bool aPressed, startPressed;
bool canRotate, canMove, flipping, puttingBack;
int rotating, rotatingNext, rotateCount, rotSteps, lastRotate = 1, flipCount, doGameOver;
u32 sourceX, sourceY, destX, destY, tileNum, pointerX, pointerY;
int flipNum;
struct cursorCoords coords, puttingBackCoords;
struct loop newLoop;

u32 score, loopScore, bonusScore;
u16 pipesCleared, level, maxLoop, nextLevelPipes, nextBlockPipes, startLevel = 0;

//---------------------------------------------------------------------------------
void setLevel (int levelNumber)
//---------------------------------------------------------------------------------
{
    level = levelNumber;
    if (level > maxLevel) level = maxLevel;
    pipeAddDelay = (40 - level) * 15;
    pipeAddCount = 0;
    nextLevelPipes = pipesPerLevel * (level + 1);
#ifdef DEBUGGERY
    //mgba_printf(MGBA_LOG_INFO, "level: %i, nextLevelPipes: %i", level, nextLevelPipes);
#endif
}//void setLevel

//---------------------------------------------------------------------------------
void pipePlaced (int x, int y)
//---------------------------------------------------------------------------------
{
    bool levelUp = false;
    loopScore = bonusScore = 0;
    newLoop = loopCheck(x, y, false);
    if (newLoop.loop) {
        pipesCleared += newLoop.numPipes;
        if (newLoop.numPipes > maxLoop) {
            maxLoop = newLoop.numPipes;
            maxLoopTimer = loopScoreTime;
        }
        //score += newLoop.numPipes * (level + 1);

        for (int i = 0; i < newLoop.numPipes; i++) {
            loopScore += (i + 1) * (level + 1);
        }
        for (int i = 0; i < newLoop.crossed; i++) {
            bonusScore += (i + 1) * crossBonus * (level + 1);
        }
        for (int i = 0; i < newLoop.numBlocks; i++) {
            bonusScore += (i + 1) * blockBonus * (level + 1);
        }
        //bonusScore += newLoop.crossed * crossBonus * (level + 1);
        //bonusScore += newLoop.numBlocks * blockBonus * (level + 1);
#ifdef DEBUGGERY
        mgba_printf(MGBA_LOG_INFO, "level: %i, numPipes: %i, loopScore: %i, bonusScore: %i", level, newLoop.numPipes, loopScore, bonusScore);
#endif

#ifdef DEBUGGERY
        //mgba_printf(MGBA_LOG_INFO, "pipesCleared: %i", pipesCleared);
        //mgba_printf(MGBA_LOG_INFO, "numPipes: %i, numBlocks: %i, crossed: %i", newLoop.numPipes, newLoop.numBlocks, newLoop.crossed);
#endif
        while (pipesCleared >= nextLevelPipes) { //this can be simplified if it doesn't end up doing anything besides increasing the level
            if (level < maxLevel) {
                setLevel(level+1);
                levelUp = true;
            }
            //score += levelBonus;
            //nextLevelPipes += pipesPerLevel;
            //addBlock();
        }
        while (pipesCleared >= nextBlockPipes) {
            nextBlockPipes += pipesPerBlock;
            addBlock();
        }
        score += loopScore + bonusScore;
        showLoopScore(newLoop.centreX, newLoop.centreY, loopScore + bonusScore);
        if (levelUp) {
            playSound(levelSound);
            levelTimer = loopScoreTime;
        }
        playSound(removePipeSound);
#ifdef DEBUGGERY
        //mgba_printf(MGBA_LOG_INFO, "score: %i, pipes: %i, level: %i, maxLoop: %i, startLevel: %i", score, pipesCleared, level, maxLoop, startLevel);
#endif
    }
}//void pipePlaced

//---------------------------------------------------------------------------------
void gameScreen ()
//---------------------------------------------------------------------------------
{
    BGCTRL[2] = (BG_PRIORITY(gridPrio) | CHAR_BASE(pipeCharBlock) | BG_256_COLOR | SCREEN_BASE(bgMap) | ROTBG_SIZE_128x128); //game grid
    BGCTRL[1] = (BG_PRIORITY(frontPrio) | CHAR_BASE(pipeCharBlock) | BG_16_COLOR | SCREEN_BASE(floorMap) | TEXTBG_SIZE_256x256); //menu
    BGCTRL[0] = (BG_PRIORITY(gameUIPrio) | CHAR_BASE(gameUICharBlock) | BG_256_COLOR | SCREEN_BASE(gameUIMap) | TEXTBG_SIZE_256x256); //interface
    BG_OFFSET[1].x = 0;
    BG_OFFSET[1].y = 0;
    clearMenu(0, 0, 32, 32);
    disableAllSprites();
    SetMode(MODE_1);
    copyGraphics();
    setPalettes();

    soundEnable();

    gridInit();

    //configure window 1 to contain play grid
    REG_WIN1H = (gridXOffset << 8) | (gridXOffset + 128);
    REG_WIN1V = (gridYOffset << 8) | (gridYOffset + 128);
    REG_WININ = (0b010101 << 8) | (0b000010); //win1 (Obj BG2 BG0) | win0 (BG1)
    REG_WINOUT = (0b000000 << 8) | (0b010001); //winObj () | winOut (Obj BG0)

    //blending for beams
    REG_BLDCNT = (0b000001 << 8) | (0b01 << 6) | (0b010000); //BOT (BG0) | normal blend | TOP (Obj)
    REG_BLDALPHA = (0b11111 << 8) | (0b00100); //BOT weight | TOP weight

    SetMode(MODE_1 | BG0_ON | BG2_ON | OBJ_1D_MAP | OBJ_ON | WIN1_ON | OAM_HBL);

    mmStart(MOD_METALLIC, MM_PLAY_LOOP);

    inGame = true;
    playing = true;
    gameOverState = false;
    doGameOver = 0;
    gameQuit = false;

    canMove = true;
    flipping = false;
    puttingBack = false;
    pointerX = pointerY = 3;
    coords.foundHoriz = false;
    coords.foundVert = false;

    score = pipesCleared = maxLoop = 0;

    rotation = 0x0000;
    getGridRotation();
    rotating = rotatingNext = rotateCount = 0;
    canRotate = true;
    bgAS = (BGAffineSource){
            .x = (s32) 64 << 8, .y = (s32) 64 << 8, //Original data's center (8bit fp)
            .tX = (s16) (gridXOffset + 64), .tY = (s16) (gridYOffset + 64), //Display's center (not fp)
            .sX = (s16) 1 << 8, .sY = (s16) 1 << 8, //Scaling ratio (8bit fp)
            .theta = rotation    //Angle of rotation (8bit fp) Effective Range 0-FFFF
    };

    flipOA = (OBJAFFINE){ .pa=1<<8, .pb=0, .pc=0, .pd=1<<8 };

    vidInit();

    while (true) {
        VBlankIntrWait();

        keysPressedRepeat = keysDownRepeat();
        keysPressed = keysDown();
        keysReleased = keysUp();
        keysHolded = keysHeld();

        if (gameOverState) {
            if (doGameOver == 1) {
                mmStop();
                mmSetModuleTempo(1024);//standard speed
                playSound(gameOverSound1);
                logoScale = 1;
                doGameOver = 2;
            } else if (doGameOver == 3) {
                SetMode(MODE_1 | BG0_ON | BG1_ON | OBJ_1D_MAP | OBJ_ON | WIN1_ON | OAM_HBL); //hide affine bg while changing
                BGCTRL[2] = (BG_PRIORITY(frontPrio) | CHAR_BASE(pipeCharBlock) | BG_256_COLOR | SCREEN_BASE(bgMap) | ROTBG_SIZE_256x256); //put affine bg in front
                REG_WININ = (0b010111 << 8) | (0b000000); //win1 (Obj BG2 BG1 BG0) | win0 ()
                REG_WINOUT = (0b000000 << 8) | (0b010111); //winObj () | winOut (Obj BG2 BG1 BG0)
                copyGameOver();
                rotation = 0;
                bgAS = (BGAffineSource){
                        .x = (s32) 128 << 8, .y = (s32) 80 << 8, //Original data's center (8bit fp)
                        .tX = (s16) 128, .tY = (s16) 80, //Display's center (not fp)
                        .sX = (s16) bgAS.sX >> 1, .sY = (s16) bgAS.sY >> 1, //Scaling ratio (8bit fp)
                        .theta = rotation    //Angle of rotation (8bit fp) Effective Range 0-FFFF
                };
                SetMode(MODE_1 | BG0_ON | BG1_ON | BG2_ON | OBJ_1D_MAP | OBJ_ON | WIN1_ON | OAM_HBL);
                logoScale = 1 << 8;
                doGameOver = 4;
            } else if (doGameOver == 5) {
                playSound(gameOverSound2);
                BGCTRL[0] = (BG_PRIORITY(gameUIPrio) | CHAR_BASE(gameUICharBlock) | BG_MOSAIC | BG_256_COLOR | SCREEN_BASE(gameUIMap) | TEXTBG_SIZE_256x256);
                BGCTRL[1] = (BG_PRIORITY(floorPrio) | CHAR_BASE(floorCharBlock) | BG_MOSAIC | BG_256_COLOR | SCREEN_BASE(floorMap) | TEXTBG_SIZE_256x256);
                mosaicAllSprites();
                doGameOver = 6;
            } else if (doGameOver == 6) {
                if (keysPressed & KEY_A) {
                    aPressed = true;
                } else if (keysPressed & KEY_START) {
                    startPressed = true;
                }
                if (((keysReleased & KEY_A) && aPressed) || ((keysReleased & KEY_START) && startPressed)) {
                    FadeToBlack(60);
                    inGame = false;
                    gameEnded = true;
                    break;
                }
            }
        } else if (!playing) { //paused
            bool repeatMenu = true;
            while (repeatMenu) {
                switch (doMenu(mPause, true, true, true)) {
                    case 1:
                        SetMode(MODE_1 | BG0_ON | BG2_ON | OBJ_1D_MAP | OBJ_ON | WIN1_ON | OAM_HBL);
                        doMenu(mSettings, false, false, true);
                        repeatMenu = true;
                        break;
                    case 2:
                        SetMode(MODE_1 | BG0_ON | BG2_ON | OBJ_1D_MAP | OBJ_ON | WIN1_ON | OAM_HBL);
                        switch (doMenu(mConfirm, true, true, true)) {
                            case 0:
                                SetMode(MODE_1 | BG0_ON | BG2_ON | OBJ_1D_MAP | OBJ_ON | WIN1_ON | OAM_HBL);
                                repeatMenu = true;
                                break;
                            case 1:
                                SetMode(MODE_1 | BG0_ON | BG2_ON | OBJ_1D_MAP | OBJ_ON | WIN1_ON | OAM_HBL);
                                repeatMenu = false;
                                playing = false;
                                gameOverState = true;
                                doGameOver = 1;
                                gameQuit = true;
                                break;
                        }

                        break;
                    default:
                        SetMode(MODE_1 | BG0_ON | BG2_ON | OBJ_1D_MAP | OBJ_ON | WIN1_ON | OAM_HBL);
                        repeatMenu = false;
                        if (pauseMusicWhenPaused) mmResume();
                        playing = true;
                        break;
                }
            }
        } else {
            if ((keysPressed & KEY_A) && (!flipping) && (rotating == 0)) { //cannon
                if (coords.foundHoriz) {
                    if (heldPipe.piece->type == noPipe) { //pick up piece
                        heldPipe = removePipe(coords.hGridX, coords.hGridY);
                        heldPipe.rot = gridToScreenRot(heldPipe.rot);
                        heldPipe.tile = pipeTile(heldPipe.piece, heldPipe.rot, heldPipe.colour);
                        holdingPipe = heldPipe;
                        playSound(descend);
                        //animate
                        heldPipeSprite.y = (coords.hScreenY << 4) + gridYOffset - 8; //start screen coords
                        heldPipeSprite.x = (coords.hScreenX << 4) + gridXOffset - 8;
                        heldPipeSprite.gridY = (pointerHorizYpos + 8 - heldPipeSprite.y) >> 3; //step size (distance / 8)
#ifdef DEBUGGERY
                        //mgba_printf(MGBA_LOG_INFO, "heldPipeSprite.y: %i, heldPipeSprite.x: %i, heldPipeSprite.gridY: %i", heldPipeSprite.y, heldPipeSprite.x, heldPipeSprite.gridY);
#endif
                        pipeHold = 8;
                        canMove = false;
                        canRotate = false;
                    } else { //put back piece
                        holdingPipe = heldPipe;
                        puttingBackCoords = coords;
                        puttingBack = true;
                        heldPipe.piece = &pPiece[noPipe];
                        heldPipe.tile = pipeTile(heldPipe.piece, heldPipe.rot, heldPipe.colour);
                        playSound(ascend);
                        //animate
                        heldPipeSprite.y = pointerHorizYpos + 8; //start screen coords
                        heldPipeSprite.x = (pointerX << 4) + gridXOffset - 8;
                        heldPipeSprite.gridY = (((coords.hScreenY << 4) + gridYOffset - 8) - heldPipeSprite.y) >> 3; //step size (distance / 8)
                        heldPipeSprite.y -= heldPipeSprite.gridY;
#ifdef DEBUGGERY
                        //mgba_printf(MGBA_LOG_INFO, "heldPipeSprite.y: %i, heldPipeSprite.x: %i, heldPipeSprite.gridY: %i", heldPipeSprite.y, heldPipeSprite.x, heldPipeSprite.gridY);
#endif
                        pipeHold = 8;
                        canMove = false;
                        canRotate = false;
                    }
                } else { //target (A) not found
                    playSound(buzz1);
                }
            } else if ((keysPressed & KEY_B) && (!flipping) && (rotating == 0)) { //flip sideways
                if (coords.foundVert) {
                    if (coords.vScreenX != (gridWidth - 1) ) { //can't flip right from rightmost square
                        sourceX = coords.vGridX;
                        sourceY = coords.vGridY;
                        switch (gridRotation) { //find square under cursor & square to the right of that depending on grid rotation
                            case up0: //0deg upright
                                destX = sourceX + 1;
                                destY = sourceY;
                                break;
                            case left90: //90deg anticlockwise
                                destX = sourceX;
                                destY = sourceY + 1;
                                break;
                            case down180: //180deg
                                destX = sourceX - 1;
                                destY = sourceY;
                                break;
                            case right270: //90deg clockwise
                                destX = sourceX;
                                destY = sourceY - 1;
                                break;
                            default: //silence more warnings
                                break;
                        }
                        tileNum = grid[sourceX][sourceY].tile;
                        if ((tileNum != emptyTile) && (grid[destX][destY].tile == emptyTile)) {
                            flipPipe = removePipe(sourceX, sourceY);
                            if (flipPipe.piece->type == corner)
                                tileNum = pipeTile(flipPipe.piece, (right270 - cornerFlip[gridToScreenRot(flipPipe.rot)]), flipPipe.colour);
                            else
                                tileNum = pipeTile(flipPipe.piece, (gridToScreenRot(flipPipe.rot)), flipPipe.colour);
                            canRotate = false; //DO need to prevent rotation while piece is flipping because animation
                            //canMove = false; //don't need to prevent movement while piece is flipping?
                            flipping = true;
                            playSound(flipSound);
                        }
                    } else { //rightmost square can't flip
                        playSound(buzz2);
                    }
                } else { //target (B) not found
                    playSound(buzz2);
                }

            } else if (canRotate) {
                if (((keysPressed & KEY_L) && (keysPressed & KEY_R)) || (rotatingNext == 2)) {
                    rotatingNext = 0;
                    rotating = lastRotate << 1;
                    rotSteps = rotateSteps >> 1;
                    canRotate = false;
                    playSound(turn3);
                } else if ((keysPressed & KEY_L) || (rotatingNext == 1)) {
                    rotatingNext = 0;
                    rotating = 1;
                    lastRotate = rotating;
                    rotSteps = rotateSteps;
                    canRotate = false;
                    playSound(turn1);
                } else if ((keysPressed & KEY_R) || (rotatingNext == -1)) {
                    rotatingNext = 0;
                    rotating = -1;
                    lastRotate = rotating;
                    rotSteps = rotateSteps;
                    canRotate = false;
                    playSound(turn2);
                } else if (keysPressed & KEY_START) {
                    if (pauseMusicWhenPaused) mmPause();
                    playing = false; //pause
                } else if (keysPressed & KEY_SELECT) {
                    /*
                    correctCol = 1 - correctCol;
                    setPalettes();
                    */
                    pipeAddCount = pipeAddDelay - 1;
                /*
                } else if (keysPressed & KEY_START) {
                    gammaLevel += 1;
                    if (gammaLevel > 11) gammaLevel = 11;
                    sram[5] = (u8) gammaLevel;
                    setPalettes();
                } else if (keysPressed & KEY_SELECT) {
                    gammaLevel -= 1;
                    if (gammaLevel < 0) gammaLevel = 0;
                    sram[5] = (u8) gammaLevel;
                    setPalettes();
                */
                }
            } else { //(!canRotate)
                if ((keysPressed & KEY_L) && (keysPressed & KEY_R)) {
                    rotatingNext = 2;
                } else if (keysPressed & KEY_L) {
                    if (rotating != -1)
                        rotatingNext = 1;
                } else if (keysPressed & KEY_R) {
                    if (rotating != 1)
                        rotatingNext = -1;
                }
            }

            if (puttingBack && (pipeHold == 0)) {
                placePipe(puttingBackCoords.hGridX, puttingBackCoords.hGridY, holdingPipe.piece->type, screenToGridRot(holdingPipe.rot), holdingPipe.colour);
                pipePlaced(puttingBackCoords.hGridX, puttingBackCoords.hGridY);
                puttingBack = false;
            }

            if (rotating != 0) { //rotating
                rotation += (rotating * rotateStep);

                rotateCount += 1;
                if (rotateCount == rotateSteps) {
                    rotating = 0;
                    rotateCount = 0;
                    getGridRotation();
                    canRotate = true;
                }
                disableSprite(cursorhOA, NULL);
                disableSprite(cursorvOA, NULL);
            } else if (flipping) { //flipping
                if (flipCount == flipSteps) { //finished flipping
                    disableSprite(flipTileOA, NULL);
                    flipping = false;
                    flipCount = 0;
                    canRotate = true;
                    canMove = true;
                    enum rotDirection rDir = flipPipe.rot;
                    if (flipPipe.piece->type == corner)
                        rDir = screenToGridRot(cornerFlip[gridToScreenRot(rDir)]);
                    placePipe(destX, destY, flipPipe.piece->type, rDir, flipPipe.colour);
                    pipePlaced(destX, destY);
                } else {
                    if (flipCount > 7) {
                        flipNum = 15 - flipCount;
                        flipOA = flipOAs[flipNum];
                        flipOA.pa = -flipOA.pa;
                        flipOA.pc = -flipOA.pc;
                    } else {
                        flipNum = flipCount;
                        flipOA = flipOAs[flipNum];
                    }
                    setSprite(flipTileOA, (coords.vScreenX << 4) + gridXOffset - 8 + flipCount, (coords.vScreenY << 4) + gridYOffset - 8 + flipYOff[flipNum], pipeTileOffset + (tileNum << 1), -flipObjAff, flipPrio, false, false, 0);
                    disableCursor();
                }
                flipCount += 1;
            } else { //not rotating or flipping
                if (!puttingBack) {//(true) {//(scrollQueue == 0) {
                    coords = placeCursor(pointerX, pointerY);
                /*
                } else {
                    disableCursor();
                */
                }
            }

            if (canMove) {
                if (keysPressedRepeat & KEY_LEFT) {
                    if (pointerX != 0) pointerX -= 1;
                    playSound(moveTick);
                } else if (keysPressedRepeat & KEY_RIGHT) {
                    if (pointerX != (gridWidth - 1)) pointerX += 1;
                    playSound(moveTick);
                }
                if (keysPressedRepeat & KEY_UP) {
                    if (pointerY != 0) pointerY -= 1;
                    playSound(moveTick);
                } else if (keysPressedRepeat & KEY_DOWN) {
                    if (pointerY != (gridWidth - 1)) pointerY += 1;
                    playSound(moveTick);
                }
            }

            setSprite(pointerHorizOA, (pointerX << 4) + gridXOffset, pointerHorizYpos, pointerStartTile, -255, pointerPrio, false, false, 2);
            if (pipeHold == 0) { //not animating pipe add/remove
                if (heldPipe.piece->type != noPipe) {
                    setSprite(pipeHeldOA, (pointerX << 4) + gridXOffset - 8, pointerHorizYpos + 16 - 8, pipeTileOffset + (heldPipe.tile << 1), -heldObjAff, pointerPrio, false, false, 0);
                    ObjAffineSet(&pipeShrink, &pipeShrinkOA.pa, 1, 8);
                    ObjAffBuffer[heldObjAff].pa = pipeShrinkOA.pa;
                    ObjAffBuffer[heldObjAff].pb = pipeShrinkOA.pb;
                    ObjAffBuffer[heldObjAff].pc = pipeShrinkOA.pc;
                    ObjAffBuffer[heldObjAff].pd = pipeShrinkOA.pd;
                } else
                    disableSprite(pipeHeldOA, NULL);
            }
            setSprite(pointerHorizBottomOA, (pointerX << 4) + gridXOffset, pointerHorizYpos + 16, pointerStartTile + 16, -255, pointerPrio, false, false, 0);
            setSprite(pointerVertOA, pointerVertXpos, (pointerY << 4) + gridYOffset, pointerStartTile + 4, -255, pointerPrio, false, false, 1);
            setSprite(pointerVertBottomOA, pointerVertXpos, (pointerY << 4) + gridYOffset, pointerStartTile + 18, -255, gameUIPrio, false, false, 0L);
        }
    }
}//void gameScreen
