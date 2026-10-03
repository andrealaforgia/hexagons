I want to make a game: the playfield is populated with hexagons with a number in them, from 1 to 1024.
The game is played by clicking on a hexagon and an empty cell and have that hexagon travel to that cell (shortest path).
The purpose of the game is to align 4 or mor hexagons. When that happens, the hexagons are removed and replaced by one hexagon containing the sum of all hexagons.
The new hexagon appears in the position the hexagon causing the merge travelled to.
Only hexagons with the same number count as a valid sequence.
When selecting a hexagon and an empty cell, that hexagon travels to the empty cell.
When the sum of the hexagons is beyond 1024, they all become solid wall, freezing the joining cell forever.
Every time a hexagon travels to a position but no merge happens, a new set of hexagons appear in random positions. Again, 1% of the total number of hexagons. There should be no automatic merges following the appearance of the hexagons.
The hexagons are randomly coloured and fluctuate up and down a bit. Each hexagon has a tick coloured border and the number written at the centre.
Clicking on a hexagon selects it. Clicking again unselects it. A selected hexagon changes its black background to a different colour (a light tone of its border).
The game should be written like ~/dev/games/asteroids, using the same engine as a git submodule. Add support for the mouse to the engine.
The game runs in full screen and the grid of hexagons fills up the full screen. However, at the beginning, only a limited number of hexagons appears, 10% of the total number of hexagons.
