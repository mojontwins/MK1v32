/*
    cpcrslib_mt.h

    CPCRSLIB "Mojon Tilemap" - pruned for MTE MK1 v3.2.

    This header declares ONLY the API that actually exists in
    cpcrslib_mt.lib / cpcrslib_fg_mt.lib (see cpcrslib.lst) and that the
    MK1 v3.2 engine uses. The original CPCRSLIB was a general purpose
    library; everything that was not oriented to the tilemap / dirty cells
    model (printing, key management, generic sprite and graphic utilities,
    the tilemap manager, alternative screen updaters) has been removed.

    Symbols not declared here but needed by the library itself
    (cpc_Bit2Mask, cpc_TblLookup, cpc_bitleft2mask, cpc_bitright2mask,
    cpc_UpdTileTableClp) are internal to the library.

    The engine side of the contract lives in dev/system/tilemap_conf.asm
    (tiles, pantalla_juego, tiles_tocados, posiciones_pantalla,
    posiciones_super_buffer, posicion_inicial_area_visible,
    posicion_inicial_superbuffer, ancho_pantalla_bytes,
    alto_pantalla_bytes, ancho_pantalla_bytes_visible, behindtilemasks)
    and in dev/cpc.h (viewport_x, viewport_y).
*/

#ifndef _CPCRSLIB_MT_H
#define _CPCRSLIB_MT_H

// Screen, tilemap and dirty cells

extern void  __FASTCALL__ __LIB__   cpc_ShowTileMap(int x);
extern void  __CALLEE__ __LIB__     cpc_UpdScr(void);
extern void  __CALLEE__ __LIB__     cpc_ShowTouchedTiles(void);
extern void  __CALLEE__ __LIB__     cpc_ResetTouchedTiles(void);

// Mode and video

extern void  __FASTCALL__ __LIB__   cpc_SetMode(unsigned char mode);
extern void  __CALLEE__ __LIB__     cpc_MakeM1RotationLUTs(void);

// Keyboard

extern char  __FASTCALL__ __LIB__   cpc_TestKey(unsigned char tecla);

// Sprites: one routine per size and pixel format.
// The engine picks them from the generated dev/cpc/spriteset_mappings.h
// (invfunc / updfunc), so the whole matrix has to be declared here.

extern void  __FASTCALL__ __LIB__   cpc_PutSpTileMap4x8Px(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutSpTileMap8x8Px(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutSpTileMap8x8PxM1(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutSpTileMap8x16Px(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutSpTileMap8x24Px(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutSpTileMap16x16PxM1(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutSpTileMap16x24PxM1(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutSpTileMap12x24CA(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutSPTileMap2Bx8(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutSPTileMap4Bx16(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutSPTileMap4Bx24(int sprite);

extern void  __FASTCALL__ __LIB__   cpc_PutTrSp4x8TileMapPx(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp4x8TileMapGPx(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp8x8TileMapPx(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp8x8TileMapPxM1(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp8x8TileMapGPxM1(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp8x16TileMapPx(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp8x16TileMapGPx(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp8x16TileMapGPxP(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp8x24TileMapPx(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp8x24TileMapGPx(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp8x24TileMapGPxP(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp16x16TileMapPxM1(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp16x16TileMapPxM1LUT(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp16x16TileMapGPxM1(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp16x16TileMapGPxM1P(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp16x24TileMapPxM1(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp16x24TileMapGPxM1P(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp12x24TileMapGCA(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp2Bx8TileMap(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp2Bx8TileMapG(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp4Bx16TileMap(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp4Bx16TileMapG(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp4Bx24TileMap(int sprite);
extern void  __FASTCALL__ __LIB__   cpc_PutTrSp4Bx24TileMapG(int sprite);

#endif
