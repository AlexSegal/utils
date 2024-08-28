#!/bin/env python

import sys
import time
import random

try:
    import pygame
except ImportError as e:
    print('%s. Please install it: "pip install pygame"' % e)
    sys.exit(1)


class Piece:
    CELLMAPS = {
        'I': [
            (
                '    ',
                'XXXX',
                '    ',
                '    ',
            ),
            (
                '  X ',
                '  X ',
                '  X ',
                '  X ',
            ),
            0,
            1
        ],
        'J': [
            (
                '   ',
                'XXX',
                '  X',
            ),
            (
                ' X ',
                ' X ',
                'XX ',
            ),
            (
                'X  ',
                'XXX',
                '   ',
            ),
            (
                ' XX',
                ' X ',
                ' X ',
            ),
        ], 
        'L': [
            (
                '   ',
                'XXX',
                'X  ',
            ),
            (
                'XX ',
                ' X ',
                ' X ',
            ),
            (
                '  X',
                'XXX',
                '   ',
            ),
            (
                ' X ',
                ' X ',
                ' XX',
            ),
        ],
        'O': [
            (
                '    ',
                ' XX ',
                ' XX ',
                '    ',
            ),
            0, 
            0, 
            0
        ],
        'S': [
            (
                '   ',
                ' XX',
                'XX ',
            ),
            (
                ' X ',
                ' XX',
                '  X',
            ),
            0,
            1
        ],
        'T': [
            (
                '   ',
                'XXX',
                ' X ',
            ),
            (
                ' X ',
                'XX ',
                ' X ',
            ),
            (
                ' X ',
                'XXX',
                '   ',
            ),
            (
                ' X ',
                ' XX',
                ' X ',
            ),
        ],
        'Z': [
            (
                '   ',
                'XX ',
                ' XX',
            ),
            (
                '  X',
                ' XX',
                ' X ',
            ),
            0,
            1
        ]
    }

    PROTOTYPES = sorted(CELLMAPS.keys())

    def __init__(self, prototype):
        self.prototype = prototype
        self.rotCellMaps = self.CELLMAPS[prototype]
        self.bboxsize = len(self.rotCellMaps[0][0]) # assuming they are square

    def __repr__(self):
        return '%s(%r)' % (type(self).__name__, self.prototype)

    def getCellMap(self, rot=0):
        result = self.rotCellMaps[rot % 4]

        if not isinstance(result, int):
            return result
         
        # Integer cellmap is just a reference to another angle:
        return self.getCellMap(result)
    
    def getCellCoords(self, dx=0, dy=0, rot=0):
        """Return a list of integer (x, y) pairs: coordinates of the piece
        cells, corresponding to the specified rotation (0..3 == 0..270 degrees). 
        Add dx and dy to all of them.
        """
        cellmap = self.getCellMap(rot)
        result = []

        for y, s in enumerate(cellmap):
            if s.strip():
                for x, ch in enumerate(s):
                    if ch.strip():
                        result.append((x + dx, y + dy))

        return result

    @classmethod
    def makeRandom(cls):
        """Make an instance of the class with a random prototype
        """
        return cls(random.choice(cls.PROTOTYPES))


class Well:
    CELLS_X = 10
    CELLS_Y = 20

    HIT_NONE = 0
    HIT_SIDES = 1
    HIT_BOTTOM = 2
    HIT_SHARDS = 3
    GAME_OVER = 10

    def __init__(self):
        self.shards = {}
        self.curPieceData = {}

    def addPiece(self, piece, color=(255, 255, 255)):
        self.curPieceData = {
            'piece': piece,
            'color': color,
            'x': self.CELLS_X // 2 - piece.bboxsize // 2,
            'y': -piece.bboxsize,
            'rot': 0
        }

    def checkCollision(self, dx, dy, drot):
        """Check the hypotetical/future position of the piece and 
        return one of HIT_* values.
        Abs values of dx, dy and drot cannot be more than 1!
        """
        if not self.curPieceData:
            return False

        assert(abs(dx) <= 1)
        assert(abs(dy) <= 1)
        assert(abs(drot) <= 1)

        piece = self.curPieceData['piece']
        x = self.curPieceData['x'] + dx
        y = self.curPieceData['y'] + dy
        rot = self.curPieceData['rot'] + drot

        # Hypothetical new transform:
        cellCoords = piece.getCellCoords(x, y, rot)
        
        # Check against wells boundaries:
        for x, y in cellCoords:
            if x < 0 or x >= self.CELLS_X:
                return self.HIT_SIDES

            if y >= self.CELLS_Y:
                return self.HIT_BOTTOM
            
        # Check against the existing shards:
        return self.HIT_SHARDS if any(k in self.shards for k in cellCoords) \
               else self.HIT_NONE

    def shardPiece(self):
        if not self.curPieceData:
            return
        
        piece = self.curPieceData['piece']
        x = self.curPieceData['x']
        y = self.curPieceData['y']
        rot = self.curPieceData['rot']
        color = self.curPieceData['color']

        for x, y in piece.getCellCoords(x, y, rot):
            if x >= 0 and x < self.CELLS_X and \
               y >= 0 and y < self.CELLS_Y:
                self.shards[(x, y)] = color

        self.curPieceData = {}

    def removeShardLayer(self, row):
        """Remove the row of shards, but that's it. Don't attempt to drop the
        cells above.
        """
        for x, y in list(self.shards.keys()):
            if y == row:
                self.shards.pop((x, y))

    def dropCellsAboveCollapsedRow(self, row):
        """Drop sharded cells above the specified row, into empty spaces below.
        """
        while True:
            anyChanges = False

            for eachrow in range(row-1, -1, -1):
                rowXYs = [(x, y) for (x, y) in self.shards.keys() if y == eachrow]
                for x, y in rowXYs:
                    if (x, y+1) not in self.shards:
                        self.shards[(x, y+1)] = self.shards.pop((x, y))
                        anyChanges = True

            if not anyChanges:
                return

    def checkAndCollapseShards(self):
        result = 0

        y = self.CELLS_Y - 1

        while y >=0:
            if all((x, y) in self.shards for x in range(0, self.CELLS_X)):
                self.removeShardLayer(y)
                self.dropCellsAboveCollapsedRow(y)
                result += self.CELLS_X 
            else:
                y -= 1
        
        return result

    def makeCellsForDrawing(self):
        """Return a full list of cells to be drawn. Each element is a tuple:
        (x, y, (R, G, B), is_piece)
        """
        result = []

        if self.curPieceData:
            piece = self.curPieceData['piece']
            x = self.curPieceData['x']
            y = self.curPieceData['y']
            rot = self.curPieceData['rot']
            color = self.curPieceData['color']

            # Current piece:
            for (x, y) in piece.getCellCoords(x, y, rot):
                if x >= 0 and x < self.CELLS_X and \
                   y >= 0 and y < self.CELLS_Y:
                    result.append((x, y, color, True))

        # Shards:
        for (x, y), color in sorted(self.shards.items()):
            result.append((x, y, color, False))

        return result

    def advance(self, dx=0, dy=1, drot=0, nextPiece=None, nextPieceColor=None):
        """Advance the time, attempt to move the falling piece 1 cell down, 
        and do whatever is necessary after that, e.g. shard a new piece and 
        collapse the shards etc.
        Return a 2-element tuple: the hit event and one of:
            * number of collapsed shards if we hit the bottom/other shards, 
            * zero if nothing important happened, 
            * or a negative value: game over
        """
        score = 0

        if not self.curPieceData:
            if not nextPiece:
                nextPiece = Piece.makeRandom()
                score += 4  # TODO?
            
            color = nextPieceColor or (random.randint(50, 250), 
                                       random.randint(50, 250), 
                                       random.randint(50, 250))

            self.addPiece(nextPiece, color)

            if self.checkCollision(dx=0, dy=0, drot=0) != self.HIT_NONE:
                # Well is full up to the brim, stop the game!
                return (self.GAME_OVER, score)
            
            return (self.HIT_NONE, score)
        
        hit = self.checkCollision(dx=dx, dy=dy, drot=drot)

        if hit == self.HIT_SIDES:
            return (hit, 0)

        if hit == self.HIT_NONE:
            self.curPieceData['x'] += dx
            self.curPieceData['y'] += dy
            self.curPieceData['rot'] += drot
            return (hit, score)
        
        if hit in (self.HIT_BOTTOM, self.HIT_SHARDS):
            if dy == 0:
                # We hit the bottom or the shards, but we were moving sideways
                # or rotating, not falling, so it's okay to do nothing, as if 
                # we hit the sides:
                return (hit, score)
            
            self.shardPiece()
            return (hit, score + self.checkAndCollapseShards())

        raise RuntimeError('checkCollision returned unsupported value: ' \
                           '%s' % repr(hit))
    

SCREEN_MIN_X = 0
SCREEN_MAX_X = 1024
SCREEN_MIN_Y = 0
SCREEN_MAX_Y = 768

class Canvas:
    def __init__(self, cellSize=20):
        """Create the draw manager, initialize the screen for drawing
        """
        pygame.init()
        pygame.key.set_repeat(300, 200)

        self.screen = pygame.display.set_mode([SCREEN_MAX_X, SCREEN_MAX_Y])
        self.cellSize = cellSize
        self.well = Well()
        wellOrgX = (SCREEN_MAX_X + SCREEN_MIN_X) // 2 - Well.CELLS_X // 2 * cellSize
        wellOrgY = (SCREEN_MAX_Y + SCREEN_MIN_Y) // 2 - Well.CELLS_Y // 2 * cellSize
        self.wellBbox = (
            (wellOrgX, 
             wellOrgY), 
            (wellOrgX + Well.CELLS_X * cellSize, 
             wellOrgY + Well.CELLS_Y * cellSize)
         )

        sidePanelWidth = ((SCREEN_MAX_X - SCREEN_MIN_X) - \
                          (self.wellBbox[1][0] - self.wellBbox[0][0])) // 4
        sidePanelHeight = (SCREEN_MAX_Y - SCREEN_MIN_Y) // 4
        
        leftPanelOrgX = (self.wellBbox[0][0] + SCREEN_MIN_X) // 2 - \
                        sidePanelWidth // 2
        rightPanelOrgX = (self.wellBbox[1][0] + SCREEN_MAX_X) // 2 - \
                        sidePanelWidth // 2
        
        self.leftPanelBbox = (
            (leftPanelOrgX, 
             wellOrgY),
            (leftPanelOrgX + sidePanelWidth,
             wellOrgY + sidePanelHeight)
        )

        self.rightPanelBbox = (
            (rightPanelOrgX, 
             wellOrgY),
            (rightPanelOrgX + sidePanelWidth,
             wellOrgY + sidePanelHeight)
        )

        self.sysFont = pygame.font.SysFont(None, 24)

        # Fill the background:
        self.screen.fill((127, 127, 127))
        self.score = 0
        self.frameDelay = 0.3

    def drawText(self, x, y, text, color=(255, 255, 255), font=None):
        if not font:
            font = self.sysFont
        textobj = font.render(text, True, color)
        textrect = textobj.get_rect(left=x, bottom=y)
        self.screen.blit(textobj, textrect)

    def mapCellCoordsToCanvas(self, x, y):
        return (self.wellBbox[0][0] + x * self.cellSize,
                self.wellBbox[0][1] + y * self.cellSize)

    def drawCell(self, x, y, color):
        cx, cy = self.mapCellCoordsToCanvas(x, y)
        verts = [
            (cx + 1,                 cy + 1),             
            (cx + self.cellSize - 1, cy + 1),
            (cx + self.cellSize - 1, cy + self.cellSize - 1),
            (cx + 1,                 cy + self.cellSize - 1)
        ]
        pygame.draw.polygon(self.screen, color, verts) 

    @classmethod
    def makeBboxVerts(cls, bbox):
        minx, miny = bbox[0]
        maxx, maxy = bbox[1]
        return [
            (minx, miny), (maxx, miny), (maxx, maxy), (minx, maxy)
        ]

    def drawWellBG(self, color):
        return pygame.draw.polygon(self.screen, 
                                    color, 
                                    self.makeBboxVerts(self.wellBbox)) 

    def drawWellContents(self):
        for x, y, color, is_piece in self.well.makeCellsForDrawing():
            self.drawCell(x, y, color)

    def drawLeftPanel(self, bgcolor=(50, 50, 50)):
        """Draw the score panel on the left
        """
        pygame.draw.polygon(self.screen, 
                             bgcolor, 
                             self.makeBboxVerts(self.leftPanelBbox))
        x, y = self.leftPanelBbox[0]
        self.drawText(x+10, y+20, 'Score: %d' % self.score) 

    def drawRightPanel(self, bgcolor=(50, 50, 50)):
        pygame.draw.polygon(self.screen, 
                             bgcolor, 
                             self.makeBboxVerts(self.rightPanelBbox)) 

    def loop(self):
        """Run this until the user asks to quit
        """
        freeFalling = False
        t = time.time()

        self.drawLeftPanel()
        self.drawRightPanel()

        while True:
            self.drawWellBG((30, 30, 30))
            self.drawLeftPanel()

            dx = 0
            drot = 0
            
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    return
                
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_SPACE:
                        freeFalling = True
                        dx = 0
                        drot = 0 
                    else:
                        if event.key == pygame.K_LEFT:
                            dx = -1
                        if event.key == pygame.K_RIGHT:
                            dx = 1
                        if event.key == pygame.K_UP:
                            drot = -1
                        if event.key == pygame.K_DOWN:
                            drot = 1

            if not freeFalling:
                # Check 1: only consider user input: move sideways or rotate:
                # Even if we hit something here, it would not cause sharding
                # of the piece!
                if dx or drot:
                    self.well.advance(dx, 0, drot)[0]

            # Check 2: apply dy=1
            if freeFalling or time.time() > t + self.frameDelay:
                t = time.time()
                hit, score = self.well.advance(dx=0, dy=1, drot=0)
                self.score += score

                if hit in (Well.HIT_BOTTOM, Well.HIT_SHARDS):
                    freeFalling = False

            self.drawWellContents()

            # Flip the display page
            pygame.display.flip()        


    def close(self):
        pygame.quit()
        self.screen = None

def main():
    canvas = Canvas()
    canvas.loop()
    canvas.close()

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        import traceback
        traceback.print_exc()
        time.sleep(60)
