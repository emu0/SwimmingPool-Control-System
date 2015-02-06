//Funciones auxiliares para el Sistema Electrónico Digital de una Piscina Cubierta
//Por Emilio Martínez.

/*Declaración de variables*/
#include <stdio.h>
#define LCD_CLEAR 0x01
#define LCD_HOME 0x02
#define LCD_DISPLAY_OFF  0x08
#define LCD_DISPLAY_ON   0x0C
#define LCD_CURSOR_ON    0x0E
#define LCD_CURSOR_BLINK 0x0F
short equal=FALSE, estadocorrecto=FALSE;
char *dir;
//Creamos una tabla con la forma que le damos a la memoria EEPROM y saber en cada moemnto donde tenemos que leer o escribir.
char tabla_EEPROM[10]={79,75,48,48,48,48,50,51,54,55}; //{O,K,0,0,0,0,2,3}//{(cabecera1, cabecera2, clave [x4], temp [x2], )}
char ascii[10]={48,49,50,51,52,53,54,55,56,57};
char const mes[12][11]={{"Enero"}, {"Febrero"}, {"Marzo"}, {"Abril"}, {"Mayo"}, {"Junio"}, {"Julio"}, {"Agosto"}, {"Septiembre"}, {"Octubre"}, {"Noviembre"}, {"Diciembre"}};
char const semana[7][10]={{"Domingo"},{"Lunes"},{"Martes"},{"Miercoles"},{"Jueves"},{"Viernes"},{"Sabado"}};
char const tabla_menu[6][14]={{" CAMB CONTRA "},{" GEST PISCIN "},{"  CAMB RELOJ "},{" CICLO DEPUR "},{" MODO AHORRO "},{"    SALIR   "}};
char const tabla_1[6][14]={{" TEMPERATURA "},{"    CLORO    "},{"   TURBIDEZ  "},{" NIVEL AGUA "},{"     LUZ     "},{"  PASO ATRAS "}};
char const tabla_2[2][14]={{" CAMBIA HORA "},{"  PASO ATRAS "}};
char const tabla_3[5][14]={{" DEPURAR YA  "},{" PARAR DEPUR "},{" MODIF CICLO "},{" MODO TRATAM "},{"  PASO ATRAS "}};
char const tabla_4[3][14]={{" TIEM ESPERA "},{"  PASO ATRAS "}};
int  i, TempUs, CloroUs, TurbidezUs, NivelUs, TiempoUs;
char weekday[10];
char tecla=NULL, tecla2;
int ind_menu=1;
date_time_t dt;   //El tipo de estructura date_time_t está definido en PCF8583.c
date_alarm_t al;  // Definido en PCF8583.c

/*Listado de funciones*/
void iniciosistema();
void comprobarMemoria();
void establecerOK();
void iniciarRTC();
void modificarHoraRTC();
void iniciarAlarmaRTC();
void modificarAlarmaRTC();
void configurarVariables();
short compararMemorias(char direccion);
void establecercontras();
void establecerTemperatura();
void establecerCloro();
void establecerTurbidez();
void establecerNivel();
void establecerTiempo();
void establecerTipo();
void menu();
char teclado();
long int conversionCloro2Int(float valor);
float conversionInt2Cloro(long int valor);
long int conversionTurbidez2Int(float valor);
float conversionint2Turbidez(long int valor);
long int conversionNivelAgua2Int(float valor);
float conversionint2NivelAgua(long int valor);
long int conversionLuz2Int(float valor);
float conversionint2NivelLuz(long int valor);
float medirTemperatura();
float medirNivelCloro();
float medirNivelTurbidez();
float medirNivelAgua();
float medirNivelLuz();
void nivelesUS();
void RS232();


/* Funciones auxiliares a usar por el sistema */
void iniciosistema(){
      lcd_init();
      lcd_send_byte(0, LCD_CLEAR); //Borra LCD
      lcd_gotoxy(2 ,1);
      printf(lcd_putc, "INICIALIZANDO.");
      printf("Inicializando el sistema");
      lcd_gotoxy(1,2);
      for (i=0; i<8; i++){
         delay_ms(30);
        printf(lcd_putc, "..");
        printf("..");
      }
      delay_ms(400);
      lcd_send_byte(0, LCD_CLEAR);
      lcd_send_byte(0, LCD_CURSOR_ON);
}

void comprobarMemoria(){

   short diferencia=FALSE;
   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "Comprobando \nmemoria");
   printf("\rComprobando la memoria...");
   //Ver asignación de direcciones de memoria en manual.
   //Comprobamos si es la primera vez que se arranca el sistema.
   //Si las dos primeras direcciones no contiene 'O', 'K' se considera que es la primera vez que se arranca el sistema

   for (dir=0; dir<2; dir++){
      *dir=read_eeprom(dir);
      if (dir==0){ //para la dirección 0 debe haber un valor 'O'
         if(*dir !='O') break;
         //Si en la memoria interna está el valor deseado pasamos a comprobar que en la externa también lo tenga
         equal=compararmemorias(dir);
         if(!equal) break;
      }
      if (dir==1){ //para la dirección 1 debe haber un valor 'K'.
         if(*dir=='K') {
         equal=compararmemorias(dir);
         if(!equal) break;
            //Si las dos primeras direcciones de ambas memorias satisfacen las condiciones pasamos a la siguiente comprobación
            estadocorrecto=TRUE;
         }
      }
   }

   //Si estado vale 1, es porque se ha comprobado que ambas eeprom ya han sido escritas por el sistema
   //Comparamos si los valores de ambas eeprom coinciden, si no lo hicieran se pueden introducir
   //de nuevo las variables, o se le da prioridad a la eeprom externa.
   if(estadocorrecto){
      for(dir=0; dir<50; dir++){
         equal=compararMemorias(dir);
         if(!equal) diferencia=TRUE;
      }
      if(diferencia){
         for(dir=0; dir<50; dir++){
            *dir=read_ext_eeprom(dir);
            delay_ms(15);
            write_eeprom(dir, *dir);
         }
      }
      else return;
   }
   else configurarVariables();
}

void establecerOK(){
   write_eeprom(0, 'O');
   delay_ms(15);
   write_ext_eeprom(0, 'O');
   delay_ms(15);
   write_eeprom(1, 'K');
   delay_ms(15);
   write_ext_eeprom(1, 'K');
   delay_ms(15);
}

void establecercontras(){
   char intento[4];
   char intento2[4];

   PSW:
   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "ESTABLECER CLAVE:");
   lcd_gotoxy(1,2);
   for(dir=0; dir<4; dir++){
      do{
         tecla=teclado();
         //Las teclas pulsadas deben ser números o cancelar.
      }while(tecla=='L' || tecla=='R' || tecla=='O' || tecla=='U' || tecla=='D');
      //En el caso en que se cancelara se volvería a pedir que se introdujera una contraseña
      if (tecla=='C') goto PSW;
      //Se crea un vector con la contraseña
      intento[dir]=tecla;
      printf(lcd_putc, "*");
   }
   lcd_gotoxy(7,2);
   printf(lcd_putc, "Aceptar?");

   do{
      tecla=teclado();
   }while(!(tecla=='O'|| tecla=='C'));
   if (tecla=='C') goto PSW;

   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "REPITA:          ");
   lcd_gotoxy(1,2);

   PSW2:
   for(dir=0; dir<4; dir++){
      do{
         tecla=teclado();
      }while(tecla=='L' || tecla=='R' || tecla=='O' || tecla=='D' || tecla=='U');
      if (tecla=='C') goto PSW2;
      intento2[dir]=tecla;
      printf(lcd_putc, "*");
   }
   lcd_gotoxy(7,2);
   printf(lcd_putc, "Aceptar?");

   do{
      tecla=teclado();
   }while(!(tecla=='O'|| tecla=='C'));
   if (tecla=='C') goto PSW2;

   i=0;
   for(dir=0; dir<4; dir++){
      if(intento2[dir]==intento[dir]) i++;
   }

   lcd_gotoxy(1,2);

   if (i!=4) {
      printf(lcd_putc, "  NO COINCIDEN  ");
      delay_ms(800);
   goto PSW;
   }

   for(dir=0; dir<4; dir++){
      write_eeprom(dir+2, intento[dir]);
      delay_ms(10);
      write_ext_eeprom(dir+2, intento[dir]);
      delay_ms(10);
   }

   lcd_send_byte(0, LCD_CLEAR);
   lcd_gotoxy(1,2);
   printf(lcd_putc, " CLAVE GUARDADA");
   delay_ms(1000);
}

void establecerTemperatura(){
   long int temp=20;

   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "TEMPERATURA AGUA");
   lcd_gotoxy(8,2);
   printf(lcd_putc, "%cC", 223);
    do{
      lcd_gotoxy(5,2);
      //Mostromos por pantalla el valor la cadena
      printf(lcd_putc, "%2ld", temp);
      tecla=teclado();

      //Establecemos los límites
      if (tecla == 'R') temp++;
      if (tecla == 'L') temp--;
      if (temp<15) temp=40;
      if (temp>40) temp=15;
      //Mientras que no se pulse ok...
   }while(tecla != 'O');

   //Si se acepta ese valor guarda en la eeprom
   write_eeprom(6, temp);
   delay_ms(15);
   write_ext_eeprom(6, temp);
   delay_ms(15);
}

void establecerCloro(){
   float cloro=1;
   long int cloro2;

   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "CONCENT. CLORO: ");
   lcd_gotoxy(9,2);
   printf(lcd_putc, "mgr/l");
    do{
      lcd_gotoxy(5,2);
      //Mostromos por pantalla el valor la cadena
      printf(lcd_putc, "%1.1f", cloro);
      tecla=teclado();

      //Establecemos los límites
      if (tecla == 'R') cloro += 0.1;
      if (tecla == 'L') cloro -= 0.1;
      if (cloro<0.1) cloro=1.5;
      if (cloro>1.6) cloro=0.1;
      //Mientras que no se pulse ok...
   }while(tecla != 'O');

   cloro2 =conversionCloro2Int(cloro);
   //Si se acepta ese valor guarda en la eeprom
   write_eeprom(7, cloro2);
   delay_ms(15);
   write_ext_eeprom(7, cloro2);
   delay_ms(15);
}

void establecerTurbidez(){
   float turbidez=4;
   long int turbidez2;

   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "TURBIDEZ DE AGUA:");
   lcd_gotoxy(9,2);
   printf(lcd_putc, "Kpart/l");
    do{
      lcd_gotoxy(5,2);
      //Mostromos por pantalla el valor la cadena
      printf(lcd_putc, "%1.1f", turbidez);
      tecla=teclado();

      //Establecemos los límites
      if (tecla == 'R') turbidez+=0.2;
      if (tecla == 'L') turbidez-=0.2;
      if (turbidez<0.2) turbidez=4;
      if (turbidez>4) turbidez=0.2;
      //Mientras que no se pulse ok...
   }while(tecla != 'O');

   turbidez2 =conversionTurbidez2Int(turbidez);
   //Si se acepta ese valor guarda en la eeprom
   write_eeprom(8, turbidez2);
   delay_ms(15);
   write_ext_eeprom(8, turbidez2);
   delay_ms(15);
}

void establecerNivel(){

   float agua=0;
   long int agua2;

   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "NIVEL AGUA:");
   lcd_gotoxy(9,2);
   printf(lcd_putc, "cm neg");
    do{
      lcd_gotoxy(5,2);
      //Mostromos por pantalla el valor la cadena
      printf(lcd_putc, "%1.1f", agua);
      tecla=teclado();

      //Establecemos los límites
      if (tecla == 'R') agua+=5;
      if (tecla == 'L') agua-=5;
      if (agua<0) agua=30;
      if (agua>30) agua=0;
      //Mientras que no se pulse ok...
   }while(tecla != 'O');

   agua2 =conversionNivelAgua2Int(agua);
   //Si se acepta ese valor guarda en la eeprom
   write_eeprom(9, agua2);
   delay_ms(15);
   write_ext_eeprom(9, agua2);
   delay_ms(15);
}

void establecerTiempo(){
   int aux;
   long int tiempo=90;
   char intento[3];

   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "TIEMPO DE ESPERA:");
   lcd_gotoxy(7,2);
   printf(lcd_putc, "segundos");

   do{
      lcd_gotoxy(2,2);
      //Mostromos por pantalla el valor la cadena
      printf(lcd_putc, "%3ld", tiempo);
      tecla=teclado();

      //Establecemos los límites
      if (tecla == 'R') tiempo+=30;
      if (tecla == 'L') {
         if(tiempo==30) tiempo=450;
         else tiempo-=30;
      }
      if (tiempo>450) tiempo=30;
      //Mientras que no se pulse ok...
   }while(tecla != 'O');

   write_eeprom(10, tiempo);
   delay_ms(15);
   write_ext_eeprom(10, tiempo);
   delay_ms(15);
}

void establecerTipo(){
   char const tipo[3][8]={{"Intenso"},{"Medio  "},{"Bajo   "}};
   int aux=0;
   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "TIPO DEPURADO:");
   lcd_gotoxy(1,2);
   do{
      lcd_gotoxy(5,2);
      for (i=0; i<8; i++){
         //Mostramos por pantalla el mensaje entero
         printf(lcd_putc, "%c", tipo[aux][i]);
      }
      tecla=teclado();
      //Establecemos los límites
      if (tecla == 'R') aux++;
      if (tecla == 'L') {
         if(aux==0) aux=2;
         else aux--;
      }
      if (aux>2) aux=0;
      //Mientras que no se pulse ok...
   }while(tecla != 'O');

   write_eeprom(13, aux);
   delay_ms(15);
   write_ext_eeprom(13, aux);
   delay_ms(15);
}

void configurarVariables(){
   //Preguntamos los valores de las diferentes variables a tratar.
   establecerOK();
   establecerContras();
   establecerTemperatura();
   establecerCloro();
   establecerTurbidez();
   establecerNivel();
   establecerTiempo();
   modificarAlarmaRTC();
   establecerTipo();
}

//Compara el valor guardado en la eepron interna y externa de direccion de memoria pasada.
short compararMemorias(char direccion){
   char interna, externa;
   interna = read_eeprom(direccion);
   delay_ms(10);
   externa = read_ext_eeprom(direccion);
   delay_ms(10);
   //lcd_send_byte(0, LCD_CLEAR);
   //printf(lcd_putc, "%d  %d - %d", direccion, interna, externa);
   //delay_ms(1000);
   if(externa==interna) return TRUE;
   else return FALSE;
}

                            //HAY QUE REALIZAR BIEN ESTAS 4 FUNCIONES
long int conversionCloro2Int(float valor){
   float cloro;
   long int cloro2;
   cloro=((valor*255)/1.5);
   cloro2=(long int)cloro;
   return cloro2;
}

float conversionint2Cloro(long int valor){
   float cloro;
   cloro=(valor*1.5)/255;
   return cloro;
}

long int conversionTurbidez2Int(float valor){
   float turbidez;
   long int turbidez2;
   turbidez=((valor*255)/4);
   turbidez2=(long int)turbidez;
   return turbidez2;
}

float conversionint2Turbidez(long int valor){
   float turbidez;
   turbidez=(valor*4)/255;
   return turbidez;
}

long int conversionNivelAgua2Int(float valor){
   float agua;
   long int agua2;
   agua=((valor*255)/20);
   agua2=(long int)agua;
   return agua2;
}

float conversionint2NivelAgua(long int valor){
   float agua;
   agua=(valor*20)/255;
   return agua;
}

long int conversionNivelLuz2Int(float valor){
   float luz;
   long int luz2;
   luz=((valor*255)/450);
   luz2=(long int)luz;
   return luz2;
}

float conversionint2NivelLuz(long int valor){
   float luz;
   luz=(valor*450)/255;
   return luz;
}

void iniciarRTC(){
   PCF8583_init_al(); //Inicialización del RTC con función de alarma

   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "Hora automatica?");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "OK/CANCEL");
   do{
   tecla=teclado();
      if (tecla=='O'){
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "Hora ajustada.");
         delay_ms(800);
         lcd_send_byte(0, LCD_CLEAR);
         return;
      }
      if (tecla=='C'){
         modificarHoraRTC();
         return;
      }
    }while(1);
}

void modificarHoraRTC(){

   int d=1, m=0, w=0, h=1, min=1;
   int dlimit;
   int limite[3]={31, 30, 28};
   unsigned int a=10;

   //SELECCIÓN DE MES
   lcd_send_byte(0, LCD_CLEAR);
   do{
      printf(lcd_putc, "MES: ");
      for (i=0; i<12; i++){
         //Mostrmos por pantalla el mes entero
         printf(lcd_putc, "%c", mes[m][i]);
      }
      tecla=teclado();
      lcd_send_byte(0, LCD_CLEAR);
      //Establecemos los límites
      if (tecla == 'R') m++;
      if (tecla == 'L') {
         if(m==0) m=11;
         else m--;
      }
      if (m>11) m=0;
      //Mientras que no se pulse ok...
   }while(tecla != 'O');
   dt.month=(m+1);

   //SELECCIÓN DE DÍA
   lcd_send_byte(0, LCD_CLEAR);
   do{
      //identificamos los días que tiene el mes seleccionado
      if (m==3 || m==5 || m==8 || m==10) dlimit = limite[1];
      else if (m==1) dlimit = limite[2];
      else dlimit = limite[0];
      //Establecemos el DÍA
      printf(lcd_putc, "DIA: ");
      printf(lcd_putc, "%d", d);
      tecla=teclado();
      lcd_send_byte(0, LCD_CLEAR);
      if (tecla == 'R') d++;
      if (tecla == 'L') d--;
      //según que mes, habrá un límite diferente
      if (d>dlimit) d=1;
      if (d<1) d=dlimit;
      //Hasta que no se pulsa Ok...
   }while(tecla != 'O');
   dt.day = d;

   //SELECCIÓN DE DIA DE LA SEMANA
   lcd_send_byte(0, LCD_CLEAR);
   do{
      printf(lcd_putc, "DIA SEMANA: ");
      lcd_gotoxy(1,2);
      for (i=0; i<10; i++){
         //Mostrmos por pantalla el mes entero
         printf(lcd_putc, "%c", semana[w][i]);
      }
      tecla=teclado();
      lcd_send_byte(0, LCD_CLEAR);
      //Establecemos los límites
      if (tecla == 'R') w++;
      if (tecla == 'L'){
         if(w==0) w=6;
         else w--;
      }
      if (w>6) w=1;
      //Mientras que no se pulse ok.
   }while(tecla != 'O');
   dt.weekday = w;

   //SELECCIÓN DE AÑO
   lcd_send_byte(0, LCD_CLEAR);
   do{
      printf(lcd_putc, "A%cO: ", 238);
      printf(lcd_putc, "%d", a);
      tecla=teclado();
      lcd_send_byte(0, LCD_CLEAR);
      if (tecla == 'R') a++;
      if (tecla == 'L') a--;
      if (a>50) a=10;
      if (a<10) a=50;
      //hasta que no se pulse OK...
   }while(tecla != 'O');
   dt.year=a;

   //SELECCIÓN DE HORA
   lcd_send_byte(0, LCD_CLEAR);
   do{
      //Establecemos la hora
      printf(lcd_putc, "HORA: ");
      printf(lcd_putc, "%d", h);
      tecla=teclado();
      lcd_send_byte(0, LCD_CLEAR);
      if (tecla == 'R') h++;
      if (tecla == 'L'){
         if(h==0) h=23;
         else h--;
      }
      //formato 24 horas
      if (h>23) h=0;
      //Hasta que no se pulse OK...
   }while(tecla != 'O');
   dt.hours=h;

   //SELECCIÓN DE MINUTO
   lcd_send_byte(0, LCD_CLEAR);
   do{
      //Establecemos los minutos
      printf(lcd_putc, "MINUTO: ");
      printf(lcd_putc, "%d", min);
      tecla=teclado();
      lcd_send_byte(0, LCD_CLEAR);
      if (tecla == 'R') min++;
      if (tecla == 'L') min--;
      if (min>59) min=1;
      if (min<1) min=59;
      //Hasta que no se pulse OK...
   }while(tecla != 'O');
   dt.minutes=min;

   dt.seconds = 00;    // Segundos por defecto
   PCF8583_set_datetime(&dt); //Puesta en fecha-hora
   return;
}

void iniciarAlarmaRTC(){
   al.hours=read_eeprom(11);
   delay_ms(15);
   al.minutes=read_eeprom(12);
   delay_ms(15);
   al.seconds =00;    // Segundos por defecto
   PCF8583_set_datealarm(&al); //Puesta en hora
}

void modificarAlarmaRTC(){

   int h=1, min=1;
   //SELECCIÓN DE HORA
   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "HORA COMIENZO \nDEPURADO:");
   do{
      //Establecemos la hora

      lcd_gotoxy(11,2);
      printf(lcd_putc, "%2d", h);
      tecla=teclado();
      if (tecla == 'R') h++;
      if (tecla == 'L'){
         if(h==0) h=23;
         else h--;
      }
      //formato 24 horas
      if (h>23) h=0;
      //Hasta que no se pulse OK...
   }while(tecla != 'O');

   write_eeprom(11, h);
   delay_ms(15);
   write_ext_eeprom(11, h);
   delay_ms(15);

   //SELECCIÓN DE MINUTO
   lcd_send_byte(0, LCD_CLEAR);
   printf(lcd_putc, "MINUTO COMIENZO \nDEPURADO:");
   do{
      //Establecemos los minutos
      lcd_gotoxy(11,2);
      printf(lcd_putc, "%d", min);
      tecla=teclado();
      if (tecla == 'R') min++;
      if (tecla == 'L') min--;
      if (min>59) min=1;
      if (min<1) min=59;
      //Hasta que no se pulse OK...
   }while(tecla != 'O');

   write_eeprom(12, min);
   delay_ms(15);
   write_ext_eeprom(12, min);
   delay_ms(15);

   return;
}

char teclado(){
   char tecla_teclado;
   //Iniciamos con un valor nulo
   tecla_teclado = NULL;
   do{
      tecla_teclado=kbd_getc();
      //Se muestrea el teclado mientras ninguna tecla sea pulsada
   }while (tecla_teclado==NULL);
      return tecla_teclado;
}

float medirTemperatura(){
   //Medimos la temperatura (ver ds1820.c)
   return ds1820_read ();
}

float medirNivelCloro(){
   set_adc_channel(3); //Selección del canal 3 ( pin RA3 )
   delay_us(10);
   return conversionInt2Cloro(read_adc());
}

float medirNivelTurbidez(){
   set_adc_channel(2); //Selección del canal 2 ( pin RA2 )
   delay_us(10);
   return conversionInt2Turbidez(read_adc());
}

float medirNivelAgua(){
   set_adc_channel(1); //Selección del canal 2 ( pin RA1 )
   delay_us(10);
   return conversionInt2NivelAgua(read_adc());
}

float medirNivelLuz(){
   set_adc_channel(0);//Selección del canal 2 ( pin RA0 )
   delay_us(10);
   return conversionInt2NivelLuz(read_adc());

}

void menu(){

   char clave[4];
   char intento[4];
   clear_interrupt(int_ext);

   LOOP_INGRESO:
   lcd_send_byte (0, LCD_CLEAR);
   printf (lcd_putc, "CLAVE: ");
   //Pedimos la clave de usuario y la guardamos en un vector
   for(dir=0; dir<4; dir++){
      do{
         tecla=teclado();
         //Las teclas pulsadas deben ser números o cancelar.
      }while(tecla=='L' || tecla=='R' || tecla=='O' || tecla=='U' || tecla=='D');
      if (tecla=='C') return;
      intento[dir]=tecla;
      printf(lcd_putc, "* ");
   }

   lcd_gotoxy(1,2);
   printf(lcd_putc, "Aceptar?");

   do{
      tecla=teclado();
   }while(!(tecla=='O'|| tecla=='C'));
   if (tecla=='C') goto LOOP_INGRESO;

   for(dir=0; dir<4; dir++){
      clave[dir]=read_eeprom((dir+2));
      delay_ms(15);
   }

   i=0;
   for(dir=0; dir<4; dir++){
      if(clave[dir]==intento[dir]) i++;
   }
   lcd_gotoxy(1,2);

   if (i!=4) {
      printf(lcd_putc, "INCORRECTO");
      delay_ms(800);
      goto LOOP_INGRESO;
   }
   lcd_putc("CORRECTO");
   delay_ms(800);
   lcd_send_byte (0, LCD_CLEAR);


   //CABECERA MENU
   LOOPMENU:
   do{
      printf(lcd_putc, "--MENU USUARIO--");
      lcd_gotoxy(1,2);
      lcd_putc(127);
      for (i=0; i<14; i++){
         //Mostramos por pantalla el mensaje entero
         printf(lcd_putc, "%c", tabla_menu[ind_menu][i]);
      }
      lcd_putc(126);
      tecla=teclado();
      //Establecemos los límites
      if (tecla == 'R') ind_menu++;
      if (tecla == 'L'){
         if (ind_menu==0) ind_menu=5;
         else ind_menu--;
      }
      if (tecla == 'C') return;
      if (ind_menu>5) ind_menu=0;
      //Mientras que no se pulse ok...
   }while(!(tecla == 'O' || tecla =='D'));

   //SUB MENU-CAMBIO CONRASEÑA
   if (ind_menu==0){
      ind_menu=1;
      lcd_send_byte (0, LCD_CLEAR);

      LOOP_CONTRA:
      printf(lcd_putc, "-CAMBIO CONTRAS-");
      lcd_gotoxy(1,2);
      printf(lcd_putc, "ACTUAL:         ");
      lcd_gotoxy(9,2);
      for(dir=0; dir<4; dir++){
         do{
            tecla=teclado();
         }while(tecla=='L' || tecla=='R' || tecla=='O' || tecla=='D');
         if (tecla=='C' || tecla=='U') goto LOOPMENU;
         intento[dir]=tecla;
         printf(lcd_putc, "*");
      }

      for(dir=0; dir<4; dir++){
         clave[dir]=read_eeprom((dir+2));
         delay_ms(15);
      }

      i=0;
      for(dir=0; dir<4; dir++){
         if(clave[dir]==intento[dir]) i++;
      }

      if (i!=4) {
         lcd_gotoxy(8,2);
         printf(lcd_putc, "INCORRECTO");
         delay_ms(800);
         goto LOOP_CONTRA;
      }
      establecercontras();
      goto LOOPMENU;
   }

   //SUBMENU GESTION PISCINA
   else if(ind_menu==1){
      LOOPGESTION:
      ind_menu=0;
      do{
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "-GESTION PISCIN-");
         lcd_gotoxy(1,2);
         lcd_putc(127);
         for (i=0; i<14; i++){
            //Mostramos por pantalla el mensaje entero
            printf(lcd_putc, "%c", tabla_1[ind_menu][i]);
         }
         lcd_putc(126);
         tecla=teclado();
         //Establecemos los límites
         if (tecla == 'R') ind_menu++;
         if (tecla == 'L'){
            if (ind_menu==0) ind_menu=5;
            else ind_menu--;
         }
         if (tecla=='C' || tecla=='U') goto LOOPMENU;
         if (ind_menu>5) ind_menu=0;
         //Mientras que no se pulse ok...
      }while(!(tecla == 'O' || tecla =='D'));

      if(ind_menu==0){
         //mostramos la temepratura establecida y daremos la oprtunidad de modificarla.
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "%d", tempUs);
         lcd_putc (223);
         lcd_putc ("C ");
         lcd_gotoxy(1,2);
         printf(lcd_putc, "Modificar?");
         tecla=teclado();
         if (tecla=='O'){
            lcd_send_byte(0, LCD_CLEAR);
            printf(lcd_putc, "Nueva temp:");
            establecerTemperatura();
         }
         else goto LOOPGESTION;
       }
      else if(ind_menu==1){
         //mostramos el nivel de cloro establecido y daremos la oprtunidad de modificarlo
         lcd_send_byte(0, LCD_CLEAR);

         printf(lcd_putc, "%1.1f mgr/l", conversionInt2Cloro(cloroUs));
         lcd_gotoxy(1,2);
         printf(lcd_putc, "Modificar?");
         tecla=teclado();
         if (tecla=='O'){
            lcd_send_byte(0, LCD_CLEAR);
            printf(lcd_putc, "Nuevo nivel?");
            establecerCloro();
         }
         else goto LOOPGESTION;
       }
   else if(ind_menu==2){
      //mostramos el nivel de turbidez establecido y daremos la oprtunidad de modificarlo
         lcd_send_byte(0, LCD_CLEAR);

         printf(lcd_putc, "%1.1f Kpart/l", conversionInt2Turbidez(turbidezUs));
         lcd_gotoxy(1,2);
         printf(lcd_putc, "Modificar?");
         tecla=teclado();
         if (tecla=='O'){
            lcd_send_byte(0, LCD_CLEAR);
            printf(lcd_putc, "Nuevo nivel?");
            establecerTurbidez();
         }
         else goto LOOPGESTION;
      }
      else if(ind_menu==3){
      //mostramos el nivel de agua establecido y daremos la oprtunidad de modificarlo
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "%1.1f cm neg", conversionInt2NivelAgua(nivelUs));
         lcd_gotoxy(1,2);
         printf(lcd_putc, "Modificar?");
         tecla=teclado();
         if (tecla=='O'){
            lcd_send_byte(0, LCD_CLEAR);
            printf(lcd_putc, "Nuevo nivel?");
            establecerNivel();
         }
         else goto LOOPGESTION;

      }
      else if(ind_menu==4){
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "Luminos. actual:");
         lcd_gotoxy(4,2);
         printf(lcd_putc, "%1.1f lux", medirNivelLuz());
         lcd_gotoxy(1,2);
         tecla=teclado();
         goto LOOPGESTION;
      }
      else{GOTO LOOPMENU;}
   }
   else if(ind_menu==2){
      LOOPHORA:
      ind_menu=0;
      do{
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "--GESTION HORA--");
         lcd_gotoxy(1,2);
         lcd_putc(127);
         for (i=0; i<14; i++){
            //Mostramos por pantalla el mensaje entero
            printf(lcd_putc, "%c", tabla_2[ind_menu][i]);
         }
         lcd_putc(126);
         tecla=teclado();
         //Establecemos los límites
         if (tecla == 'R') ind_menu++;
         if (tecla == 'L'){
            if (ind_menu==0) ind_menu=1;
            else ind_menu--;
         }
         if (tecla=='C' || tecla=='U') goto LOOPMENU;
         if (ind_menu>1) ind_menu=0;
         //Mientras que no se pulse ok...
      }while(!(tecla == 'O' || tecla =='D'));
      if(ind_menu==0){
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "Cambiar hora?");
         tecla=teclado();
         if (tecla=='O'){
            modificarHoraRTC();
         }
         else goto LOOPHORA;
      }
      else goto LOOPMENU;
   }
   else if(ind_menu==3){
      LOOPDEPURACION:
      ind_menu=0;
      do{
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "-GESTION DEPURA-");
         lcd_gotoxy(1,2);
         lcd_putc(127);
         for (i=0; i<14; i++){
            //Mostramos por pantalla el mensaje entero
            printf(lcd_putc, "%c", tabla_3[ind_menu][i]);
         }
         lcd_putc(126);
         tecla=teclado();
         //Establecemos los límites
         if (tecla == 'R') ind_menu++;
         if (tecla == 'L'){
            if (ind_menu==0) ind_menu=4;
            else ind_menu--;
         }
         if (tecla=='C' || tecla=='U') goto LOOPMENU;
         if (ind_menu>4) ind_menu=0;
         //Mientras que no se pulse ok...
      }while(!(tecla == 'O' || tecla =='D'));

      if(ind_menu==0){
         //posibilita comenzar el ciclo de depuración
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "Comenzar");
         lcd_gotoxy(1,2);
         printf(lcd_putc, "depuracion?");
         tecla=teclado();
         if (tecla=='O'){
            lcd_send_byte(0, LCD_CLEAR);
            printf(lcd_putc, "Depurando");
            output_bit(PIN_E2, 1);
            delay_ms(300);
         }
         else goto LOOPDEPURACION;
       }
       else if(ind_menu==1){
         //posibilita detener el ciclo de depuración
         lcd_send_byte(0, LCD_CLEAR);

         printf(lcd_putc, "Parar");
         lcd_gotoxy(1,2);
         printf(lcd_putc, "depuracion?");
         tecla=teclado();
         if (tecla=='O'){
            lcd_send_byte(0, LCD_CLEAR);
            printf(lcd_putc, "Parado");
            output_bit(PIN_E2, 0);
            delay_ms(300);
         }
         else goto LOOPDEPURACION;
       }
      else if(ind_menu==2){
      //posibilita modificar la hora de comienzo del ciclo
         lcd_send_byte(0, LCD_CLEAR);

         printf(lcd_putc, "Modificar hora");
         lcd_gotoxy(1,2);
         printf(lcd_putc, "comienzo?");
         tecla=teclado();
         if (tecla=='O'){
            lcd_send_byte(0, LCD_CLEAR);
            modificarAlarmaRTC();
            iniciarAlarmaRTC();
         }
         else goto LOOPDEPURACION;
      }
      else if(ind_menu==3){
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "Modificar tiempo");
         lcd_gotoxy(1,2);
         printf(lcd_putc, "depurado?");
         tecla=teclado();
         if (tecla=='O'){
            establecerTipo();
         }
         else goto LOOPDEPURACION;
      }
      else goto LOOPMENU;
   }
   else if(ind_menu==4){
      LOOPAHORRO:
      do{
         ind_menu=0;
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "-GESTION SLEEP-");
         lcd_gotoxy(1,2);
         lcd_putc(127);
         for (i=0; i<14; i++){
            //Mostramos por pantalla el mensaje entero
            printf(lcd_putc, "%c", tabla_4[ind_menu][i]);
         }
         lcd_putc(126);
         tecla=teclado();
         //Establecemos los límites
         if (tecla == 'R') ind_menu++;
         if (tecla == 'L'){
            if (ind_menu==0) ind_menu=1;
            else ind_menu--;
         }
         if (tecla=='C' || tecla=='U') goto LOOPMENU;
         if (ind_menu>1) ind_menu=0;
         //Mientras que no se pulse ok...
      }while(!(tecla == 'O' || tecla =='D'));
      if(ind_menu==0){
         //posibilita comenzar el ciclo de depuración
         lcd_send_byte(0, LCD_CLEAR);
         printf(lcd_putc, "Mod. tiempo");
         lcd_gotoxy(1,2);
         printf(lcd_putc, "de espera?");
         tecla=teclado();
         if (tecla=='O'){
            lcd_send_byte(0, LCD_CLEAR);
            establecerTiempo();
         }
         else goto LOOPAHORRO;
      }
         else goto LOOPMENU;
   }
   else return;
}

void actualizarNivelesUs(){
//Se actualizan los niveles al salir del menu
   TempUs=read_eeprom(6);
   delay_ms(15);
   CloroUs=read_eeprom(7);
   delay_ms(15);
   TurbidezUs=read_eeprom(8);
   delay_ms(15);
   NivelUs=read_eeprom(9);
   delay_ms(15);
   TiempoUs=read_eeprom(10);
   delay_ms(15);
}

void menuRS232(){
   char tecla;
   printf("Pulse la tecla 'e' en cualquier momento para salir\n\r");
   while(1){
      MENURS232:
      printf("<<MENU DE CONTROL PISCINA>> \n\r1. Gestion de la piscina\n\r2. Cambio del reloj\n\r3. Cambio de ciclo de depuracion\n\r4. Modo ahorro de energia\n\r");
      tecla = getchar();
      if(tecla=='1'){
         printf("<<MENU DE GESTION DE LA PISCINA>>\n\r1. Temperatura\n\r2. Cloro\n\r3. Turbidez\n\r4. Nivel de agua\n\r5. Luz\n\r0. Paso atras\n\r");
         tecla = getchar();
         if(tecla=='1'){
            printf("\tTEMPERATURA\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='2'){
            printf("\tCLORO\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='3'){
            printf("\tTURBIDEZ\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='4'){
            printf("\tNIVEL AGUA\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }else if(tecla=='5'){
            printf("\tLUZ\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='0') goto MENURS232;
         else if(tecla=='e') return;
      }
      else if(tecla=='2'){
         printf("<<MENU DE GESTIÓN RELOJ>>\n\r1. Cambiar hora\n\r0. Paso atras\n\r");
         tecla = getchar();
         if(tecla=='1'){
            printf("\tCAMBIAR HORA\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='0') goto MENURS232;
         else if(tecla=='e') return;
      }
      else if(tecla=='3'){
         printf("<<MENU DE DEPURACION>>\n\r1. Depurar ahora\n\r2. Parar depuración\n\r3. Modificar hora ciclo\n\r4. Modo tratamiento\n\r0. Paso atras\n\r");
         tecla = getchar();
         if(tecla=='1'){
            printf("\DEPURAR AHORA\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='2'){
            printf("\tPARAR DEPURACION\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='3'){
            printf("\tMODIFICAR CICLO\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='4'){
            printf("\tMODO TRATAMIENTO\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='0') goto MENURS232;
         else if(tecla=='e') return;

      }
      else if(tecla=='4'){
         printf("<<MENU DE MODO AHORRO>>\n\r1. Tiempo de espera\n\r2. Activar\n\r0. Paso atras\n\r");
         tecla = getchar();
         if(tecla=='1'){
            printf("\tTiempo de espera\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='2'){
            printf("\tActivar\n\r");
            tecla = getchar();
            if(tecla=='0') goto MENURS232;
            if(tecla=='e') return;
         }
         else if(tecla=='0') goto MENURS232;
         else if(tecla=='e') return;

      }
      if(tecla=='e') return;
   }
}
