/* \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*-------*///////////////////////////////// * \
 /* \\ SISTEMAS ELECTRÓNICOS DIGITALES  UNIVERSIDAD DE GRANADA 2010 - 2011 // * \
  /* \\\\\\\\\ SISTEMA ELECTRÓNICO DIGITAL DE UNA PISCINA CUBIERTA ///////// * \
   /* \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*---*//////////////////////////////// * \
    /* \\\\\\\\\\\\\\\\\\\\\\ EMILIO MARTÍNEZ PÉREZ ////////////////////// * \
     /* \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*//////////////////////////////// * \

//////////////////////DIRECTIVAS DE INCLUSIÓN Y DEFINICIÓN\\\\\\\\\\\\\\\\\\\\\\\

#include "18F4520.H"
#use delay(clock=8000000, restart_wdt)// función restart_WDT() para que el compilador reinicie el WDT durante el retardo
#use rs232 ( baud = 4800, xmit = PIN_C6, rcv = PIN_C7, stream = FT232 )
#fuses NOPROTECT,NOCPD,NOLVP,HS,NOXINST,WDT16384//Protección de código y datos=OFF, LVP=OFF, WDT=OFF, OSC=XT y desbordamiento de WDT cada 65,5 segundos (65536 ms = 16384*4ms)
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "PCF8583.c" //funciones para el PCF8583 a través de I2C
#include "LCDeasy.c" //funcines para el LCD
#include "1wire.c" //funciones básicas para gestión del 1wire
#include "ds1820.c" //función de medida de temperatura
#include "keyboard.c" //función de toma de datos por el teclaDO
#include "2432.c" //función de gestión memoria EXTERNa EEPROM
#include "funciones.c" //funciones auxiliares del sistema

////////////////////////////////////VARIABLES\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
short bajoconsumo=FALSE, gestionmenu=FALSE, depurado=FALSE, gestionmenuRS232=FALSE, medicion=FALSE;
char in_rs232;
int aux;
long int contador_timer1=0;
float tiempoinactividad=0, maximoinactividad, tiempo_depurado=0, tiempoCloro=0, tiempoFloculante=0; //Medidos en segundos
float cloro, turbidez, temperatura, nivelAgua, nivelLuz; //variables para resultaDOs de conversiones A/D, se supone ADC = 8 en la directiva DEVICE del archivo 18F4520.h
void comprobarNiveles();
void mostrar();

////////////////////////////////FUNCIONES USUALES\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void mostrar(){

   strcpy(weekday, semana[dt.weekday]);  // ptr=strncpy (s1, s2,  n) Copy up to n characters s2->s1
   //Si el dígito pasado por dt.XXXX es de una sola unidad, lo detectamos y colocamos un cero antes de la difra.
   //Si no estamos gestionando el sistema por la consola
   if(gestionMenuRS232==FALSE){
      //Escribimos la fecha
      printf("Fecha: %s, ", weekday);
      if (dt.day<10) printf("0%u/", dt.day);
      else printf("%2u/", dt.day);
      if (dt.month<10) printf("0%u/", dt.month);
      else printf("%2u/", dt.month);
      if (dt.year<10) printf("0%u", dt.year);
      else printf("%2u", dt.year);
      //Escribimos la hora
      printf(" -  Hora: ");
      if (dt.hours<10) printf("0%u:", dt.hours);
      else printf("%2u:", dt.hours);
      if (dt.minutes<10) printf("0%u:", dt.minutes);
      else printf("%2u:", dt.minutes);
      if (dt.seconds<10) printf("0%u \n\r", dt.seconds);
      else printf("%2u \n\r", dt.seconds);

      printf("Temperatura agua: %3.1f - ", temperatura);
      if(temperatura >= (tempUs+2)) printf("Agua demasiado caliente  ");
      else if( temperatura >= tempUs && temperatura < (tempUs+2)) printf("Temperatura optima.      ");
      else printf("Temperatura baja.        ");

      printf("CLORO: %3.1f mgr/l, TURBIDEZ: %3.1f kpart/l\n\r", cloro, turbidez);
      fputc('\n',FT232);
      fputc('\r');
   }
}

void comprobarNiveles(){

   //Si la temperatura del sistema es menor que la establecida, se enciende la caldera. Sino, no.
   if(temperatura<=TempUs) output_bit(PIN_C2, 1);
   else {output_bit(PIN_C2, 0);}

   //Si el nivel del agua es menor al establecido, se encienden los motores y se llena la piscina.
   if(nivelAgua>conversionInt2NivelAgua(NivelUs)){output_bit(PIN_C5, 1);}
   else {output_bit(PIN_C5, 0);}

   //Se comprueban los niveles de cloro y turbidez para actuar en consecuencia en el depurado
   //Siempre y cuando ya no se esté depurando
   if(!depurado){
      //Si el cloro es menor al recomendado se introduce a la piscina una cantidad proporcional al mismo
      if(cloro<conversionInt2Cloro(CloroUs)){
         tiempoCloro=(conversionInt2Cloro(CloroUs)-cloro)*10/1.4;
      }

      //Si el nivel de turbidez es mayor, se introduce a la piscina una cantidad proporcional de floculante
      if(turbidez>conversionInt2Turbidez(TurbidezUs)){
         tiempoFloculante=(turbidez-conversionInt2Turbidez(TurbidezUs))*10/4;
      }
   }
}

////////////////////////////INTERRUPCCIONES EXTERNAS\\\\\\\\\\\\\\\\\\\\\\\\\\\\

#int_ext
alarma_interrupcion(){

   //bajamos el flag de alarma

   clear_interrupt(int_ext2);
   //Al producirse la alarma del RTC se provoca una interrupción.
   //Comprobamos cuando tiempo se quiere depurando.
   //aux=read_eeprom(13);
   delay_ms(20);
   if (aux==0) tiempo_depurado=200;
   else if (aux==1) tiempo_depurado=50;
   else tiempo_depurado=10;
   depurado=TRUE;   output_bit(PIN_E2, 1);



}

#int_ext1
menu_interrupcion(){
   clear_interrupt(int_ext1);
   if (!gestionmenu) bajoconsumo=FALSE; gestionmenu=TRUE;
   if (gestionmenu) return;
}


////////////////////////////INTERRUPCCIONES INTERNAS\\\\\\\\\\\\\\\\\\\\\\\\\\\\

//Rutina de Interrupcion del timer0, para llevar el contador para el modo sleep y el tiempo de funcionamiento del procesado de la piscina.
//Se produce un desbordamiento cada 8,5 segundos aproximadamente.
#INT_TIMER0
rutina_timer0(){///COMPLETAR
   if (tiempoinactividad >= (maximoinactividad-5)) bajoconsumo=TRUE;
   tiempoinactividad += 8.5;
   if(depurado && !bajoconsumo){
      tiempoinactividad=0;
      tiempo_depurado -= 8.5;
      if(tiempo_depurado<0){
         output_bit(PIN_E2, 0);
         //PCF8583_write_byte(PCF8583_CTRL_STATUS_REG, PCF8583_START_COUNTING_WITH_ALARM);
         depurado=FALSE;
      }
   }
}

#INT_TIMER1
tratamiento()
{
   if (contador_timer1 == 237){
      contador_timer1=0;
   }

    //Si el timer se desborda señalamos para que el loop principal haga las mediciones
   if (contador_timer1 == 0 ) medicion = TRUE;

   //Contador del tiempo de cloro y floculante
   if (depurado && tiempo_depurado<20){
      if(tiempoCloro >0) output_bit(PIN_C1,1);
      else output_bit(PIN_C1,0);
      if(tiempoFloculante>0) output_bit(PIN_C0,1);
      else output_bit(PIN_C0,0);

      tiempoCloro-=0.23;
      tiempoFloculante-=0.23;

   }
   contador_timer1++;
   restart_wdt ( );
   set_timer1 ( ~25000 ); //Repone TMR1 con el complemento a uno de 25000 [( 2**16 ) - 1 - 25000]
}

///////////////////////////////////FUNCION MAIN\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void main()
{

   //CONFIGURACIÓN DE LOS PINES
   //PUERTO A

   setup_adc_ports( AN0_TO_AN3 ); //RA0 y RA1 entradas analógicas (ver 18F4520.h)

   //PUERTO B 00000011
   set_tris_b(0x03);    // Fijamos RB0 - RB1 como entrada y RB2-RB7 salida
   //PUERTO C 00000000
   set_tris_c(0xFF);    // Fijamos RC0 - RC7 como entrada.
   //PUERTO D 00001111
   set_tris_d(0x0F);    // Fijamos RD0 - RD3 entradas, RD4-RD7 salidas
   //PUERTO E     1100
   set_tris_e(0x0C);     //Fijamos RE0 - RE1 como salida y RE2 - RE3 como entrada

   setup_adc ( adc_clock_div_32 ); //Ajusta tiempo de conversión de cada bit

   /*
   |===============================| Ponemos valores lógicos en
   | E1 | E0 |   Luz   |    Modo   | las puertas que gobiernan
   |===============================| en led siguiendo la tabla.
   |  0 |  0 | Apagado |  Apagado  |
   |-------------------|-----------|
   |  0 |  1 |  Verde  |   Normal  |
   |-------------------|-----------|
   |  1 |  0 |  Roja   | Económico |
   |-------------------|-----------|
   |  1 |  1 | Apagado |  Apagado  |
   |===================|===========| */
   output_bit(PIN_E1,0);
   output_bit(PIN_E0,1);
   output_bit(PIN_E2,0);


   //Mensaje inicial
   inicioSistema( );
   //Comprobamos el estado de la memoria EEPROM
   comprobarMemoria( );
   //Iniciamos el RTC PCF8583
   iniciarRTC();
   //Activamos la alarma del RTC para el ciclo diario de depurado
   iniciarAlarmaRTC();
   //Tomamos de la eeprom los niveles configurados
   actualizarNivelesUs();

   maximoInactividad=TiempoUs;

   //Mensaje para conexión serie
   printf ( "Comenzamos el log.\n\r" );
   printf ( "Presiona q para finalizar el log.\n\r" );
   printf ( "La toma de medidas se produce cada minuto, tenga paciencia.\n\r" );

   enable_interrupts ( int_timer1 ); //Activa interrupción del Timer1
   enable_interrupts ( int_timer0 ); //Activa interrupción del Timer0

    //Interrupciones Externas
    enable_interrupts(int_ext); // habilitacion de interupcion externa
    ext_int_edge(H_TO_L); // interrupcion para comparacion flanco de subida
    enable_interrupts(int_ext1); // habilitacion de interupcion externa
    ext_int_edge(H_TO_L); // interrupcion para comparacion flanco de subida

    clear_interrupt(int_ext);
    clear_interrupt(int_ext1);


   /* El TMR1 trabaja con oscilaDOr interno y un preescaler de 1:8. Con un reloj
   de 8 MHz ( 0, 5us por instrucción ), el TMR1 deberá desbordarse cada 25000 cuentas
   para producir una INTerrupción cada 0.1s ( 25000 * 8 * 0, 5us = 100000uS = 0.1" ) */
   set_timer1 (~25000); //Carga TMR1 con el complemento a 1 de 25000
   setup_timer_1(T1_INTERNAL | T1_DIV_BY_8); //TMR1 ON y 1:8
   setup_timer_0(RTCC_INTERNAL);
   enable_interrupts(global);
   setup_counters(RTCC_INTERNAL,WDT_TIMEOUT);

   MAIN:
   in_rs232='a';
   while(1/*!(in_rs232 == 'e') || in_rs232 != 'm'*/){
      //Si el timer se ha desbordado se realiza la medidión de niveles.
      if (medicion){
         temperatura=medirTemperatura();
         cloro=medirNivelCloro();
         turbidez=medirNivelTurbidez();
         nivelAgua=medirNivelAgua();
         nivelLuz=medirNivelLuz();
         comprobarNiveles();
         mostrar();
         medicion=FALSE;
      }




      //APARTADO PARA EL MODO SLEEP()
      LOOP_BAJOCONSUMO:
      //Comprobar el estado de bajo consumo para entrar en modo económico
      if(bajoconsumo){
         //Indicamos por el led que estamos en modo ahorro de energía (Color rojo)
         output_bit(PIN_E0, 0);
         output_bit(PIN_E1, 1);
         aux=1;
         //Indicamos por pantallas
         printf("<<<MODO BAJO CONSUMO>>>\n\r");
         lcd_send_byte(0,LCD_CLEAR);
         while(bajoconsumo && !gestionmenu && !depurado) {//mientras no se indique lo contrario seguimos en el modo de bajo consumo

            //Actualizamos la información del LCD
            printf(lcd_putc, "<<BAJO CONSUMO>>");
            lcd_gotoxy(2,2);
            printf(lcd_putc, "%3.1f%cC ", temperatura, 223);
            lcd_gotoxy(11,2);
            if (dt.hours<10) printf(lcd_putc, "0%u:", dt.hours);
            else printf(lcd_putc, "%2u:", dt.hours);
            if (dt.minutes<10) printf(lcd_putc, "0%u", dt.minutes);
            else printf(lcd_putc, "%2u", dt.minutes);
            //Por el perro guardián, el micro se despertará cada 65,5 segundos (65536 ms = 16384*4ms).
            sleep();
            //Aprovechamos este desbordamiento del WDT para tomar algunas medidas y luego volver a llamar a sleep()
            //Consultamos la hora
            PCF8583_read_datetime (&dt);
            //Ponemos el contador del timer1 a cero para que simular un desbordamiento para la toma de medidas.
            contador_timer1=0;
            //Añadimos un pequeño retardo para que el timer1 pueda realizar las sentencias
            delay_ms(500);

            //Si está el depurado activo
            if(aux!=1){
               if (depurado) tiempo_depurado -= 65,5;
            }
            if (tiempo_depurado<0 && depurado){
               output_bit(PIN_E2, 0);
               depurado=FALSE;
            }
         }
         //Al salir del sleep ponemos verde el led, el indicador de funcionamiento 'normal'.
         output_bit(PIN_E0, 1);
         output_bit(PIN_E1, 0);
         //Ponemos el contado de inactividad a cero
         tiempoinactividad=0;
      }

      LOOP_GESTIONMENU:
      //Comprobamos si se ha pedido entrar en menú
      if(gestionmenu) {
         menu();
         tiempoinactividad=0;
         gestionmenu=FALSE;
         actualizarNivelesUs();
      }

      //HORA
      lcd_send_byte(0, LCD_CLEAR);
      PCF8583_read_datetime (&dt); //Lectura del RTC
      //Si el dígito pasado por dt.XXXX es de una sola unidad, lo detectamos y colocamos un cero antes de la difra.
      strcpy(weekday, semana[dt.weekday]);  // ptr=strncpy (s1, s2,  n) Copy up to n characters s2->s1
      //Escribimos la fecha
      printf(lcd_putc, "%s,", weekday);
      if (dt.day<10) printf(lcd_putc, "0%u/", dt.day);
      else printf(lcd_putc, "%2u/", dt.day);
      if (dt.month<10) printf(lcd_putc, "0%u/", dt.month);
      else printf(lcd_putc, "%2u/", dt.month);
      if (dt.month<10) printf(lcd_putc, "0%u", dt.year);
      else printf(lcd_putc, "%2u", dt.year);
      //Ahora la hora.
      lcd_gotoxy(1,2);
      printf(lcd_putc, "Hora: ");
      if (dt.hours<10) printf(lcd_putc, "0%u:", dt.hours);
      else printf(lcd_putc, "%2u:", dt.hours);
      if (dt.minutes<10) printf(lcd_putc, "0%u:", dt.minutes);
      else printf(lcd_putc, "%2u:", dt.minutes);
      if (dt.seconds<10) printf(lcd_putc, "0%u", dt.seconds);
      else printf(lcd_putc, "%2u", dt.seconds);
      delay_ms(2000);

      if (kbhit(FT232))in_rs232=fgetc();
      if (in_rs232=='m' || in_rs232=='e') break;
      if (gestionmenu) goto LOOP_GESTIONMENU;
      if (bajoconsumo) goto LOOP_BAJOCONSUMO;

      //TEMPERATURA
      lcd_send_byte (0, LCD_CLEAR);
      printf (lcd_putc, "TEMP: %3.1f ", temperatura);
      lcd_putc (223);
      lcd_putc ("C ");
      lcd_gotoxy (1, 2);
      if(temperatura >= (tempUs+2)) printf(lcd_putc, "Agua caliente   ");
      else if( temperatura >= tempUs && temperatura < (tempUs+2)) printf(lcd_putc, "Temp optima     ");
      else printf(lcd_putc, "Agua fria       ");

      delay_ms (2000);

      if (kbhit(FT232))in_rs232=fgetc();
      if (in_rs232=='m' || in_rs232=='e') break;
      if (gestionmenu) goto LOOP_GESTIONMENU;
      if (bajoconsumo) goto LOOP_BAJOCONSUMO;

      //CONCENTRACIONES AGUA
      lcd_send_byte (0, LCD_CLEAR);
      printf (lcd_putc, "TURB: %1.1f kpar/l", turbidez);
      lcd_gotoxy (1, 2);
      printf (lcd_putc, "CLORO: %1.1f mgr/l", cloro);
      delay_ms (2000);

      if (kbhit(FT232))in_rs232=fgetc();
      if (in_rs232=='m' || in_rs232=='e') break;
      restart_wdt();
   }

   if(in_rs232=='m'){
      lcd_send_byte(0, LCD_CLEAR);
      gestionMenuRS232=TRUE;
      printf(lcd_putc, "INTERCAMBIO DE\nDATOS RS232");
      menuRS232();
      gestionMenuRS232=FALSE;
      //Actualizamos los niveles por si hay algún cambio
      actualizarNivelesUs();
      //Nos gustaría queuna vez salimos del sistema, sepamos como nos encontramos
      PCF8583_read_datetime (&dt);
      //Ponemos el contador del timer1 a cero para que simular un desbordamiento para la toma de medidas.
      contador_timer1=0;
      goto MAIN;
   }
   printf("fin");
}
