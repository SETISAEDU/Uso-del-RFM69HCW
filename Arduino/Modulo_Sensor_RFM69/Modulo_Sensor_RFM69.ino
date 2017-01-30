// Librerías utilizadas
#include "RFM69.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

//Definicion de los parametros de configuracion de el nodo
// CAMBIAR ESTO PARA CADA MODULO!
#define RED_TRABAJO     0   // Debe ser el mismo para todos los nodos (0 a 255)
#define NODO_ID         1   // Mi identificador del nodo (0 a 255)
#define NODO_DESTINO    2   // Identificador del nodo destido (0 a 254, 255 = broadcast)

//Definicion de la frecuencia de nuestro modulo
#define FRECUENCIA     RF69_915MHZ

// Párametros de encriptación:
#define CLAVE "SETISA-EDUCATION" //Clave de encriptacion, se debe utilizar la misma para todos los módulos

RFM69 radio;//Definicion del objeto para el control del modulo

// Pines fisicos del LED RGB que se conectará
#define VERDE 8 
#define ROJO 9

// Variables de control del programa
int Ajuste_temp = 50;
char Alarma = '*';


void setup()
{
  // Inicializa el sensor de temperatura:
  mlx.begin();

  // Ajustamos los indicadores LED:
  pinMode(VERDE, OUTPUT);
  digitalWrite(VERDE, LOW);
  pinMode(ROJO, OUTPUT);
  digitalWrite(ROJO, LOW);

  // Inicializacion del modulo  RFM69HCW
  radio.initialize(FRECUENCIA, NODO_ID, RED_TRABAJO);
  radio.setHighPower(); // Siempre usar esto para el RFM69HCW

  // Encendido de la encriptacion
  radio.encrypt(CLAVE);
}

void loop()
{
  // Medición de la temperatura:
  float temp = mlx.readObjectTempC();
  
  // Compara la temperatura con el nivel ajustado:
  CompararTemp(temp);
   
  if (radio.receiveDone()) //Espera hasta recibir un dato
  {
    // Se recibe el mensaje 
    String Mensaje = "";
    for (byte i = 0; i < radio.DATALEN; i++)
    {
      // Se construye el mesaje recibido:
      Mensaje.concat((char)radio.DATA[i]);
    }
    //Envio de ACK cuando es solicitada
    if (radio.ACKRequested())
    {
      radio.sendACK();
    }
    // Si se ha solicitado envíar la temperatura:
    if (Mensaje == "T") 
    {
      // Se contruye la trama a enviar (Temperatura medida + Valor de consigna):
      String str = String(Alarma) + String(temp) + 'P'  + String(Ajuste_temp);
      int sendlength2 = str.length();
      char sendbuffer2[sendlength2];
      str.toCharArray(sendbuffer2, sizeof(sendbuffer2) + 1);

      // Verificación del envio:
      radio.sendWithRetry(NODO_DESTINO, sendbuffer2, sendlength2);
    }
    // De no ser la petición de la temperatura:
    else if (Mensaje[0] == 'S')
    {
      // Se cambia el nivel de comparación:
      String str_Ajuste = Mensaje.substring(1, radio.DATALEN - 1);
      Ajuste_temp = str_Ajuste.toInt();
    }
  }
}

// Comparación de la temperatura medida:
void CompararTemp(int temperatura) 
{
  if (temperatura >= Ajuste_temp) 
  {
    // Se activa el led ROJO:
    digitalWrite(VERDE, LOW);
    digitalWrite(ROJO, HIGH);
    // Se indica que se ha activado la alarma:
    Alarma = 'A';
  }
  else if (temperatura < Ajuste_temp) 
  {
    // Se activa el led VERDE:
    digitalWrite(VERDE, HIGH);
    digitalWrite(ROJO, LOW);
    // Se indica que se ha desactivado la alarma:
    Alarma = '*';
  }
}
