//
//  hiscores.c
//  PipeSpin
//
//  Created by Dave on 03/07/2021.
//

// local headers
#include "hiscores.h"

// globals
bool lsAPressed, lsBPressed, lsStartPressed, waitLevelSelect, levelSelect, doHiScore, hsMusic, scoreOrderDone, nameDone, startGame, gameEnded;
int lCursorX, lCursorY, currentHi, currentChar;

struct levelHiScore levelScores[10];

volatile u8 *sram = (u8*) 0x0E000000;
int sramCounter;
u8* byteConv;

int saveIndicatorTime = 5;//seconds
int saveIndicatorSetting = 1;

//---------------------------------------------------------------------------------
void loadHiScores ()
//---------------------------------------------------------------------------------
{
#ifdef DEBUGGERY
    mgba_printf(MGBA_LOG_INFO, "loadHiScores()");
#endif
    sramCounter = 0;
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 7; k++) {
                levelScores[i].name[j][k] = sram[scoreSramStart + sramCounter++] ^ sx;
            }
            byteConv = (u8*)&levelScores[i].score[j];
            for (int l = 0; l < 4; l++) {
                byteConv[l] = sram[scoreSramStart + sramCounter++] ^ sx;
            }
        }
        byteConv = (u8*)&levelScores[i].maxLoop;
        for (int l = 0; l < 2; l++) {
            byteConv[l] = sram[scoreSramStart + sramCounter++] ^ sx;
        }
    }
}//void loadHiScores

//---------------------------------------------------------------------------------
void saveHiScores ()
//---------------------------------------------------------------------------------
{
#ifdef DEBUGGERY
    mgba_printf(MGBA_LOG_INFO, "saveHiScores()");
#endif
    saveTimer = saveIndicatorTime * 60;
    sramCounter = 0;
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 7; k++) {
                sram[scoreSramStart + sramCounter++] = levelScores[i].name[j][k] ^ sx;
            }
            byteConv = (u8*)&levelScores[i].score[j];
            for (int l = 0; l < 4; l++) {
                sram[scoreSramStart + sramCounter++] = byteConv[l] ^ sx;
            }
        }
        byteConv = (u8*)&levelScores[i].maxLoop;
        for (int l = 0; l < 2; l++) {
            sram[scoreSramStart + sramCounter++] = byteConv[l] ^ sx;
        }
    }
}//void saveHiScores

//---------------------------------------------------------------------------------
void initHiScores ()
//---------------------------------------------------------------------------------
{
#ifdef DEBUGGERY
    mgba_printf(MGBA_LOG_INFO, "initHiScores()");
#endif
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 7; k++) {
                levelScores[i].name[j][k] = 0;
            }
            levelScores[i].score[j] = 0;
        }
        levelScores[i].maxLoop = 0;
    }
    saveHiScores();
}//void initHiScores

//---------------------------------------------------------------------------------
void updateSettings ()
//---------------------------------------------------------------------------------
{
    saveIndicatorTime = (saveIndicatorSetting == 1 ? 5 : 1);
    if (inGame) copyCannons();
    setPalettes();
    setVolume();
    sram[4] = (u8) correctCol;
    sram[5] = (u8) gammaLevel;
    sram[6] = (u8) buttonLabels;
    sram[7] = (u8) musicVolume;
    sram[8] = (u8) sfxVolume;
    sram[9] = (u8) saveIndicatorSetting;

    saveTimer = saveIndicatorTime * 60;
}//void updateSettings

//---------------------------------------------------------------------------------
bool levelScoreScreen ()
//---------------------------------------------------------------------------------
{
    lsScreen = true;
    startGame = false;

    SetMode(MODE_1);
    copyLevelScore();
    BGCTRL[1] = (BG_PRIORITY(floorPrio) | CHAR_BASE(pipeCharBlock) | BG_256_COLOR | SCREEN_BASE(bgMap) | TEXTBG_SIZE_256x256);
    BGCTRL[0] = (BG_PRIORITY(gameUIPrio) | CHAR_BASE(gameUICharBlock) | BG_256_COLOR | SCREEN_BASE(gameUIMap) | TEXTBG_SIZE_256x256);
    BG_OFFSET[1].x = 0;
    BG_OFFSET[1].y = 0;

    REG_BLDCNT = (0b100000 << 8) | (0b11 << 6) | (0b000010); //BOT (BD) | fade to black | TOP (BG1)
    REG_BLDY = 0b10000; //blend weight - start at 16, fade to 8
    blendCount = 16;
    blendTimer = 0;

    r256init(newSeed);
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 15; x++) {
            doubleTileReg(x, y, ranRange(1, 28), pipeTileOffset, bgMap, 32); //random tile bg
        }
    }
    disableAllSprites();

    VBlankIntrWait();
    SetMode(MODE_1 | BG0_ON | BG1_ON | OBJ_1D_MAP | OBJ_ON);
    setPalettes ();

    //set up state depending how we got here and whether there's a new high score
    currentHi = 99;
    currentChar = 0;
    scoreOrderDone = false;
    nameDone = false;
    hsMusic = false;
    if (gameEnded && !gameQuit) {
        if (score > levelScores[startLevel].score[2]) {
            doHiScore = true;
            levelSelect = false;
            hsMusic = true;
        } else {
            doHiScore = false;
            levelSelect = true;
        }
        gameEnded = false;

        if (maxLoop > levelScores[startLevel].maxLoop) {
            levelScores[startLevel].maxLoop = maxLoop;
            maxLoopTimer = loopScoreTime;
            waitLevelSelect = true;
            levelSelect = false;
        }
    } else {
        doHiScore = false;
        levelSelect = true;
    }

    //music
    mmSetModuleTempo(1024);//standard speed
    if (hsMusic) {
        mmStart(MOD_HISCORE, MM_PLAY_LOOP);
    } else {
        if (!mmActive()) mmStart(MOD_TITLE, MM_PLAY_LOOP);
    }

    //place level select cursor
    if (startLevel < 5) {
        lCursorY = 0;
        lCursorX = startLevel;
    } else {
        lCursorY = 1;
        lCursorX = startLevel - 5;
    }

    int lsTime = 0;
    while (true) {
        VBlankIntrWait();
        lsTime++;

        keysPressedRepeat = keysDownRepeat();
        keysPressed = keysDown();
        keysReleased = keysUp();
        keysHolded = keysHeld();

        if (levelSelect) {
            if ((keysPressed & KEY_B) && (lsTime > (1 * 30))) {
                lsBPressed = true;
            }
            if (keysReleased & KEY_B && lsBPressed) {
                playSound(descend);
                levelSelect = false;
                FadeToBlack(60);
                lsScreen = false;
                startGame = false;
                break;
            }
            if ((keysPressed & KEY_A) && (lsTime > (1 * 30))) {
                lsAPressed = true;
            }
            if ((keysPressed & KEY_START) && (lsTime > (1 * 30))) {
                lsStartPressed = true;
            }
            if ((keysReleased & KEY_A && lsAPressed) || (keysReleased & KEY_START && lsStartPressed)) {
                playSound(ascend);
                levelSelect = false;
                FadeToBlack(60);
                lsScreen = false;
                startGame = true;
                break;
            }
            if (keysPressedRepeat & KEY_RIGHT) {
                if (lCursorX != 4) {
                    lCursorX++;
                    levelTimer = 0;
                    maxLoopTimer = 0;
                    playSound(moveTick);
                } else
                    if (lCursorY == 0) {
                        lCursorY = 1;
                        lCursorX = 0;
                        levelTimer = 0;
                        maxLoopTimer = 0;
                        playSound(moveTick);
                    }
            } else if (keysPressedRepeat & KEY_LEFT) {
                if (lCursorX != 0) {
                    lCursorX--;
                    levelTimer = 0;
                    maxLoopTimer = 0;
                    playSound(moveTick);
                } else
                    if (lCursorY == 1) {
                        lCursorY = 0;
                        lCursorX = 4;
                        levelTimer = 0;
                        maxLoopTimer = 0;
                        playSound(moveTick);
                    }
            } else if (keysPressed & KEY_UP) {
                if (lCursorY != 0) {
                    lCursorY--;
                    levelTimer = 0;
                    maxLoopTimer = 0;
                    playSound(moveTick);
                }
            } else if (keysPressed & KEY_DOWN) {
                if (lCursorY != 1) {
                    lCursorY++;
                    levelTimer = 0;
                    maxLoopTimer = 0;
                    playSound(moveTick);
                }
            }

            startLevel = lCursorX + (5 * lCursorY);
        } else if (doHiScore) {
            if (!scoreOrderDone) {
                for (int i = 2; i > -1; i--) {
                    if (score > levelScores[startLevel].score[i]) {
                        if (i < 2) { //move current hiscore down
                            levelScores[startLevel].score[i + 1] = levelScores[startLevel].score[i];
                            for (int j = 0; j < 8; j++) {
                                levelScores[startLevel].name[i + 1][j] = levelScores[startLevel].name[i][j];
                            }
                        }
                        currentHi = i;
                        levelScores[startLevel].score[currentHi] = score;
                    }
                }
                levelScores[startLevel].name[currentHi][0] = 'A';
                for (int i = 1; i < 7; i++) {
                    levelScores[startLevel].name[currentHi][i] = 0;
                }
                scoreOrderDone = true;
            } else if (!nameDone) {
                levelTimer = loopScoreTime;
                if (keysPressed & KEY_A) {
                    if (currentChar < 6) {
                        currentChar++;
                        levelScores[startLevel].name[currentHi][currentChar] = 'A';
                        playSound(moveTick);
                    } else {
                        playSound(buzz1);
                    }
                } else if (keysPressed & KEY_B) {
                    if (currentChar > 0) {
                        levelScores[startLevel].name[currentHi][currentChar] = 0;
                        currentChar--;
                        playSound(moveTick);
                    } else {
                        playSound(buzz1);
                    }
                } else if ((keysPressedRepeat & KEY_UP) || (keysPressedRepeat & KEY_RIGHT)) {
                    if (levelScores[startLevel].name[currentHi][currentChar] == 'Z') {
                        levelScores[startLevel].name[currentHi][currentChar] = ' ';
                    } else if (levelScores[startLevel].name[currentHi][currentChar] == '!') {
                        levelScores[startLevel].name[currentHi][currentChar] = '0';
                    } else if (levelScores[startLevel].name[currentHi][currentChar] == '9') {
                        levelScores[startLevel].name[currentHi][currentChar] = 'A';
                    } else {
                        levelScores[startLevel].name[currentHi][currentChar]++;
                    }
                    playSound(moveTick);
                } else if ((keysPressedRepeat & KEY_DOWN) || (keysPressedRepeat & KEY_LEFT)) {
                    if (levelScores[startLevel].name[currentHi][currentChar] == ' ') {
                        levelScores[startLevel].name[currentHi][currentChar] = 'Z';
                    } else if (levelScores[startLevel].name[currentHi][currentChar] == 'A') {
                        levelScores[startLevel].name[currentHi][currentChar] = '9';
                    } else if (levelScores[startLevel].name[currentHi][currentChar] == '0') {
                        levelScores[startLevel].name[currentHi][currentChar] = '!';
                    } else {
                        levelScores[startLevel].name[currentHi][currentChar]--;
                    }
                    playSound(moveTick);
                } else if (keysPressed & KEY_START) {
                    playSound(ascend);
                    saveHiScores();
                    nameDone = true;
                    waitLevelSelect = true;
                    doHiScore = false;
                }
            }
        } else if (waitLevelSelect) {
            if ((levelTimer == 0) && (maxLoopTimer == 0)) {
                waitLevelSelect = false;
                levelSelect = true;
                if (hsMusic) {
                    hsMusic = false;
                    mmStop();
                    mmStart(MOD_TITLE, MM_PLAY_LOOP);
                }
            }
        }
    }
    if (startGame) mmStop();
    REG_DMA3CNT = 0;
    REG_BG1HOFS = 0;
    disableAllSprites();
    return startGame;
}//void levelScoreScreen

//---------------------------------------------------------------------------------
void checkSave ()
//---------------------------------------------------------------------------------
{
    if ((sram[0] == (u8) 'p') && (sram[1] == (u8) 'S') && (sram[2] == (u8) 'p') && (sram[3] == (u8) 'n')) {
#ifdef DEBUGGERY
        mgba_printf(MGBA_LOG_INFO, "checkSave(): save data found, loading");
#endif
        correctCol = (u8)sram[4];
        gammaLevel = (u8)sram[5];
        buttonLabels = (u8)sram[6];
        musicVolume = (u8)sram[7];
        sfxVolume = (u8)sram[8];
        saveIndicatorSetting = (u8)sram[9];
        saveIndicatorTime = (saveIndicatorSetting == 1 ? 5 : 1);

        loadHiScores();
    } else {
#ifdef DEBUGGERY
        mgba_printf(MGBA_LOG_INFO, "checkSave(): save data not found, initialising");
#endif
        sram[0] = (u8) 'p';
        sram[1] = (u8) 'S';
        sram[2] = (u8) 'p';
        sram[3] = (u8) 'n';
        sram[4] = (u8) correctCol;
        sram[5] = (u8) gammaLevel;
        sram[6] = (u8) buttonLabels;
        sram[7] = (u8) musicVolume;
        sram[8] = (u8) sfxVolume;
        sram[9] = (u8) saveIndicatorSetting;

        initHiScores();
    }
}//void checkSave
