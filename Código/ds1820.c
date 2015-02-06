
/***************************************************/
/*Driver básico del termómetro/termostato ds1820.c */
//
//Realiza una secuencia completa de incialización, conversión
//de temperatura, y lectura del (único) sensor de temperatura
//con alimentación externa (no parásita).
//
/***************************************************/

float ds1820_read()
{
 int8 busy=0, temp1, temp2;
 signed int16 temp3;
 float result;

 onewire_reset();     // Cada acceso al sensor debe iniciarse con reset
 onewire_write(0xCC); //Instrucción SKIP ROM (selecciona todos los dispositivos del 1wire)
 onewire_write(0x44); //Orden de inicio de conversión de temperatura

 while (busy == 0) 	//el sensor mantiene la linea a 0 mientras realiza la conversión
  busy = onewire_read(); //y la deja a 1 al completarla

 onewire_reset();
 onewire_write(0xCC);
 onewire_write(0xBE); //Envío de la orden READ SCRATCHPAD (lectura de la memoria temporal)
 temp1 = onewire_read(); //lectura del LSB de la temperatura
 temp2 = onewire_read(); //lectura del MSB de la temperatura

 temp3 = make16(temp2, temp1); //
 result = (float) temp3 / 2.0;   //Calculation for DS18S20 with 0.5 deg C resolution
// result = (float) temp3 / 16.0;  //Calculation for DS18B20 with 0.1 deg C resolution

 delay_ms(200);
 return(result);
}

//////////////////////////////////////////////////////////////////////////////////////

