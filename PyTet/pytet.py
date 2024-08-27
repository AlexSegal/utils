#!/bin/env python

import time
import random
import pygame

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

    def canTransformPiece(self, dx, dy, drot):
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

    def collapseShardRow(self, row):
        for x, y in list(self.shards.keys()):
            if y == row:
                self.shards.pop((x, y))
            elif y < row:
                self.shards[(x, y+1)] = self.shards.pop((x, y))

    def checkAndCollapseShards(self):
        result = 0

        y = self.CELLS_Y - 1

        while y >=0:
            if all((x, y) in self.shards for x in range(0, self.CELLS_X)):
                self.collapseShardRow(y)
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
        Return either the number of collapsed shards, zero, or a negative value:
        game over.
        """
        if not self.curPieceData:
            if not nextPiece:
                nextPiece = Piece.makeRandom()
            
            color = nextPieceColor or (random.randint(50, 235), 
                                           random.randint(50, 235), 
                                           random.randint(50, 235))

            self.addPiece(nextPiece, color)

            if self.canTransformPiece(dx=0, dy=0, drot=0) != self.HIT_NONE:
                # Well is full up to the brim, stop the game!
                return -1
            
            return 0
        
        hit = self.canTransformPiece(dx=dx, dy=dy, drot=drot)

        if hit == self.HIT_SIDES:
            return 0

        if hit == self.HIT_NONE:
            self.curPieceData['x'] += dx
            self.curPieceData['y'] += dy
            self.curPieceData['rot'] += drot
            return 0
        
        if hit in (self.HIT_BOTTOM, self.HIT_SHARDS):
            self.shardPiece()
            return self.checkAndCollapseShards()

        raise RuntimeError('canTransformPiece returned unsupported value: ' \
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
        self.screen = pygame.display.set_mode([SCREEN_MAX_X, SCREEN_MAX_Y])
        self.cellSize = cellSize
        self.well = Well()
        wellOrgX = (SCREEN_MAX_X + SCREEN_MIN_X) // 2 - Well.CELLS_X // 2 * cellSize
        wellOrgY = (SCREEN_MAX_Y + SCREEN_MIN_Y) // 2 - Well.CELLS_Y // 2 * cellSize
        self.wellBbox = ((wellOrgX, 
                          wellOrgY), 
                         (wellOrgX + Well.CELLS_X * cellSize, 
                          wellOrgY + Well.CELLS_Y * cellSize))
        # Fill the background:
        self.screen.fill((127, 127, 127))
            
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

    def drawWellBG(self, color):
        wminx, wminy = self.wellBbox[0]
        wmaxx, wmaxy = self.wellBbox[1]
        verts = [
            (wminx, wminy), (wmaxx, wminy), (wmaxx, wmaxy), (wminx, wmaxy)
        ]
        pygame.draw.polygon(self.screen, color, verts) 

    def drawWellContents(self):
        for x, y, color, is_piece in self.well.makeCellsForDrawing():
            self.drawCell(x, y, color)

    def loop(self):
        # Run until the user asks to quit
        rot = 0

        while True:
            self.drawWellBG((20, 40, 0))

            dx = 0
            dy = 1
            drot = 0
            
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    break
                
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_LEFT:
                        dx = -1
                    if event.key == pygame.K_RIGHT:
                        dx = 1
                    if event.key == pygame.K_UP:
                        drot = -1
                    if event.key == pygame.K_DOWN:
                        drot = 1

            # Check 1: only consider user input with no falling down:
            if dx or drot:
                ret = self.well.advance(dx, 0, drot)

                if ret < 0:
                    break

            # Check 2: apply dy
            ret = self.well.advance(0, dy, 0)

            if ret < 0:
                break

            self.drawWellContents()

            # Flip the display page
            pygame.display.flip()        

            time.sleep(0.25)

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
