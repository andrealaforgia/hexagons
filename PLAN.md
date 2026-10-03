# Hexagons: implementation plan

Each iteration ends with a game you can launch and use, a green test suite and a commit on main.
The spec is in `SPEC.md`.

## Shape of the code

Same layout as `~/dev/games/asteroids`: C99, SDL2, `engine` as a git submodule, `game/src/...`, `tests/test_game.c` driven by `tests/run_tests.py`, `make test` and `make sanitize`.

The rules live in a pure module with no SDL in it (`game/src/board`): grid, cells, pathfinding, line detection, merging, spawning. It takes a seeded random source so tests are deterministic. Everything in iterations 2 to 8 is test-driven against this module.

The SDL side (stage, rendering, input) stays thin: it turns a click into a cell, calls the board and draws the result. It is covered by the build test and by a headless stage test, as in asteroids.

## Iterations

### 0. Walking skeleton (done)
Usable: `make` builds, the game opens full screen on a black screen, Esc quits.
Work: `git init`, add the engine submodule, Makefile, test runner, `.gitignore`, CI workflow copied from asteroids.
First test: the build test, then a stage test that creates and destroys the playing stage with no leaked allocations.

### 1. The grid fills the screen (done)
Usable: an empty hexagonal grid covers the whole screen, and the cell under the mouse is highlighted.
Work: axial hex coordinates, grid dimensions derived from screen size and hex radius, cell to pixel centre, pixel to cell.
First test: pixel to cell of a cell's own centre returns that cell, for every cell in the grid.
Engine: the mouse module already exists (position and button state). Check it is enough before adding anything.

### 2. Hexagons on the board (done)
Usable: 10% of the cells hold a hexagon with a thick coloured border, black fill and its number at the centre.
Work: cell states (empty, hexagon with value and colour), initial population from the seeded random source, number rendering.
First test: a new board of N cells holds exactly round(N / 10) hexagons, all on distinct cells.
Decided: new hexagons carry a random number from 1 to 8, and the border colour is set by the number.

### 3. Selection (done)
Usable: click a hexagon to select it, its fill turns to a light tone of its border; click it again to unselect; click another hexagon to move the selection.
Work: selection state on the board, click edge detection (a press is one click, not one per frame), light tone colour function.
First test: selecting the selected cell clears the selection.

### 4. Movement (done)
Usable: with a hexagon selected, click an empty cell and the hexagon goes there if a path exists; otherwise nothing happens.
Work: breadth-first search over the six neighbours through empty cells only, move applied to the board, then animation of the hexagon along the returned path.
First test: the path between two cells on an empty board has length equal to their hex distance. Second: a hexagon enclosed by others has no path.

### 5. Merging (done)
Usable: moving a hexagon so that it forms a line of four or more equal numbers removes the line and leaves one hexagon with the sum on the destination cell.
Work: line detection through the destination cell along the three hex axes, merge applied to the board.
First test: three 2s in a row plus a fourth 2 moved to the end leaves a single 8 on the destination and three empty cells.
Decided: every merge is worth four times the number, however many hexagons or lines are involved, so that numbers stay powers of two.

### 6. Spawning (done)
Usable: a move that does not merge brings in new hexagons, 1% of the cell count (at least one), and they never trigger a merge on their own. This is the first iteration that is a real game.
Work: spawn on random empty cells after a non-merging move.
First test: after a non-merging move the hexagon count grows by round(N / 100); after a merging move it does not grow.
Decision needed: how "no automatic merges" is honoured (see open decisions).

### 7. Walls
Usable: a merge whose sum would exceed 1024 produces solid wall instead; walls cannot be selected, moved or crossed.
Work: wall cell state, pathfinding treats walls as blocked, wall rendering.
First test: four 512s in a line produce wall and no 2048.
Decision needed: which cells become wall (see open decisions).

### 8. Start, end and score
Usable: intro screen, play, game over when no move is possible, score shown, restart without relaunching.
Work: intro and game over stages as in asteroids, game over detection, score.
First test: a board with no empty cell reports game over.
Decision needed: losing condition and scoring (see open decisions).

### 9. Polish
Usable: hexagons bob up and down slightly, each with its own phase; sounds for select, move, merge, wall and game over.
Work: bobbing as a pure function of time and cell, applied at render time only so it never affects hit testing.
First test: the bobbing offset stays within its amplitude for any time value.

## Open decisions

Each one blocks the iteration named, not the ones before it.

1. Spawn values (iteration 2). Decided by Andrea: every new hexagon is a random number from 1 to 8. Each number has its own border colour, which replaces the random colouring in the spec.
2. Crossing lines (iteration 5). Decided by Andrea: numbers are powers of two, so a merge is always worth four times the number. Every qualifying line through the destination is cleared.
3. No automatic merges after a spawn (iteration 6). Proposal: lines formed by spawning are left alone; only the player's move is checked.
4. Wall rule (iteration 7). Proposal: every hexagon in the line becomes wall.
5. Losing condition (iteration 8). Decided by Andrea: the game ends when no hexagon can move. "GAME OVER" flashes in yellow at the centre, with a small "Press space to restart or ESC to exit" bobbing gently. Score is not decided.
6. Unreachable target (iteration 4). Proposal: the click is ignored and the selection stays.
