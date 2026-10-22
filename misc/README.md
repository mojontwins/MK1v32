# misc

Ejemplos **standalone** que usan las librerías del repo directamente, sin
el motor, para ver cómo se manejan las APIs.

- [`zx-splib2-demo/`](zx-splib2-demo/) — ZX Spectrum, con **splib2**
  (Sprite Pack 2.2). Pantalla de tiles al azar y dos sprites de 16x16
  moviéndose encima, con la animación por desplazamiento de bitdef.
- [`cpc-cpcrslib-demo/`](cpc-cpcrslib-demo/) — Amstrad CPC, con
  **cpcrslib-mt** (CPCRSLIB "Mojon Tilemap"). Lo mismo, pero con el modelo
  de nametable + superbuffer + celdas tocadas, y usando el `cpc_UpdateNow()`
  del propio motor.

Los dos tienen un `build.bat` que los compila (necesitan el z88dk) y un
README con los detalles.

La referencia de las APIs está en [`docs/lib-splib2.md`](../docs/lib-splib2.md)
y [`docs/lib-cprslib.md`](../docs/lib-cprslib.md).
