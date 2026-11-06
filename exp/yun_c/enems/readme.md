# Enemies

`level0.ene` .. `level2.ene`: one per level of the load, ponedor format with
3-byte hotspots (`3b` on top of the edit grid). Each one points at its map
(`..\map\levelN.map`) and tileset (`..\gfx\speccy\tsN.png`).

    ponedor.bat level0.ene

The build (`dev/comp.bat`, `dev/comp_cpc.bat`) runs `ene2h ... bin=` on them:
no need to export `enems.h` from ponedor.

Types: 1-3 linear (the level's three enemies, cells of `spritesN.png`),
4 moving platform, 5 fanty, 6 monococo. Hotspots: 1 star (extra life),
2 key, 3 refill.
