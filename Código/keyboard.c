/*-------------------------------------------*\
 |  keyboard.C                               |
 |  Autor: Pedro Mª Jiménez                  |
 |                                           |
 |  (c) Microsystems Engineering (Bilbao)    |
\*-------------------------------------------*/
//Modificado por Emilio Martínez.

// Funciones para la gestión del teclado de la Trainer PLUS.

// char kbd_getc()	Devuelve el código ASCII de la tecla pulsada.
//			Si no se pulsó ninguna, devuelve 0.


// Conexiones del teclado.
/*
            RD3 RD2 RD1 RD0
             ^   ^   ^   ^
             |   |   |   |
           |---|---|---|---|
  RD4 ---> | 1 | 2 | 3 | O |
           |---|---|---|---|
  RD5 ---> | 4 | 5 | 6 | C |
           |---|---|---|---|
  RD6 ---> | 7 | 8 | 9 | U |
           |---|---|---|---|
  RD7 ---> | L | 0 | R | D |
           |---|---|---|---|
*/
//PIC DE GAMA ALTA en el puerto d
#byte kbd_port_d = 0xf83

// Caracteres ASCII asociados a cada tecla:
char const KEYS[4][4] = {{'O','3','2','1'},
                         {'C','6','5','4'},
                         {'U','9','8','7'},
                         {'D','R','0','L'}};





char kbd_getc()
{
  char tecla=0;
  int f,c,t,i,j;

  //port_d_pullups(true);
  set_tris_d(0b00001111); // RD7-RD4 salidas, RD3-RD0 entradas

  for(f=0x10, i=0; i<4; f<<=1, i++)
  {
    for(c=0x01, j=0; j<4; c<<=1, j++)
    {
      kbd_port_d = ~f;
      delay_cycles(1);
      t = kbd_port_d & 0x0F;
      t = ~(t | 0xF0);
      if(t == c)
      {
        delay_ms(20);
        tecla=KEYS[i][j];
        while(t==c)
        {
          restart_wdt();
          t = kbd_port_d & 0x0F;
          t = ~(t | 0xF0);
        }
        break;
      }
    }
    if(tecla)
      break;
  }
  restart_wdt();

  //port_b_pullups(false);
  return tecla;
}
