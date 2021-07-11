//
//  menu.c
//  PipeSpin
//
//  Created by Dave on 06/07/2021.
//

// local headers
#include "menu.h"

// globals
const struct menuItems mStartItems = { {
                                { 1, 0, { "SCORE ATTACK MODE", "", "", "", "", "", "", "" } },
                                { 1, 0, { "SETTINGS", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } }
                                } };

const struct menuItems mPauseItems = { {
                                { 1, 0, { "CONTINUE", "", "", "", "", "", "", "" } },
                                { 1, 0, { "SETTINGS", "", "", "", "", "", "", "" } },
                                { 0, 0, { "QUIT GAME", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } }
                                } };

const struct menuItems mSettingsItems = { { //lowercase a-e = colours (a grey, b yellow, c orange, d purple, e green)
                                { 2, 1, { "BUTTON LABELS  cOFFb>", "BUTTON LABELS  b<eON ", "", "", "", "", "", "" } },
                                { 8, 7, { "MUSIC VOLUME     c0b>", "MUSIC VOLUME    b<d1b>", "MUSIC VOLUME    b<d2b>", "MUSIC VOLUME    b<d3b>", "MUSIC VOLUME    b<d4b>", "MUSIC VOLUME    b<d5b>", "MUSIC VOLUME    b<d6b>", "MUSIC VOLUME    b<d7 " } },
                                { 8, 7, { "SOUND FX VOLUME  c0b>", "SOUND FX VOLUME b<d1b>", "SOUND FX VOLUME b<d2b>", "SOUND FX VOLUME b<d3b>", "SOUND FX VOLUME b<d4b>", "SOUND FX VOLUME b<d5b>", "SOUND FX VOLUME b<d6b>", "SOUND FX VOLUME b<d7 " } },
                                { 2, 1, { "COLOUR ADJUST  cOFFb>", "COLOUR ADJUST  b<eON ", "", "", "", "", "", "" } },
                                { 8, 4, { "GAMMA ADJUST    d-4b>", "GAMMA ADJUST   b<d-3b>", "GAMMA ADJUST   b<d-2b>", "GAMMA ADJUST   b<d-1b>", "GAMMA ADJUST  b<cOFFb>", "GAMMA ADJUST   b<d+1b>", "GAMMA ADJUST   b<d+2b>", "GAMMA ADJUST   b<d+3 " } },
                                { 2, 1, { "SAVE INDICATOR  d1Sb>", "SAVE INDICATOR b<d5S ", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } }
                                } };

const struct menuItems mConfirmItems = { {
                                { 1, 0, { "cNO", "", "", "", "", "", "", "" } },
                                { 1, 0, { "eYES", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } },
                                { 0, 0, { "", "", "", "", "", "", "", "" } }
                                } };

struct menu mStart = { 2, false, "", { 0, 0, 0, 0, 0, 0, 0, 0 }, { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, NULL };

struct menu mPause = { 3, false, "", { 0, 0, 0, 0, 0, 0, 0, 0 }, { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, NULL };

struct menu mSettings = { 6, false, "", { 1, 7, 7, 1, 4, 1, 0, 0 }, { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, NULL };

struct menu mConfirm = { 2, true, "ARE YOU SURE?", { 0, 0, 0, 0, 0, 0, 0, 0 }, { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, NULL };

//---------------------------------------------------------------------------------
void initMenu ()
//---------------------------------------------------------------------------------
{
    mStart.items = &mStartItems;
    mPause.items = &mPauseItems;
    mSettings.items = &mSettingsItems;
    mConfirm.items = &mConfirmItems;

    mSettings.setVar[0] = &buttonLabels;
    mSettings.setVar[1] = &musicVolume;
    mSettings.setVar[2] = &sfxVolume;
    mSettings.setVar[3] = &correctCol;
    mSettings.setVar[4] = &gammaLevel;
    mSettings.setVar[5] = &saveIndicatorSetting;
}//void initMenu

//---------------------------------------------------------------------------------
int sLen (u8 s[])
//---------------------------------------------------------------------------------
{
    int c = 0, l = 0;
    while (s[c] != 0) {
        if (s[c] < '[') {
            l++;
        }
        c++;
    }
    return l;
}//int sLen

//---------------------------------------------------------------------------------
int doMenu (struct menu mMenu, bool startSelects, bool aSelects, bool win1on)
//---------------------------------------------------------------------------------
{
    playSound(moveTick);
    int yPos = 9 - (mMenu.numItems >> 1);
    int cursorX = 99;
    int lowX = 99, highX = 0;
    int leftX, rightX;
    if (mMenu.hasTitle) {
        leftX = 15 - (sLen(mMenu.title) >> 1);
        rightX = 15 + (sLen(mMenu.title) >> 1);
        printText (leftX, yPos, mMenu.title, 15);
        if (leftX - 1 < lowX)
            lowX = leftX - 1;
        if (rightX + 1 > highX)
            highX = rightX + 1;
    }
    for (int l = 0; l < mMenu.numItems; l++) {
        if (mMenu.setVar[l] != NULL) {
            mMenu.currentVersion[l] = *mMenu.setVar[l];
        }
        leftX = 15 - (sLen(mMenu.items->items[l].versions[mMenu.currentVersion[l]]) >> 1);
        rightX = 15 + (sLen(mMenu.items->items[l].versions[mMenu.currentVersion[l]]) >> 1);
        printText (leftX, yPos + l + 1, mMenu.items->items[l].versions[mMenu.currentVersion[l]], 15);
        if (leftX - 1 < lowX)
            lowX = leftX - 1;
        if (leftX - 1 < cursorX)
            cursorX = leftX - 1;
        if (rightX + 1 > highX)
            highX = rightX + 1;
    }
    drawBorder(lowX - 1, yPos - 1, highX + 2 - lowX, mMenu.numItems + 4);
    REG_WIN0H = (((lowX - 1) * 8) << 8) | ((highX + 1) * 8);
    REG_WIN0V = (((yPos - 1) * 8) << 8) | ((yPos + mMenu.numItems + 3) * 8);
    if (win1on)
        SetMode(MODE_1 | BG0_ON | BG1_ON | BG2_ON | OBJ_1D_MAP | OBJ_ON | WIN1_ON | WIN0_ON | OAM_HBL);
    else
        SetMode(MODE_1 | BG0_ON | BG1_ON | BG2_ON | OBJ_1D_MAP | OBJ_ON | WIN0_ON | OAM_HBL);

    int selection = 0;
    int menuTime = 0;
    while (menuTime < 20) {
        VBlankIntrWait();
        menuTime++;
    }
    while (true) {
        printText (cursorX, yPos + selection + 1, (u8*) "b>", 15);
        VBlankIntrWait();
        keysPressed = keysDown();

        if (keysPressed & KEY_START) {
            if (startSelects) {
                break;
            } else {
                selection = -1;
                break;
            }
        } else if (keysPressed & KEY_B) {
            selection = -1;
            break;
        } else if (keysPressed & KEY_A) {
            if (aSelects) break;
        } else if (keysPressed & KEY_UP) {
            if (selection > 0) {
                printText (cursorX, yPos + selection + 1, (u8*) " ", 15);
                selection -= 1;
                playSound(moveTick);
            }
        } else if (keysPressed & KEY_DOWN) {
            if (selection < (mMenu.numItems - 1)) {
                printText (cursorX, yPos + selection + 1, (u8*) " ", 15);
                selection += 1;
                playSound(moveTick);
            }
        } else if (keysPressed & KEY_LEFT) {
            if (mMenu.currentVersion[selection] > 0) {
                mMenu.currentVersion[selection]--;
                if (mMenu.setVar[selection] != NULL) {
                    *mMenu.setVar[selection] = mMenu.currentVersion[selection];
                    updateSettings();
                }
                printText (15 - (sLen(mMenu.items->items[selection].versions[mMenu.currentVersion[selection]]) >> 1), yPos + selection + 1, mMenu.items->items[selection].versions[mMenu.currentVersion[selection]], 15);
                playSound(moveTick);
            }
        } else if (keysPressed & KEY_RIGHT) {
            if (mMenu.currentVersion[selection] < mMenu.items->items[selection].numVersions - 1) {
                mMenu.currentVersion[selection]++;
                if (mMenu.setVar[selection] != NULL) {
                    *mMenu.setVar[selection] = mMenu.currentVersion[selection];
                    updateSettings();
                }
                printText (15 - (sLen(mMenu.items->items[selection].versions[mMenu.currentVersion[selection]]) >> 1), yPos + selection + 1, mMenu.items->items[selection].versions[mMenu.currentVersion[selection]], 15);
                playSound(moveTick);
            }
        }
    }
    playSound(moveTick);
    clearMenu(lowX - 1, yPos - 1, highX + 2 - lowX, mMenu.numItems + 4);
    return selection;
}//int doMenu

