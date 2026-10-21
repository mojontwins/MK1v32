# MK1v3.2 diary of things.

## Mejor disparasión

Rehacer la disparasión para soportar los modos de Goku Mal. Esto incluye:

### `#define CELL_ALIGNED_BULLETS`

Para que las balas se rendericen más rápido, se alinean a la rejilla. Esto evita que los sprites se tengan que rotar y, en CPC, supone una ganancia (en ZX da igual, todas las rotaciones tiran de LUT).

### `#define SHOOT_4_WAY`

Se puede disparar en todas las direcciones, 

* Esto activa el array `bullets_my`.
* Si estamos en perspectiva genital, activar `PLAYER_CAN_FIRE` activa esto automáticamente.
* Si estamos en perspectiva lateral, hay que activarlo específicamente y para que los disparos tengan componente `my` se debe pulsar `FIRE` en combinación con `UP` or `DOWN`. Si además se está pulsando `LEFT` o `RIGHT` tendrán componente mx, así:

```
	// Player pulsó FIRE este frame.

	if (UP || DOWN) {
		if (UP) {
			my = -V;
		} else {
			my = V;
		}

		mx = 0;
		if (LEFT) {
			mx = -V;
		} else {
			mx = V;
		}

	} else {
		my = 0;
		if (facing (LEFT)) {
			mx = -V;
		} else {
			mx = V;
		}

	}
```

## Escalerasión

Si se activa la escalerasión, debe haber un detector que detecte "tocando escalera" (`hit_stairs`) y un flag `on_stairs` y eso defina un contexto.

* Si `!on_stairs`, pero `hit_stairs` y pulsamos UP|DOWN -> `on_stairs = 1`.
* Si `on_stairs` -> gravedad no aplica.
* Si `on_stairs` -> podemos iniciar un salto, lo cual deberá poner `on_stairs = 0`.
* Si `on_stairs` y `!hit_stairs` -> `on_staris = 0`.
* Si `on_stairs` -> UP|DOWN suben y bajan, LEFT|RIGHT como normalmente, frame específico que tiene que ver con `(y >> 3) | 1`.

Vamos a probar a hacer que `hit_stairs` cubra un área que haga que sea sencillo encararmarse a las escaleras. Por ejemplo, un rectángulo `[x+4, x+11]` e `[y+8,y+16]` (cubre un pixel por debajo).

Hay que detectar 4 attr, recuerda que:

```c
	// If you put x in C and y in A you can call here
	._attr_2
```

