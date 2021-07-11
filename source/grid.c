//
//  grid.c
//  PipeSpin
//
//  Created by Dave on 27/04/2021.
//

// local headers
#include "grid.h"

struct gridItem *squares[gridSquares];
struct gridItem grid[gridWidth][gridWidth];
struct gridItem heldPipe, flipPipe, holdingPipe;
struct gridItem pipesToRemove[gridSquares];
struct gridItem bag[maxColours * numTypes][2];
struct gridItem blocks[maxBlocks];
int bagSize, bagNumber, bagIndex;
int minY[gridWidth], maxY[gridWidth];

enum rotDirection gridRotation;
struct screenCoords tempSC;
int squaresFilled = 0;
int startPipes = 5;
int numColours = 2;
bool onlySameColour = true;
int blockCount;
int pipeAddDelay;
const int tileDxy[axisSize][(int) none] = { { 0, -1, 0, 1 }, { -1, 0, 1, 0 } };
enum sides opposite[(int) none] = { bottom, right, top, left };
enum rotDirection cornerFlip[(int) rDsize] = { right270, down180, left90, up0 };

const struct pipePiece pPiece[pTsize] = {
                { straight, { 1, 2, 1, 2 }, { bottom, none, top, none } },
                { corner, { 3, 4, 5, 6 }, { none, none, right, bottom } },
                { cross, { 7, 7, 7, 7 }, { bottom, right, top, left } },
                { block, { 8, 8, 8, 8 }, { none, none, none, none } },
                { noPipe, { 0, 0, 0, 0 }, { none, none, none, none } } };

const struct numToPipe nToP[numTypes] = {
                { &pPiece[straight], up0 },
                { &pPiece[straight], left90 },
                { &pPiece[corner], up0 },
                { &pPiece[corner], left90 },
                { &pPiece[corner], down180 },
                { &pPiece[corner], right270 },
                { &pPiece[cross], up0 } };

//---------------------------------------------------------------------------------
void fillBag (int bagNum) {
//---------------------------------------------------------------------------------
    bool taken[numTypes][numColours];
    //r256init(newSeed);
    for (int i = 0; i < numColours; i++) {
        for (int j = 0; j < numTypes; j++) {
            taken[j][i] = false;
        }
    }
    int colour;
    int pT;
    for (int i = 0; i < bagSize; i++) {
        bool found = false;
        while (!found) {
            pT = ranRange(0, numTypes);
            colour = ranRange(0, numColours);
            if (!taken[pT][colour]) {
                taken[pT][colour] = true;
                bag[i][bagNum].colour = colour;
                bag[i][bagNum].piece = nToP[pT].piece;
                bag[i][bagNum].rot = nToP[pT].rot;
                bag[i][bagNum].tile = pipeTile(bag[i][bagNum].piece, bag[i][bagNum].rot, bag[i][bagNum].colour);
                found = true;
            }
        }
    }
}//void fillBag

//---------------------------------------------------------------------------------
void gridInit ()
//---------------------------------------------------------------------------------
{
    setLevel(startLevel);
    nextBlockPipes = pipesPerBlock;
    //fill background with tiles
    u32 x, y;
    for (y = 0; y < gridWidth; y++) {
        for (x = 0; x < gridWidth; x++) {
            placePipe(x, y, noPipe, up0, ranRange(0, numColours));
        }
    }
    squaresFilled = 0;
    pipeAddCount = 0;
    bagSize = numColours * numTypes;
    bagNumber = 0;
    bagIndex = 0;
    fillBag(0);
    fillBag(1);

    for (int i = 0; i < maxBlocks; i++) {
        blocks[i].x = -1;
        blocks[i].colour = -1;
    }

    //add initial pipes & block
    for (int i = 0; i < startPipes; i++) {
        addPipe();
        nextPipe();
    }
    addBlock();
    heldPipe.piece = &pPiece[noPipe];
    heldPipe.rot = up0;
    heldPipe.colour = 0;
    heldPipe.tile = pipeTile(heldPipe.piece, heldPipe.rot, heldPipe.colour);
    drawQueue();
}//void gridInit

//---------------------------------------------------------------------------------
void getGridRotation ()
//---------------------------------------------------------------------------------
{
    switch (rotation) {
        case 0x0000: //0deg upright
            gridRotation = up0;
            break;
        case 0x4000: //90deg anticlockwise
            gridRotation = left90;
            break;
        case 0x8000: //180deg
            gridRotation = down180;
            break;
        case 0xc000: //90deg clockwise
            gridRotation = right270;
            break;
    }
}//enum rotDirection getGridRotation

//---------------------------------------------------------------------------------
enum rotDirection screenToGridRot (enum rotDirection rDir)
//---------------------------------------------------------------------------------
{
    for (int i = 0; (enum rotDirection) i < gridRotation; i++) {
        if (rDir == up0)
            rDir = right270;
        else
            rDir--;
    }
    return rDir;
}//enum rotDirection screenToGridRot

//---------------------------------------------------------------------------------
enum rotDirection gridToScreenRot (enum rotDirection rDir)
//---------------------------------------------------------------------------------
{
    for (int i = 0; (enum rotDirection) i < gridRotation; i++) {
        rDir++;
        if (rDir == rDsize) rDir = up0;
    }
    return rDir;
}//enum rotDirection gridToScreenRot

//---------------------------------------------------------------------------------
struct screenCoords gridToScreenCoords (int x, int y)
//---------------------------------------------------------------------------------
{
    struct screenCoords newSC;
    switch (gridRotation) {
        case up0:
            newSC.x = x;
            newSC.y = y;
            break;
        case left90:
            newSC.x = y;
            newSC.y = gridWidth - 1 - x;
            break;
        case down180:
            newSC.x = gridWidth - 1 - x;
            newSC.y = gridWidth - 1 - y;
            break;
        case right270:
            newSC.x = gridWidth - 1 - y;
            newSC.y = x;
            break;
        default: //murder the warnings
            newSC.x = x;
            newSC.y = y;
            break;
    }
    newSC.x = (newSC.x * 16) + gridXOffset;
    newSC.y = (newSC.y * 16) + gridYOffset;
    return newSC;
}//struct screenCoords gridToScreenCoords

//---------------------------------------------------------------------------------
u32 pipeTile (const struct pipePiece *piece, enum rotDirection rDir, int colour)
//---------------------------------------------------------------------------------
{
    u32 tile;
    if (piece->type == noPipe)
        tile = emptyTile;
    else if (piece->type == block)
        tile = blockTile + blockFrame;
    else
        tile = piece->tile[rDir] + (colour * 7);
    return tile;
}//u32 pipeTile

//---------------------------------------------------------------------------------
void placePipe (int x, int y, enum pipeType pType, enum rotDirection rDir,int colour)
//---------------------------------------------------------------------------------
{
    grid[x][y].piece = &pPiece[pType];
    grid[x][y].rot = rDir;
    grid[x][y].colour = colour;
    grid[x][y].tile = pipeTile(grid[x][y].piece, grid[x][y].rot, grid[x][y].colour);
    int inSide, outSide;
    for (enum sides i = top; i < none; i++) {
        inSide = (int) i - (int) rDir;
        if (inSide < (int) top) inSide += (int) none;
        outSide = (int) pPiece[pType].exit[inSide];
        if (outSide != (int) none) {
            outSide += (int) rDir;
            if (outSide > (int) right) outSide -= (int) none;
        }
        grid[x][y].exit[i] = outSide;
    }
    doubleTile(x, y, grid[x][y].tile, pipeTileOffset, bgMap, 16);
}//void placePipe

//---------------------------------------------------------------------------------
struct gridItem removePipe (int x, int y)
//---------------------------------------------------------------------------------
{
    struct gridItem removedPipe;
    removedPipe = grid[x][y];
    grid[x][y].piece = &pPiece[noPipe];
    grid[x][y].colour = 0;
    grid[x][y].rot = up0;
    grid[x][y].tile = pipeTile(grid[x][y].piece, grid[x][y].rot, grid[x][y].colour);
    for (enum sides i = top; i < none; i++) {
        grid[x][y].exit[i] = none;
    }
    doubleTile(x, y, grid[x][y].tile, pipeTileOffset, bgMap, 16);
    return removedPipe;
}//struct gridItem removePipe

//---------------------------------------------------------------------------------
struct loop loopCheck (int x, int y, bool test)
//---------------------------------------------------------------------------------
{
    struct loop testLoop = { false, false, 0, 0, 0, 4, 4 };
    bool loopFound = false, sameColour = true, alreadyRemoved, isCross = false;
    int currentX = x, currentY = y, count = 0, colour, leftX = gridWidth, rightX = -1, topY = gridWidth, bottomY = -1, crossCount = 0;
    enum sides checkSide, exitSide, entrySide, initialSide;
    bool done = false;
    checkSide = top;
    exitSide = none;
    colour = grid[x][y].colour;
    blockCount = 0;
    exitSide = none;
    if (grid[x][y].piece->type == cross) {
        exitSide = (enum sides)screenToGridRot((enum rotDirection)left);
        crossCount = 2;
        isCross = true;
    } else {
        while (exitSide == none) { //find first exit for current piece
            exitSide = grid[currentX][currentY].exit[checkSide];
            checkSide++;
        }
        crossCount = 1;
    }

    while (crossCount > 0) {
        if (loopFound) break;
        //clear loop scope
        for (currentX = 0; currentX < gridWidth; currentX++) {
            minY[currentX] = -1;
            maxY[currentX] = -1;
        }
        currentX = x;
        currentY = y;
        done = false;
        if (isCross && (crossCount == 1)) exitSide = (enum sides)screenToGridRot((enum rotDirection)top);
        crossCount--;
        initialSide = exitSide;
        while (!done) {
            alreadyRemoved = false; //check if piece is already on remove list (if cross is passed through both ways)
            for (int i = 0; i < count; i++) {
                if ((pipesToRemove[i].x == currentX) && (pipesToRemove[i].y == currentY)) {
                    alreadyRemoved = true;
                    testLoop.crossed++;
                    break;
                }
            }
            if (!alreadyRemoved) {
                pipesToRemove[count].x = currentX;
                pipesToRemove[count].y = currentY;
                count++;
            }
            if ((minY[currentX] == -1) || (minY[currentX] > currentY)) minY[currentX] = currentY; //track vertical scope of loop for each column
            if ((maxY[currentX] == -1) || (maxY[currentX] < currentY)) maxY[currentX] = currentY;
            if (currentX < leftX) leftX = currentX;
            if (currentX > rightX) rightX = currentX;
            if (currentY < topY) topY = currentY;
            if (currentY > bottomY) bottomY = currentY;
            currentX += tileDxy[dX][(int) exitSide]; //find new piece
            currentY += tileDxy[dY][(int) exitSide];
            entrySide = opposite[exitSide];
            exitSide = grid[currentX][currentY].exit[entrySide];
            if (exitSide == none) { //can't enter new piece on chosen side
                done = true;
            } else {
                if ((currentX == x) && (currentY == y) && (exitSide == initialSide)) { //back to start
                    loopFound = true;
                    testLoop.loop = true;
                    testLoop.sameColour = sameColour;
                    testLoop.numPipes = count;
                    done = true;
                } else if (grid[currentX][currentY].colour != colour) {
                    sameColour = false;
                    if (onlySameColour) {
                        done = true;
                    }
                }
            }
            if (count == gridSquares) done = true; //why is this needed? shouldn't become endless
        }
    }

    if ((!test) && (loopFound)) {
        //remove pieces
        pipeRemove = 16; //for animation
        canRotate = false;
        for (int i = 0; i < count; i++) {
            //animate block removal
            tempSC = gridToScreenCoords(pipesToRemove[i].x, pipesToRemove[i].y);
            removePipeSprite[i].x = tempSC.x;
            removePipeSprite[i].y = tempSC.y;
            removePipeSprite[i].gridX = pipesToRemove[i].x;
            removePipeSprite[i].gridY = pipesToRemove[i].y;
            removePipeSprite[i].oA = nextFadeSprite++;
            if (nextFadeSprite == 128) nextFadeSprite = firstFadeSprite;
            setSprite(removePipeSprite[i].oA, removePipeSprite[i].x, removePipeSprite[i].y, pipeTile(grid[removePipeSprite[i].gridX][removePipeSprite[i].gridY].piece, gridToScreenRot(grid[removePipeSprite[i].gridX][removePipeSprite[i].gridY].rot), grid[removePipeSprite[i].gridX][removePipeSprite[i].gridY].colour) << 1, -255, flipPrio, true, true, 0);

            removePipe(pipesToRemove[i].x, pipesToRemove[i].y);
        }
        //search for block enclosed in loop
        for (currentX = 0; currentX < gridWidth; currentX++) {
#ifdef DEBUGGERY
            //mgba_printf(MGBA_LOG_INFO, "currentX: %i, minY: %i, maxY: %i", currentX, minY[currentX], maxY[currentX]);
#endif
            if (minY[currentX] != -1) {
                for (currentY = minY[currentX] + 1; currentY < maxY[currentX]; currentY++) {
                    if (grid[currentX][currentY].piece->type == block) {
                        removePipe(currentX, currentY);
                        pipesToRemove[blockCount].x = currentX;
                        pipesToRemove[blockCount].y = currentY;
                        blockCount++;
#ifdef DEBUGGERY
                        //mgba_printf(MGBA_LOG_INFO, "block removed: (%i, %i)", currentX, currentY);
#endif
                        for (int i = 0; i < maxBlocks; i++) {
                            if ((blocks[i].x == currentX) && (blocks[i].y == currentY)) {
                                blocks[i].x = -1;
                                //animate block removal
                                tempSC = gridToScreenCoords(currentX, currentY);
                                removeBlockSprite[i].x = tempSC.x;
                                removeBlockSprite[i].y = tempSC.y;
                                removeBlockSprite[i].gridX = currentX;
                                removeBlockSprite[i].gridY = currentY;
                                removeBlockSprite[i].oA = nextFadeSprite++;
                                if (nextFadeSprite == 128) nextFadeSprite = firstFadeSprite;
                                setSprite(removeBlockSprite[i].oA, removeBlockSprite[i].x, removeBlockSprite[i].y, (blockTile + blockFrame) << 1, -255, flipPrio, true, true, 0);
                                break;
                            }
                        }
                    }
                }
            }
        }
        testLoop.numBlocks = blockCount;
    }
    tempSC = gridToScreenCoords(((rightX - leftX) / 2) + leftX, ((bottomY - topY) / 2) + topY);
    testLoop.centreX = tempSC.x;
    testLoop.centreY = tempSC.y;
    return testLoop;
}//bool loopCheck

//---------------------------------------------------------------------------------
bool addPipe ()
//---------------------------------------------------------------------------------
{
    //update squaresFilled by full count - workaround for rare bug where its count turns negative, no time to hunt it down before jam deadline
    int x, y;
    squaresFilled = 0;
    for (y = 0; y < gridWidth; y++) {
        for (x = 0; x < gridWidth; x++) {
            if (grid[x][y].piece->type != noPipe) squaresFilled++;
        }
    }
    //end workaround
    if (squaresFilled == gridSquares) return false;
    int tries = gridSquares - squaresFilled;
    //int x, y;
    bool found = false, noLoop = false;
    while (!noLoop) {
        while (!found) {
            x = ranRange(0, gridWidth);
            y = ranRange(0, gridWidth);
            if (grid[x][y].piece->type == noPipe) found = true;
        }
        placePipe(x, y, bag[bagIndex][bagNumber].piece->type, bag[bagIndex][bagNumber].rot, bag[bagIndex][bagNumber].colour);
        if (loopCheck(x, y, true).loop) { //don't place a piece where it will complete a loop
            if  (tries != 0) { //try somewhere else
                removePipe(x, y);
                tries--;
                found = false;
            } else {
                enum rotDirection rDir = bag[bagIndex][bagNumber].rot + 1; //just spin it (won't help for cross)
                if (rDir == rDsize) rDir = up0;
                placePipe(x, y, bag[bagIndex][bagNumber].piece->type, rDir, bag[bagIndex][bagNumber].colour);
                pipePlaced(x, y);//check in case a cross did complete a loop despite all best efforts
                noLoop = true;
            }
        } else
            noLoop = true;
    }
    if (playing) {
        doubleTile(x, y, pipeTile(&pPiece[noPipe], gridToScreenRot(grid[x][y].rot), grid[x][y].colour), pipeTileOffset, bgMap, 16);
        scrollQueue = 16;
        canRotate = false;
        tempSC = gridToScreenCoords(x, y);
        addPipeSprite.x = tempSC.x;
        addPipeSprite.y = tempSC.y;
        addPipeSprite.gridX = x;
        addPipeSprite.gridY = y;
        setSprite(addPipeOA, addPipeSprite.x, addPipeSprite.y, pipeTile(grid[x][y].piece, gridToScreenRot(grid[x][y].rot), grid[x][y].colour) << 1, -255, flipPrio, true, true, 0);
        playSound(addPipeSound);
    }
    return true;
}//bool addPipe

//---------------------------------------------------------------------------------
void nextPipe () {
//---------------------------------------------------------------------------------
    bagIndex++;
    if (bagIndex == bagSize) {
        fillBag(bagNumber);
        bagIndex = 0;
        bagNumber = 1 - bagNumber;
    }
}//void nextPipe

//---------------------------------------------------------------------------------
bool addBlock ()
//---------------------------------------------------------------------------------
{
    int x, y, emptySquares = 0, blockNum;
    bool found = false, full = true, avoidRemovedBlocks = false;
    for (y = 1; y < gridWidth - 1; y++) { //test that grid area not including outermost tiles is not full
        for (x = 1; x < gridWidth - 1; x++) {
            if (grid[x][y].piece->type == noPipe) {
                full = false;
                emptySquares++;
            }
        }
    }
    if (full) return false;
    if (emptySquares > blockCount) avoidRemovedBlocks = true;
    while (!found) {
        x = ranRange(1, gridWidth - 1);
        y = ranRange(1, gridWidth - 1);
        if (grid[x][y].piece->type == noPipe) {
            found = true;
            if (avoidRemovedBlocks) {
                for (int i = 0; i < blockCount; i++) {
                    if ((x == pipesToRemove[i].x) && (y == pipesToRemove[i].y)) {
                        found = false;
                        break;
                    }
                }
            }
        }
    }
    placePipe(x, y, block, up0, 0);
    for (blockNum = 0; blockNum < maxBlocks; blockNum++) {
        if (blocks[blockNum].x == -1) {
            blocks[blockNum].x = x;
            blocks[blockNum].y = y;
            blocks[blockNum].colour = -1;
            break;
        }
    }
    if (playing) {
        doubleTile(x, y, pipeTile(&pPiece[noPipe], gridToScreenRot(grid[x][y].rot), grid[x][y].colour), pipeTileOffset, bgMap, 16);
        blockAdd = 16;
        canRotate = false;
        tempSC = gridToScreenCoords(x, y);
        addBlockSprite[blockNum].x = tempSC.x;
        addBlockSprite[blockNum].y = tempSC.y;
        addBlockSprite[blockNum].gridX = x;
        addBlockSprite[blockNum].gridY = y;
        addBlockSprite[blockNum].oA = nextFadeSprite++;
        if (nextFadeSprite == 128) nextFadeSprite = firstFadeSprite;
        setSprite(addBlockSprite[blockNum].oA, addBlockSprite[blockNum].x, addBlockSprite[blockNum].y, (blockTile + blockFrame) << 1, -255, flipPrio, true, true, 0);
    } else {
        blocks[blockNum].colour = 0;
    }
#ifdef DEBUGGERY
    //mgba_printf(MGBA_LOG_INFO, "block added: (%i, %i)", x, y);
#endif
    return true;
}//bool addBlock

//---------------------------------------------------------------------------------
struct cursorCoords placeCursor ( u32 px, u32 py)
//---------------------------------------------------------------------------------
{
    int gridX = 0, gridY = 0, dx = 0, dy = 0, screenX, screenY, tmpCount;
    bool found, miss;
    struct cursorCoords coords;

    //horizontal
    found = false;
    miss = false;
    screenX = px;
    screenY = gridWidth - 1;
    switch (gridRotation) { //find bottom edge square and seek direction depending on grid rotation
        case up0: //0deg upright
            gridX = px;
            gridY = gridWidth - 1;
            dx = 0;
            dy = -1;
            break;
        case left90: //90deg anticlockwise
            gridX = 0;
            gridY = px;
            dx = 1;
            dy = 0;
            break;
        case down180: //180deg
            gridX = gridWidth - 1 - px;
            gridY = 0;
            dx = 0;
            dy = 1;
            break;
        case right270: //90deg clockwise
            gridX = gridWidth - 1;
            gridY = gridWidth - 1 - px;
            dx = -1;
            dy = 0;
            break;
        default:
            break;
    }
    while (!found && !miss) {
        if ((grid[gridX][gridY].piece->type == block) && (heldPipe.piece->type == noPipe)) {
            miss = true;
            coords.foundHoriz = false;
            tmpCount = screenY;
            while (tmpCount > -1) {
                disableSprite(pathOA + tmpCount--, NULL);
            }
        } else if (grid[gridX][gridY].tile != emptyTile) {
            found = true;
            coords.foundHoriz = true;
            coords.hScreenY = screenY;
            tmpCount = screenY;
            while (tmpCount > -1) {
                disableSprite(pathOA + tmpCount--, NULL);
            }
        } else {
            gridX += dx;
            gridY += dy;
            screenY--;
            if (screenY == -1) {
                miss = true;
                coords.foundHoriz = false;
            }
            setSprite(pathOA + screenY + 1, (screenX << 4) + gridXOffset, ((screenY + 1) << 4) + gridYOffset, pointerStartTile + 12, -255, pointerPrio, true, true, 0);
        }
    }
    if (heldPipe.piece->type != noPipe) {
        gridX -= dx;
        gridY -= dy;
        screenY++;
        if (screenY == gridWidth) {
            miss = true;
            coords.foundHoriz = false;
        }
        if (found) disableSprite(pathOA + screenY, NULL);
    }
    coords.hGridX = gridX;
    coords.hGridY = gridY;
    coords.hScreenX = screenX;
    if (!miss)
        setSprite(cursorhOA, (screenX << 4) + gridXOffset, (screenY << 4) + gridYOffset, pointerStartTile + 8, -255, pointerPrio, false, false, 0);
    else
        disableSprite(cursorhOA, NULL);

    //vertical
    found = false;
    miss = false;
    screenX = 0;
    screenY = py;
    switch (gridRotation) { //find left edge square and seek direction depending on grid rotation
        case up0: //0deg upright
            gridX = 0;
            gridY = py;
            dx = 1;
            dy = 0;
            break;
        case left90: //90deg anticlockwise
            gridX = gridWidth - 1 - py;
            gridY = 0;
            dx = 0;
            dy = 1;
            break;
        case down180: //180deg
            gridX = gridWidth - 1;
            gridY = gridWidth - 1 - py;
            dx = -1;
            dy = 0;
            break;
        case right270: //90deg clockwise
            gridX = py;
            gridY = gridWidth - 1;
            dx = 0;
            dy = -1;
            break;
        default:
            break;
    }
    while (!found && !miss) {
        if (grid[gridX][gridY].piece->type == block) {
            miss = true;
            coords.foundVert = false;
            tmpCount = screenX;
            while (tmpCount < gridWidth) {
                disableSprite(pathOA + 8 + tmpCount++, NULL);
            }
        } else if (grid[gridX][gridY].tile != emptyTile) {
            found = true;
            coords.foundVert = true;
            tmpCount = screenX;
            while (tmpCount < gridWidth) {
                disableSprite(pathOA + 8 + tmpCount++, NULL);
            }
        } else {
            gridX += dx;
            gridY += dy;
            screenX++;
            if (screenX == gridWidth) {
                miss = true;
                coords.foundVert = false;
            }
            setSprite(pathOA + 8 + screenX - 1, ((screenX - 1) << 4) + gridXOffset, (screenY << 4) + gridYOffset, pointerStartTile + 14, -255, pointerPrio, true, true, 0);
        }
    }
    coords.vGridX = gridX;
    coords.vGridY = gridY;
    coords.vScreenX = screenX;
    coords.vScreenY = screenY;
    if (!miss)
        setSprite(cursorvOA, (screenX << 4) + gridXOffset, (screenY << 4) + gridYOffset, pointerStartTile + 10, -255, pointerPrio, false, false, 0);
    else
        disableSprite(cursorvOA, NULL);

    return coords;
}//struct cursorCoords placeCursor

//---------------------------------------------------------------------------------
void disableCursor ()
//---------------------------------------------------------------------------------
{
    disableSprite(cursorhOA, NULL);
    disableSprite(cursorvOA, NULL);
    for (int i = 0; i < 16; i++) {
        disableSprite(pathOA + i, NULL);
    }
}//void disableCursor

//---------------------------------------------------------------------------------
void drawQueue ()
//---------------------------------------------------------------------------------
{
    int bagNumQ = bagNumber;
    int bagIndQ = bagIndex;
    int max = (scrollQueue == 0) ? 8 : 9;
    for (int i = 0; i < max; i++) {
        setSprite(queueOA + i, gridXOffset + gridWidth * 16 - 6, gridYOffset + 7 * 16 - i * 16 - 8 + ((scrollQueue == 0) ? 0 : (16 - (scrollQueue >> 0))), pipeTileOffset + (bag[bagIndQ][bagNumQ].tile << 1), -queueObjAff, queuePrio, false, false, 0);
        bagIndQ++;
        if (bagIndQ == bagSize) {
            bagIndQ = 0;
            bagNumQ = 1 - bagNumQ;
        }
    }
}//void drawQueue
