// Estructura para las distintas pantallas,
// 10 + 6 + 20 + 1 = 37 bytes por pantalla.

typedef struct
{
	unsigned char descriptor;
	unsigned int obj[10];
	unsigned int movil[3];
	unsigned char coin[10];
} FASE;

extern FASE fases[];

// Definición de las pantallas del juego según esta documentación:
/*
B:  L:   C:
0   1    Byte descriptor de fase: TTPPPIII 
         TT : 2 bits, número de tile de fondo (0 a 3) 
         PPP: 3 bits, color de PAPER (0 a 8) 
         III: 3 bits, color de INK (0 a 8) 

1   2    Descriptor de objeto, 2 bytes: XXXXYYYYLLLLTTTD 
         XXXX: 4 bits, coordenada X en tiles de la esquina sup.izq (0 a 15) 
         YYYY: 4 bits, coordenada Y en tiles de la esquina sup.izq (0 a 15) 
         LLLL: 4 bits, longitud del objeto en tiles (0 a 15) 
         TTT: 3 bits, número de objeto (0 a 7) 
         D: 1 bit, dirección: 0 horz, 1 vert. 

         P.ej. una plataforma de 6 tiles de ancho en (2, 2) del tipo 2, sería: 
         0010001001100100 
         [ 2][ 2][ 6][2]0 

... x 10 
21   2   Descriptor de enemigo, 2 bytes: AAAABBBBYYYYRTTV 
         AAAA: 4 bits, nº tile horizontal límite izquierdo (0 a 15) 
         BBBB: 4 bits, nº tile horizontal límite derecho (0 a 15) 
         YYYY: 4 bits, nº tile vertical (0 a 15) 
         R: 1 bit, reservado (por ahora no sirve pa ná).
         TT: 2 bits, tipo de enemigo (0 a 3); 1 = franky, 2 = vampy, 3 = fanty.
         V: 1 bit, velocidad: 0 lento (de 1 en 1 pixels), 1 rápido (de 2 en 2 píxels). 
... x 3 
27   1   Descriptor de moneda, 1 byte: XXXXYYYY 
         XXXX: 4 bits, coordenada X en tiles (0 a 15) 
         YYYY: 4 bits, coordenada Y en tiles (0 a 15) 
... x 10
*/
#asm

._fases

;.Pantalla 0
        ;TTPPPIII
   defb @00001101            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11000010, @00000000
   defb @00110000, @00010011
   defb @00110000, @10000011
   defb @00100000, @01010101
   defb @00110000, @00010111
   defb @00110000, @10000111
   defb @11001100, @00001001
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010101, @00011010
   defb @01100011, @00011010
   defb @00000000, @00000000


        ;Monedas
        ;XXXXYYYY
   defb @00010010
   defb @00110010
   defb @10010010
   defb @10100010
   defb @01010100
   defb @01100100
   defb @00010110
   defb @00110110
   defb @10000110
   defb @10100110

;.Pantalla 1
        ;TTPPPIII
   defb @00000100            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11000100, @00000000
   defb @00110101, @00000001
   defb @00010111, @10100010
   defb @00010111, @10010011
   defb @00110110, @00100101
   defb @00110110, @10000101
   defb @00110101, @10110110
   defb @00110110, @01010111
   defb @11000100, @00001001
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100010, @01101001
   defb @01010000, @01010111
   defb @00000000, @00000000


        ;Monedas
        ;XXXXYYYY
   defb @10010010
   defb @00100100
   defb @00110100
   defb @01000100
   defb @10000100
   defb @10110101
   defb @01010110
   defb @01100110
   defb @01110110
   defb @00011000

;.Pantalla 2
        ;TTPPPIII
   defb @00001111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11000010, @00000000
   defb @10111100, @00010011
   defb @00011101, @00000101
   defb @10011100, @00000110
   defb @00011101, @10110111
   defb @11000000, @00001001
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100101, @00001011
   defb @01010010, @00011011
   defb @10000100, @00101010


        ;Monedas
        ;XXXXYYYY
   defb @00010001
   defb @01010001
   defb @10010001
   defb @00010100
   defb @01010100
   defb @10010100
   defb @10110110
   defb @00010111
   defb @01010111
   defb @10010111

;.Pantalla 3
        ;TTPPPIII
   defb @00000011            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @10010000, @00000000
   defb @00110101, @10110000
   defb @01010101, @00000001
   defb @10010000, @00110011
   defb @00100101, @10110100
   defb @00011001, @00010101
   defb @01110000, @00100110
   defb @00011000, @10011000
   defb @11000000, @00001001
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100010, @00011010
   defb @01000011, @00011010
   defb @01110000, @00101010


        ;Monedas
        ;XXXXYYYY
   defb @00100001
   defb @01000001
   defb @01100001
   defb @10000001
   defb @10100001
   defb @00010100
   defb @00110100
   defb @01010100
   defb @01110100
   defb @10010100

;.Pantalla 4
        ;TTPPPIII
   defb @00010111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11001100, @00000000
   defb @00101000, @00000011
   defb @00101000, @01010011
   defb @00101000, @10100011
   defb @00011001, @01000100
   defb @00110010, @00000110
   defb @00100010, @01010110
   defb @00110010, @10010110
   defb @00011011, @01111000
   defb @11001010, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100010, @00000101
   defb @00100011, @01101011
   defb @01010011, @00001011


        ;Monedas
        ;XXXXYYYY
   defb @00000010
   defb @00110010
   defb @01010010
   defb @01100010
   defb @10000010
   defb @10110010
   defb @00000101
   defb @00100101
   defb @10010101
   defb @10110101

;.Pantalla 5
        ;TTPPPIII
   defb @00000110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11001010, @00000000
   defb @01100111, @10110001
   defb @00010111, @01000010
   defb @01010111, @00010011
   defb @00010111, @01100011
   defb @00100110, @01100101
   defb @00010111, @10000110
   defb @00110110, @00100111
   defb @00110110, @01111000
   defb @11001010, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010011, @00001010
   defb @01000100, @00100101
   defb @00000000, @00000000


        ;Monedas
        ;XXXXYYYY
   defb @00000010
   defb @01100010
   defb @10000010
   defb @00000011
   defb @00000100
   defb @00000101
   defb @00000110
   defb @00100110
   defb @00110110
   defb @01000110

;.Pantalla 6
        ;TTPPPIII
   defb @00010110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11001100, @00000000
   defb @01110101, @00000001
   defb @01110101, @10110001
   defb @00011001, @01010011
   defb @00011001, @10010100
   defb @00011001, @01100101
   defb @00011001, @01000110
   defb @00011001, @00100111
   defb @00011001, @10000111
   defb @11000000, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010011, @00011010
   defb @01000100, @00011000
   defb @01110001, @00110111


        ;Monedas
        ;XXXXYYYY
   defb @01000010
   defb @01100010
   defb @10010010
   defb @01000011
   defb @10010011
   defb @00100101
   defb @01000101
   defb @10000101
   defb @00100110
   defb @10000110

;.Pantalla 7
        ;TTPPPIII
   defb @00000011            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @01010111, @01100001
   defb @01110111, @10100001
   defb @00010111, @10110001
   defb @00010111, @00100010
   defb @01010111, @10000010
   defb @00110110, @00000011
   defb @00100111, @01000011
   defb @00010111, @10110101
   defb @01010010, @00010111
   defb @11000000, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000101, @00001011
   defb @01010000, @00000101
   defb @00000000, @00000000


        ;Monedas
        ;XXXXYYYY
   defb @01010010
   defb @01110010
   defb @10010010
   defb @01110011
   defb @10010011
   defb @01110100
   defb @10010100
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 8
        ;TTPPPIII
   defb @00000010            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00010101, @00100001
   defb @00110100, @01010001
   defb @01010101, @00000011
   defb @10010010, @00010101
   defb @00010101, @10110111
   defb @11000000, @00001001
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00110101, @00010110
   defb @00110101, @01101011
   defb @01100010, @00011011


        ;Monedas
        ;XXXXYYYY
   defb @01110000
   defb @01010010
   defb @01100010
   defb @01110010
   defb @00100110
   defb @00110110
   defb @10000110
   defb @10010110
   defb @00000000
   defb @00000000

;.Pantalla 9
        ;TTPPPIII
   defb @00000111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @01101101, @00110010
   defb @00111101, @10000010
   defb @00011101, @01000011
   defb @00101100, @10100011
   defb @00101100, @00000101
   defb @01011100, @01000101
   defb @00101101, @10000110
   defb @00101100, @10010111
   defb @11000000, @00001001
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000101, @00001011
   defb @00100001, @01000111
   defb @10000011, @00011011


        ;Monedas
        ;XXXXYYYY
   defb @01000100
   defb @01010100
   defb @01100100
   defb @01110100
   defb @01010111
   defb @01100111
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 10
        ;TTPPPIII
   defb @01001101            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11001100, @00000000
   defb @00011001, @00010011
   defb @00011001, @10100011
   defb @01001000, @01000100
   defb @00011101, @00110110
   defb @00011101, @10000110
   defb @00011101, @00010111
   defb @01000110, @01000111
   defb @00011101, @10100111
   defb @11001100, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00110000, @00100101
   defb @01010001, @00111000
   defb @10000010, @01000101


        ;Monedas
        ;XXXXYYYY
   defb @00010010
   defb @01000010
   defb @01010010
   defb @01100010
   defb @01110010
   defb @10100010
   defb @01000110
   defb @01010110
   defb @01100110
   defb @01110110

;.Pantalla 11
        ;TTPPPIII
   defb @01000111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00111111, @01110001
   defb @00100010, @10100001
   defb @00101110, @00100010
   defb @00110010, @01010100
   defb @00100010, @00010110
   defb @00011111, @01111000
   defb @00101110, @10101000
   defb @00101110, @00001001
   defb @00010011, @01001001
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000100, @00001011
   defb @00110000, @00000110
   defb @10000010, @00100110


        ;Monedas
        ;XXXXYYYY
   defb @01110000
   defb @10110000
   defb @00100001
   defb @00110001
   defb @01010011
   defb @01100011
   defb @00110100
   defb @00010101
   defb @00100101
   defb @01110111

;.Pantalla 12
        ;TTPPPIII
   defb @01001100            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11000100, @00000000
   defb @01010100, @00000011
   defb @00110100, @10000011
   defb @01011011, @10100100
   defb @00011001, @01100101
   defb @00101010, @00110111
   defb @00111010, @00101000
   defb @00011001, @10001000
   defb @01101010, @00001001
   defb @00101010, @10101001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100000, @00111001
   defb @01000010, @01111001
   defb @01100101, @01001001


        ;Monedas
        ;XXXXYYYY
   defb @00000001
   defb @01000001
   defb @10000001
   defb @00100010
   defb @10100010
   defb @01100100
   defb @10010101
   defb @01100111
   defb @10000111
   defb @10110111

;.Pantalla 13
        ;TTPPPIII
   defb @01000101            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @01110010, @00000000
   defb @00010001, @10000010
   defb @01011010, @00110100
   defb @00010001, @10100100
   defb @00010001, @00010110
   defb @00010001, @00110111
   defb @00111010, @00001001
   defb @01110010, @01011001
   defb @00000000, @00000000
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100010, @00010111
   defb @01100101, @00111010
   defb @10000000, @01111001


        ;Monedas
        ;XXXXYYYY
   defb @10000001
   defb @00110011
   defb @01000011
   defb @01100011
   defb @01110011
   defb @10100011
   defb @00010101
   defb @00110110
   defb @01111000
   defb @10001000

;.Pantalla 14
        ;TTPPPIII
   defb @01000010            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00011001, @00100010
   defb @01110010, @01010010
   defb @00011001, @00110011
   defb @00011001, @01000100
   defb @00011001, @01010101
   defb @00110010, @01100110
   defb @00100010, @10100111
   defb @01001110, @00001001
   defb @01101110, @01101001
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010101, @01011011
   defb @01010101, @01101000
   defb @10000100, @00110110


        ;Monedas
        ;XXXXYYYY
   defb @00010000
   defb @00000001
   defb @00100001
   defb @10110001
   defb @00110010
   defb @01000011
   defb @10110110
   defb @01000111
   defb @01010111
   defb @10011000

;.Pantalla 15
        ;TTPPPIII
   defb @01010110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @10100010, @00100001
   defb @10100010, @00010011
   defb @10100010, @00100101
   defb @10100010, @00010111
   defb @11000000, @00001001
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100010, @00001010
   defb @01000010, @00001011
   defb @01100010, @00001010


        ;Monedas
        ;XXXXYYYY
   defb @10100000
   defb @10110000
   defb @00010001
   defb @10110010
   defb @10110011
   defb @10110100
   defb @00010101
   defb @10110110
   defb @10110111
   defb @10111000

;.Pantalla 16
        ;TTPPPIII
   defb @01000100            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00110001, @01100011
   defb @00011001, @00110100
   defb @00011001, @10010100
   defb @00011001, @01000101
   defb @00011001, @10000101
   defb @00110000, @00010111
   defb @01001000, @01000111
   defb @00110000, @10000111
   defb @01011100, @00001001
   defb @01011100, @01111001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100010, @01111001
   defb @00110010, @00110101
   defb @01100011, @00011010


        ;Monedas
        ;XXXXYYYY
   defb @01100001
   defb @01100010
   defb @01000100
   defb @10000100
   defb @00010110
   defb @01000110
   defb @10000110
   defb @10100110
   defb @01011000
   defb @01101000

;.Pantalla 17
        ;TTPPPIII
   defb @01000110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00101100, @00000001
   defb @01001000, @10000010
   defb @00111000, @00110011
   defb @00011001, @00000100
   defb @10000100, @00100101
   defb @10110110, @00010111
   defb @11001110, @00001001
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010101, @10001011
   defb @00100000, @00110101
   defb @01000010, @00101000


        ;Monedas
        ;XXXXYYYY
   defb @00000000
   defb @00010000
   defb @10110001
   defb @00110010
   defb @00000011
   defb @10000100
   defb @10010100
   defb @10100101
   defb @00000110
   defb @10110110

;.Pantalla 18
        ;TTPPPIII
   defb @01001011            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00011001, @10010001
   defb @00011001, @10000010
   defb @00011001, @10100010
   defb @00011001, @01110011
   defb @00011001, @10110011
   defb @00011001, @01100100
   defb @00011001, @01010101
   defb @00011001, @01000110
   defb @00011001, @00110111
   defb @11000100, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000010, @01111011
   defb @00100010, @00000111
   defb @10000010, @00101011


        ;Monedas
        ;XXXXYYYY
   defb @10010000
   defb @10000001
   defb @10100001
   defb @00000010
   defb @01110010
   defb @10110010
   defb @01100011
   defb @01010100
   defb @01000101
   defb @00110110

;.Pantalla 19
        ;TTPPPIII
   defb @01000001            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @01000000, @00000001
   defb @00010011, @10110001
   defb @00011001, @01000011
   defb @01101010, @01010011
   defb @00011001, @00110100
   defb @00011001, @00010101
   defb @00011001, @00100110
   defb @00011001, @00110111
   defb @10100000, @00001001
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000011, @00001011
   defb @00100001, @01001010
   defb @10000011, @00110101


        ;Monedas
        ;XXXXYYYY
   defb @00000000
   defb @10110000
   defb @01000010
   defb @10100010
   defb @00110011
   defb @00010100
   defb @00100101
   defb @00110110
   defb @10101000
   defb @00000000

;.Pantalla 20
        ;TTPPPIII
   defb @01000101            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00011011, @00100010
   defb @00011011, @10010010
   defb @00101010, @01010011
   defb @00111010, @00000101
   defb @00111010, @10010101
   defb @01001110, @01000111
   defb @00111110, @00001001
   defb @00111110, @10011001
   defb @00000000, @00000000
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010001, @00001011
   defb @01100100, @00010100
   defb @01100010, @01111010


        ;Monedas
        ;XXXXYYYY
   defb @00100001
   defb @10010001
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 21
        ;TTPPPIII
   defb @10000010            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @01011101, @00100001
   defb @01101100, @00110001
   defb @00011101, @00000010
   defb @00011101, @10110010
   defb @00101100, @10000100
   defb @00101100, @00110101
   defb @00010001, @01110110
   defb @01001100, @01011000
   defb @00101110, @00001001
   defb @00101110, @10101001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000001, @00101000
   defb @00100010, @01101010
   defb @00000000, @00000000


        ;Monedas
        ;XXXXYYYY
   defb @01100000
   defb @00000001
   defb @01110010
   defb @00110100
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 22
        ;TTPPPIII
   defb @10000111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00000000, @10000001
   defb @00101110, @10000010
   defb @00111110, @01110011
   defb @01001110, @01100100
   defb @01011110, @01010101
   defb @01101110, @01000110
   defb @01111110, @00110111
   defb @10001110, @00101000
   defb @11001110, @00001001
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010001, @00000111
   defb @00110100, @00000101
   defb @01010010, @00000011


        ;Monedas
        ;XXXXYYYY
   defb @10000001
   defb @01100011
   defb @01000101
   defb @00100111
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 23
        ;TTPPPIII
   defb @10000101            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11000010, @00000000
   defb @00010001, @00000001
   defb @00010001, @10110001
   defb @10000000, @00100010
   defb @01000010, @00010100
   defb @01000010, @01110100
   defb @10000000, @00100110
   defb @00010001, @10111000
   defb @11000010, @00001001
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00110001, @00011010
   defb @01010100, @00011010
   defb @01110011, @00011010


        ;Monedas
        ;XXXXYYYY
   defb @00110001
   defb @10000001
   defb @00100011
   defb @01000011
   defb @01110011
   defb @10010011
   defb @00110101
   defb @10000101
   defb @00000111
   defb @10110111

;.Pantalla 24
        ;TTPPPIII
   defb @10010110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00011011, @00100001
   defb @00011011, @10010001
   defb @00011011, @01000010
   defb @00011011, @01110010
   defb @00101010, @01010100
   defb @00011011, @01000110
   defb @00011011, @01110110
   defb @00011011, @00100111
   defb @00011011, @10010111
   defb @11000010, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000011, @00001011
   defb @00110100, @00101001
   defb @01010001, @00001011


        ;Monedas
        ;XXXXYYYY
   defb @00100000
   defb @10010000
   defb @01000001
   defb @01110001
   defb @01010011
   defb @01100011
   defb @01000101
   defb @01110101
   defb @00100110
   defb @10010110

;.Pantalla 25
        ;TTPPPIII
   defb @10001111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11001110, @00000000
   defb @11000000, @00000001
   defb @10101010, @00010100
   defb @01010000, @00000110
   defb @01010000, @01010111
   defb @11000000, @00001001
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100101, @00001011
   defb @00110010, @00001011
   defb @01010001, @00001011


        ;Monedas
        ;XXXXYYYY
   defb @00010011
   defb @00110011
   defb @01010011
   defb @01100011
   defb @10000011
   defb @10100011
   defb @01101000
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 26
        ;TTPPPIII
   defb @10010111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @10000111, @00000000
   defb @00101100, @00010001
   defb @01101100, @01000001
   defb @10010111, @10110001
   defb @00101110, @00100011
   defb @00101110, @10010011
   defb @00101110, @00010101
   defb @00101110, @10000101
   defb @00101110, @00100111
   defb @10110010, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100001, @00111010
   defb @01000101, @00011000
   defb @01100011, @00111010


        ;Monedas
        ;XXXXYYYY
   defb @00110010
   defb @00010100
   defb @10010100
   defb @00110110
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 27
        ;TTPPPIII
   defb @10001101            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @10000100, @00010010
   defb @00100110, @01110100
   defb @00100110, @10100100
   defb @00010101, @10010110
   defb @00011001, @10000111
   defb @00011001, @10100111
   defb @00010101, @01101000
   defb @00010101, @01111000
   defb @00101000, @00001001
   defb @00101000, @10101001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010101, @00011010
   defb @01010100, @10111011
   defb @01100100, @01101000


        ;Monedas
        ;XXXXYYYY
   defb @00100001
   defb @01000001
   defb @01100001
   defb @10000001
   defb @01110011
   defb @10010011
   defb @10110011
   defb @10010101
   defb @01100111
   defb @10110111

;.Pantalla 28
        ;TTPPPIII
   defb @10001111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00011001, @10100101
   defb @00011001, @01010111
   defb @01001110, @01100111
   defb @00011001, @10100111
   defb @00011001, @00101000
   defb @00011001, @01001000
   defb @00011011, @00001001
   defb @00011011, @00011001
   defb @00011001, @00111001
   defb @00101010, @10101001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00110010, @10101011
   defb @01100000, @01011010
   defb @01110100, @00000010


        ;Monedas
        ;XXXXYYYY
   defb @10000100
   defb @10100100
   defb @01010110
   defb @01110110
   defb @10000110
   defb @10100110
   defb @00100111
   defb @01000111
   defb @00111000
   defb @10101000

;.Pantalla 29
        ;TTPPPIII
   defb @10000100            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00011001, @00010001
   defb @00011001, @10100010
   defb @00011001, @00000011
   defb @00011001, @10010100
   defb @00011001, @00010101
   defb @00011001, @10100110
   defb @00011001, @00000111
   defb @00110010, @00001001
   defb @00010011, @01011001
   defb @00110010, @10011001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000010, @00000010
   defb @00110010, @01111011
   defb @10000101, @00011011


        ;Monedas
        ;XXXXYYYY
   defb @00010000
   defb @10100001
   defb @00000010
   defb @10010011
   defb @00010100
   defb @10100101
   defb @00000110
   defb @01011000
   defb @10011000
   defb @10111000

;.Pantalla 30
        ;TTPPPIII
   defb @10000110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00011001, @00000100
   defb @00011001, @10110100
   defb @00011001, @01000101
   defb @00011001, @00100111
   defb @00011101, @01000111
   defb @00011001, @01100111
   defb @00101000, @00001001
   defb @00011101, @00111001
   defb @00011101, @01011001
   defb @00011001, @01111001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00110100, @00000100
   defb @01010000, @00100011
   defb @01010000, @01010111


        ;Monedas
        ;XXXXYYYY
   defb @00000011
   defb @10110011
   defb @01000100
   defb @00100110
   defb @01000110
   defb @01100110
   defb @00011000
   defb @00111000
   defb @01011000
   defb @01111000

;.Pantalla 31
        ;TTPPPIII
   defb @10000111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11001110, @00000000
   defb @00101011, @01100100
   defb @00011011, @01101000
   defb @00010011, @01111000
   defb @00010011, @10001000
   defb @00101110, @10101000
   defb @00101000, @00001001
   defb @00011111, @01001001
   defb @00011111, @01011001
   defb @00011011, @01101001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @01110010, @00000100
   defb @01110000, @01111010
   defb @00000000, @00000000


        ;Monedas
        ;XXXXYYYY
   defb @10000101
   defb @00010111
   defb @01010111
   defb @01100111
   defb @01110111
   defb @10100111
   defb @00101000
   defb @01001000
   defb @01011000
   defb @00000000

;.Pantalla 32
        ;TTPPPIII
   defb @10000110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00110000, @01000001
   defb @01110001, @10000001
   defb @01100001, @10100001
   defb @01010001, @00010010
   defb @00010001, @01100010
   defb @00010001, @00100011
   defb @00100000, @00100101
   defb @01010000, @00000111
   defb @00100000, @10100111
   defb @11001110, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000010, @00000101
   defb @00000000, @01101011
   defb @01100100, @01000111


        ;Monedas
        ;XXXXYYYY
   defb @10010011
   defb @10010101
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 33
        ;TTPPPIII
   defb @10001110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @10000010, @00100010
   defb @01110011, @10110010
   defb @00100010, @00010011
   defb @01111010, @01000100
   defb @00100010, @00010101
   defb @01110010, @00100110
   defb @00100010, @00010111
   defb @00010010, @10010111
   defb @01011110, @00001001
   defb @01000010, @10001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010101, @01101011
   defb @01000010, @00000011
   defb @01110100, @00110111


        ;Monedas
        ;XXXXYYYY
   defb @01110011
   defb @10010011
   defb @01010101
   defb @01110101
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 34
        ;TTPPPIII
   defb @10000111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00100100, @10010001
   defb @00111110, @00100011
   defb @00110000, @10010011
   defb @00010001, @01110100
   defb @01000000, @01010110
   defb @00010101, @00010111
   defb @01101110, @00001001
   defb @00110000, @10001001
   defb @00000000, @00000000
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010010, @00101000
   defb @01010011, @00001011
   defb @10000010, @00010101


        ;Monedas
        ;XXXXYYYY
   defb @10100000
   defb @01110011
   defb @00010101
   defb @01110101
   defb @01110111
   defb @10111000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 35
        ;TTPPPIII
   defb @10000110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @10100110, @00000000
   defb @00100110, @10010001
   defb @00111110, @00110011
   defb @00111010, @10010011
   defb @00111010, @00100100
   defb @01001010, @00010110
   defb @00101110, @10000110
   defb @00101010, @10010111
   defb @01101110, @00001001
   defb @00111010, @10011001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010100, @00000100
   defb @01110100, @00000100
   defb @10000010, @00011000


        ;Monedas
        ;XXXXYYYY
   defb @10100000
   defb @00010101
   defb @10111000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 36
        ;TTPPPIII
   defb @11000011            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11001110, @00000000
   defb @00100000, @01010010
   defb @01000000, @00010100
   defb @01000000, @01110100
   defb @00011101, @00000101
   defb @00011101, @00000111
   defb @01000000, @01000111
   defb @00011101, @10110111
   defb @11001110, @00001001
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00010000, @00001011
   defb @00110011, @00010111
   defb @10000101, @00101000


        ;Monedas
        ;XXXXYYYY
   defb @01010001
   defb @01100001
   defb @00110011
   defb @10000011
   defb @00000110
   defb @01010110
   defb @01100110
   defb @10110110
   defb @01001000
   defb @01111000

;.Pantalla 37
        ;TTPPPIII
   defb @11000101            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @01101100, @00110001
   defb @00011101, @00100010
   defb @00011101, @10010010
   defb @00111101, @00010011
   defb @00010101, @01010100
   defb @00011101, @00100110
   defb @00011101, @10010110
   defb @01101100, @00110111
   defb @00011101, @10111000
   defb @11000100, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00110000, @01010111
   defb @01010000, @01101001
   defb @01100000, @00111000


        ;Monedas
        ;XXXXYYYY
   defb @00110010
   defb @00100011
   defb @00110011
   defb @10010011
   defb @00000100
   defb @00010110
   defb @10100110
   defb @00100111
   defb @10010111
   defb @00000000

;.Pantalla 38
        ;TTPPPIII
   defb @11010110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00011101, @00000000
   defb @10111100, @00010000
   defb @00111010, @00000100
   defb @00101100, @01010100
   defb @00111010, @10010100
   defb @00011101, @01000101
   defb @00011101, @01110101
   defb @00011101, @00110111
   defb @00011101, @10000111
   defb @11001100, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00110011, @00011010
   defb @01100001, @00111000
   defb @00000000, @00000000


        ;Monedas
        ;XXXXYYYY
   defb @00100001
   defb @01000001
   defb @01110001
   defb @10010001
   defb @10110011
   defb @01010101
   defb @01100101
   defb @01000110
   defb @01110110
   defb @00000000

;.Pantalla 39
        ;TTPPPIII
   defb @11001100            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11000010, @00000000
   defb @01011000, @00000100
   defb @01011000, @01110100
   defb @00011001, @01100110
   defb @00011001, @01001000
   defb @00010011, @01011000
   defb @00011001, @01101000
   defb @00011001, @01111000
   defb @11001000, @00001001
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00110100, @00011010
   defb @01100001, @00010101
   defb @01110010, @00001011


        ;Monedas
        ;XXXXYYYY
   defb @00010001
   defb @00100001
   defb @01010001
   defb @10100001
   defb @00010101
   defb @00100101
   defb @00110101
   defb @10000101
   defb @10010101
   defb @10100101

;.Pantalla 40
        ;TTPPPIII
   defb @11001111            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @11000000, @00000000
   defb @00100001, @00000100
   defb @00100001, @01000100
   defb @00110001, @10110100
   defb @01000000, @00000110
   defb @00010000, @10010110
   defb @01010100, @01110111
   defb @11000000, @00001001
   defb @00000000, @00000000
   defb @00000000, @00000000


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00100001, @00011010
   defb @01010010, @01011000
   defb @00000000, @00000000


        ;Monedas
        ;XXXXYYYY
   defb @00010001
   defb @01010001
   defb @01100001
   defb @10100001
   defb @00010101
   defb @00100101
   defb @00110101
   defb @10100101
   defb @00000000
   defb @00000000

;.Pantalla 41
        ;TTPPPIII
   defb @11001110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00010011, @00010100
   defb @00010001, @01100100
   defb @00010001, @01010101
   defb @00010001, @01110101
   defb @00010001, @10100110
   defb @00010001, @01000111
   defb @00010001, @10010111
   defb @00011101, @00111000
   defb @00010001, @01101000
   defb @00101110, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00110001, @00001011
   defb @01100101, @00001001
   defb @01110010, @01011000


        ;Monedas
        ;XXXXYYYY
   defb @10000010
   defb @00010011
   defb @01010100
   defb @01110100
   defb @10110100
   defb @00110101
   defb @10010101
   defb @00000000
   defb @00000000
   defb @00000000

;.Pantalla 42
        ;TTPPPIII
   defb @11001101            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00010111, @00110001
   defb @00010111, @10000001
   defb @00101100, @00000011
   defb @00010111, @00010011
   defb @00010111, @10100011
   defb @00010111, @00110101
   defb @00010111, @10000101
   defb @00010111, @00000111
   defb @00100111, @10110111
   defb @00111100, @00001001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000001, @00001011
   defb @01000101, @00001011
   defb @01100001, @00001011


        ;Monedas
        ;XXXXYYYY
   defb @00110000
   defb @10000000
   defb @00000010
   defb @01100010
   defb @10110010
   defb @00110100
   defb @10000100
   defb @00000110
   defb @10110110
   defb @00000000

;.Pantalla 43
        ;TTPPPIII
   defb @11010110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00110010, @10010001
   defb @00011101, @01000010
   defb @00011101, @01110010
   defb @00011101, @00000011
   defb @00011101, @10110011
   defb @00011101, @00010101
   defb @00011101, @10100101
   defb @00100000, @01010111
   defb @00100010, @00001001
   defb @00110010, @10011001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00110011, @00011010
   defb @01010011, @00101001
   defb @10000011, @00101100


        ;Monedas
        ;XXXXYYYY
   defb @00000000
   defb @00100000
   defb @10010000
   defb @10110000
   defb @00110011
   defb @10000011
   defb @00110110
   defb @10000110
   defb @00011000
   defb @10111000

;.Pantalla 44
        ;TTPPPIII
   defb @11000110            ; Descriptor

        ;Objetos
        ;datos "al reves", por eso del "endian"
        ;LLLLTTTD   XXXXYYYY
   defb @00010111, @00100001
   defb @01010111, @01000001
   defb @01010111, @01100001
   defb @00010111, @10000001
   defb @00010111, @10110011
   defb @00010111, @10010101
   defb @00010111, @10000111
   defb @00010111, @01011000
   defb @00100110, @00001001
   defb @00110110, @10011001


        ;Moviles
        ;YYYYTTTD   AAAABBBB
   defb @00000010, @00011010
   defb @00100000, @01111011
   defb @10000101, @00100100


        ;Monedas
        ;XXXXYYYY
   defb @01010001
   defb @01010010
   defb @01110010
   defb @10110010
   defb @01010011
   defb @10010011
   defb @01010100
   defb @01010101
   defb @01010110
   defb @10000110


#endasm
   