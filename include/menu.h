//
//  menu.h
//  PipeSpin
//
//  Created by Dave on 06/07/2021.
//

#ifndef MENU_H
#define MENU_H

#define cornerBorder 59
#define vertBorder 60
#define horizBorder 61

// local headers
#include "video.h"

#ifdef DEBUGGERY
#include "mgba.h"
#endif

struct menuItem {
    int numVersions;
    int currentVersion;
    u8 versions[8][25];
};

struct menuItems {
    struct menuItem items[8];
};

struct menu {
    int numItems;
    bool hasTitle;
    u8 title[25];
    int currentVersion [8];
    int* setVar[8];
    struct menuItems *items;
};

extern struct menu mStart, mPause, mSettings, mConfirm;

//public functions
void initMenu ();
int doMenu (struct menu mMenu, bool startSelects, bool aSelects, bool win1on);

#endif // MENU_H
