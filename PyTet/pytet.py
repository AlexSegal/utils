#!/bin/env python

import sys
import time
import random
import colorsys

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

    PIECE_VALUES = {
        'I': 4,
        'J': 5,
        'L': 5,
        'O': 3,
        'S': 6,
        'T': 4,
        'Z': 6,
    }

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

    @property
    def value(self):
        return self.PIECE_VALUES[self.prototype]
    
    @classmethod
    def makeRandomColor(cls, h=(0, 1), s=(0, 1), v=(0, 1)):
        """Make a random color within the given hue/lightness/saturation ranges.
        """
        def remap(x, xmin, xmax):
            return x * (xmax - xmin) + xmin
        
        fresult = colorsys.hsv_to_rgb(remap(random.random(), h[0], h[1]),
                                        remap(random.random(), s[0], s[1]),
                                        remap(random.random(), v[0], v[1]))
        return tuple(int(x * 255) for x in fresult)


class Well:
    CELLS_X = 10
    CELLS_Y = 20

    HIT_NONE = 0
    HIT_SIDES = 1
    HIT_BOTTOM = 2
    HIT_SHARDS = 3
    GAME_OVER = 10

    def __init__(self):
        self.reset()

    def reset(self):
        self.shards = {}
        self.score = 0
        self.nrows = 0
        self.level = 1
        self.curPieceData = {}
        self.nextPieceData = self.makeNextPiece()

    def makeNextPiece(self):
        piece = Piece.makeRandom()
        color = Piece.makeRandomColor(s=(0.75, 1), v=(0.8, 0.9))
        self.nextPieceData = {
            'piece': piece,
            'color': color,
            'x': self.CELLS_X // 2 - piece.bboxsize // 2,
            'y': 1 - piece.bboxsize,
            'rot': 0
        }

    def dropNextPiece(self):
        self.curPieceData = self.nextPieceData
        self.makeNextPiece()

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
            return 0
        
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

        return piece.value

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
        """Check rows, collapse the fully populated ones, slide shards above.
        Return an int: num_destroyed_rows 
        """
        result = 0

        y = self.CELLS_Y - 1

        while y >=0:
            if all((x, y) in self.shards for x in range(0, self.CELLS_X)):
                self.removeShardLayer(y)
                self.dropCellsAboveCollapsedRow(y)
                result += 1
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

    def advance(self, dx=0, dy=1, drot=0):
        """Advance the time, attempt to move the falling piece 1 cell down, 
        and do whatever is necessary after that, e.g. shard a new piece and 
        collapse the shards etc.
        Return the hit event
        """
        if not self.curPieceData:
            self.dropNextPiece()

            if self.checkCollision(dx=0, dy=0, drot=0) != self.HIT_NONE:
                # Well is full up to the brim, stop the game!
                print("GAME OVER!")  
                return self.GAME_OVER
            
            return self.HIT_NONE
        
        hit = self.checkCollision(dx=dx, dy=dy, drot=drot)

        if hit == self.HIT_SIDES:
            return hit

        if hit == self.HIT_NONE:
            self.curPieceData['x'] += dx
            self.curPieceData['y'] += dy
            self.curPieceData['rot'] += drot
            return hit
        
        if hit in (self.HIT_BOTTOM, self.HIT_SHARDS):
            if dy == 0:
                # We hit the bottom or the shards, but we were moving sideways
                # or rotating, not falling, so it's okay to do nothing, as if 
                # we hit the sides:
                return hit
            
            value = self.shardPiece()
            nrows = self.checkAndCollapseShards()

            self.nrows += nrows
            self.score += value * nrows * self.CELLS_X * self.level
            self.level = max(self.level, self.nrows // 10 + 1)

            return hit

        raise RuntimeError('checkCollision returned unsupported value: ' \
                           '%s' % repr(hit))

  
class GraphicDevice:
    SCREEN_MIN_X = 0
    SCREEN_MAX_X = 960
    SCREEN_MIN_Y = 0
    SCREEN_MAX_Y = 720
    CELL_SIZE = 24

    def __init__(self, well):
        self.well = well
        pygame.init()
        pygame.key.set_repeat(200, 30)

        self.pgscreen = pygame.display.set_mode([self.SCREEN_MAX_X, 
                                                  self.SCREEN_MAX_Y])
        wellOrgX = (self.SCREEN_MAX_X + self.SCREEN_MIN_X) // 2 - \
                    Well.CELLS_X // 2 * self.CELL_SIZE
        wellOrgY = (self.SCREEN_MAX_Y + self.SCREEN_MIN_Y) // 2 - \
                    Well.CELLS_Y // 2 * self.CELL_SIZE
        self.wellBbox = (
            (wellOrgX, 
             wellOrgY), 
            (wellOrgX + Well.CELLS_X * self.CELL_SIZE, 
             wellOrgY + Well.CELLS_Y * self.CELL_SIZE)
         )

        sidePanelWidth = ((self.SCREEN_MAX_X - self.SCREEN_MIN_X) - \
                          (self.wellBbox[1][0] - self.wellBbox[0][0])) // 4
        sidePanelHeight = (self.SCREEN_MAX_Y - self.SCREEN_MIN_Y) // 3
        
        leftPanelOrgX = (self.wellBbox[0][0] + self.SCREEN_MIN_X) // 2 - \
                        sidePanelWidth // 2
        rightPanelOrgX = (self.wellBbox[1][0] + self.SCREEN_MAX_X) // 2 - \
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

        self.smallFont = pygame.font.SysFont(None, 22)
        self.largeFont = pygame.font.Font(pygame.font.get_default_font(), 32)

    def initPage(self):
        self.pgscreen.fill((127, 127, 127))

    @classmethod
    def flipPage(cls):
        pygame.display.flip()        

    def drawText(self, x, y, text, color=(255, 255, 255), font=None):
        if not font:
            font = self.smallFont
        textobj = font.render(text, True, color)
        textrect = textobj.get_rect(left=x, bottom=y)
        self.pgscreen.blit(textobj, textrect)

    def askUser(self, msg):
        self.drawText(250, self.SCREEN_MAX_Y - 60, 
                      msg + ' [Y/N]', 
                      font=self.largeFont)
        pygame.display.flip()

        while True:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    return False
                
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_y:
                        return True
                    elif event.key == pygame.K_n:
                        return False
        
    def mapWellCoordsToGraphicDevice(self, x, y):
        return (self.wellBbox[0][0] + x * self.CELL_SIZE,
                self.wellBbox[0][1] + y * self.CELL_SIZE)

    def drawCell(self, x, y, color):
        cx, cy = self.mapWellCoordsToGraphicDevice(x, y)
        bbox = (
            (cx + 1, cy + 1),
            (cx + self.CELL_SIZE - 1, cy + self.CELL_SIZE - 1)
        )
        pygame.draw.polygon(self.pgscreen, color, self.makeBboxVerts(bbox)) 

    @classmethod
    def makeBboxVerts(cls, bbox):
        minx, miny = bbox[0]
        maxx, maxy = bbox[1]
        return [
            (minx, miny), (maxx, miny), (maxx, maxy), (minx, maxy)
        ]

    def drawWellBG(self, color):
        pygame.draw.polygon(self.pgscreen, 
                             [x // 2 for x in color], 
                             self.makeBboxVerts(self.wellBbox))
        for x in range(self.well.CELLS_X):
            for y in range(self.well.CELLS_Y):
                self.drawCell(x, y, color)

    def drawWellContents(self):
        for x, y, color, is_piece in self.well.makeCellsForDrawing():
            self.drawCell(x, y, color)

    def drawLeftPanel(self, bgcolor=(50, 50, 50)):
        """Draw the score panel on the left
        """
        pygame.draw.polygon(self.pgscreen, 
                             bgcolor, 
                             self.makeBboxVerts(self.leftPanelBbox))
        x, y = self.leftPanelBbox[0]

        data = (
            ('Score:', self.well.score),
            ('Rows:',  self.well.nrows),
            ('Level:', self.well.level)
        )
        
        xx = x + 10
        yy = y + 35

        for label, value in data:
            self.drawText(xx, yy, label, (200, 200, 200), self.smallFont)
            yy += 40
            self.drawText(xx + 20, yy, str(value), (255, 120, 80), self.largeFont)
            yy += 20

    def drawRightPanel(self, bgcolor=(50, 50, 50)):
        pygame.draw.polygon(self.pgscreen, 
                             bgcolor, 
                             self.makeBboxVerts(self.rightPanelBbox)) 

    def getUserInput(self):
        result = {
            'device': 'keyboard',
            'command': None,
            'dx': 0,
            'drot': 0,
            'dlevel': 0,
            'free_fall': False,
        }

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                result['command'] = 'quit'
                return result
            
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_SPACE:
                    result['free_fall'] = True
                    result['dx'] = result['drot'] = 0
                    return result
                
                elif event.key == pygame.K_KP_PLUS:
                    result['dlevel'] += 1
                elif event.key == pygame.K_KP_MINUS:
                    result['dlevel'] -= 1
                else:
                    if event.key == pygame.K_LEFT:
                        result['dx'] -= 1
                    if event.key == pygame.K_RIGHT:
                        result['dx'] += 1
                    if event.key == pygame.K_UP:
                        result['drot'] -= 1
                    if event.key == pygame.K_DOWN:
                        result['drot'] += 1

        return result

class Game:
    def __init__(self):
        """Create the draw manager, initialize the screen for drawing
        """
        self.well = Well()
        self.gdevice = GraphicDevice(self.well)
        self.reset()

    def reset(self):
        self.well.reset()

    @property
    def fallPeriod(self):
        """Time between Y advancement events. The smaller, the faster the game.
        """
        return max(0, 0.5 - 0.5 * (self.well.level / 20))

    def loop(self):
        """Run this until the user asks to quit
        """
        # Fill the background:
        self.gdevice.initPage()

        freeFalling = False
        t = time.time()

        while True:
            self.gdevice.drawWellBG((40, 40, 40))
            self.gdevice.drawLeftPanel()
            self.gdevice.drawRightPanel()

            dx = 0
            drot = 0
            lastEvent = None
            
            userInput = self.gdevice.getUserInput()

            if userInput['command'] == 'quit':
                return 'quit'
            
            freeFalling += userInput['free_fall']

            dx += userInput['dx']
            drot += userInput['drot']
            
            dlevel = userInput['dlevel']

            if dlevel > 0:
                self.well.level += dlevel
            elif dlevel < 0:
                self.well.level = max(1, self.well.level - 1)

            # Check 1: only consider user input: move sideways or rotate:
            # Even if we hit something here, it would not cause sharding
            # of the piece!
            lastEvent = self.well.advance(dx, 0, drot)
                    
            if lastEvent == Well.GAME_OVER:
                # Nothing really to do here...
                pass
            else:
                # Check 2: apply dy=1
                if freeFalling or time.time() > t + self.fallPeriod:
                    t = time.time()
                    lastEvent = self.well.advance(dx=0, dy=1, drot=0)

                    if lastEvent in (Well.HIT_BOTTOM, Well.HIT_SHARDS):
                        freeFalling = False
                
            self.gdevice.drawWellContents()
            self.gdevice.drawLeftPanel()

            # Flip the display page
            self.gdevice.flipPage()

            if lastEvent == Well.GAME_OVER:
                return lastEvent

    def close(self):
        pygame.quit()
        self.pgscreen = None

    def askUserToRestart(self):
        return self.gdevice.askUser('Game Over. Restart?')


def main():
    game = Game()

    while True:
        if game.loop() == 'quit':
            break

        if game.askUserToRestart():
            game.reset()
        else:
            break
        
    game.close()

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        import traceback
        traceback.print_exc()
        time.sleep(60)
