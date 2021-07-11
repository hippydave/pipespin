//
//  grid.h
//  PipeSpin
//
//  Created by Dave on 27/04/2021.
//

#ifndef GRID_H
#define GRID_H

#define gridWidth 8
#define gridSquares gridWidth * gridWidth
#define emptyTile 0
#define blockTile 30
#define maxBlocks 36
#define maxColours 4
#define numTypes 7

// libgba headers
#include <gba.h>

// local headers
#include "video.h"

#ifdef DEBUGGERY
#include "mgba.h"
#endif

enum pipeType { straight = 0, corner, cross, block, noPipe, pTsize };
enum sides { top = 0, left, bottom, right, none };
enum rotDirection { up0 = 0, left90, down180, right270, rDsize };
enum flipType { flipX = 0, flipY, flipXY, flipNone };
enum axis { dX, dY, axisSize };

struct pipePiece {
    enum pipeType type;
    u32 tile[rDsize]; //tile graphic for each direction
    enum sides exit[none]; //for each input direction, whether there is an output direction
};

struct gridItem {
    int x;
    int y;
    int colour;
    u32 tile;
    const struct pipePiece *piece;
    enum rotDirection rot;
    enum sides exit[none];
};

struct cursorCoords {
    int hGridX;
    int hGridY;
    int hScreenX;
    int hScreenY;
    int vGridX;
    int vGridY;
    int vScreenX;
    int vScreenY;
    bool foundHoriz;
    bool foundVert;
};

struct screenCoords {
    int x;
    int y;
};

struct loop {
    bool loop;
    bool sameColour;
    int numPipes;
    int numBlocks;
    int crossed;
    int centreX;
    int centreY;
};

struct numToPipe {
    const struct pipePiece *piece;
    enum rotDirection rot;
};

extern struct gridItem *squares[gridSquares];
extern struct gridItem grid[gridWidth][gridWidth];
extern struct gridItem heldPipe, flipPipe, holdingPipe;
extern struct gridItem blocks[maxBlocks];
const extern struct pipePiece pPiece[pTsize];

extern enum rotDirection gridRotation;
extern int squaresFilled;
extern int pipeAddDelay;
extern enum rotDirection cornerFlip[(int) rDsize];

//public functions
void gridInit ();
void getGridRotation ();
enum rotDirection screenToGridRot (enum rotDirection rDir);
enum rotDirection gridToScreenRot (enum rotDirection rDir);
struct screenCoords gridToScreenCoords (int gridX, int gridY);
u32 pipeTile (const struct pipePiece *piece, enum rotDirection rDir, int colour);
void placePipe (int x, int y, enum pipeType pType, enum rotDirection rDir, int colour);
struct gridItem removePipe (int x, int y);
struct loop loopCheck (int x, int y, bool test);
bool addPipe ();
void nextPipe ();
bool addBlock ();
struct cursorCoords placeCursor ( u32 px, u32 py);
void disableCursor ();
void drawQueue ();

#endif // GRID_H
