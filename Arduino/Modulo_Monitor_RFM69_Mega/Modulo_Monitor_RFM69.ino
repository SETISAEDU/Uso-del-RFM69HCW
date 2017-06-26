// Librerías utilizadas
#include <RFM69.h> 
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Se configura la dirección de la LCD 0x27 para la mayoria de 16x2 
//                                     0x3F para la mayoria de 20x4
LiquidCrystal_I2C lcd(0x3F, 20, 4); //Construccion del objeto de control de la LCD

//Definiciones de los caracteres
#define Antena 0
#define Muerta 1
#define Pobre 2
#define Media 3
#define Fuerte 4
//Creacion de cada caracter de forma individual
byte AntenaChar[8] = {
  B00000,
  B11111,
  B10001,
  B01110,
  B00100,
  B00100,
  B00100,
  B00100
};

byte MuertaChar[8] = {
  B00000,
  B00000,
  B00000,
  B10001,
  B01010,
  B00100,
  B01010,
  B10001
};

byte PobreChar[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B10000,
  B10000
};

byte MediaChar[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00100,
  B00100,
  B10100,
  B10100
};

byte FuerteChar[8] = {
  B00000,
  B00001,
  B00001,
  B00001,
  B00101,
  B00101,
  B10101,
  B10101
};

//Definicion de los parametros de configuracion de el nodo
// CAMBIAR ESTO PARA CADA MODULO!
#define RED_TRABAJO    0   //Identificacion de la red, debe ser la misma para todos los nodos entre 0 y 255
#define NODE_ID       2   //Identificacion individual del Nodo, debe ser diferente para cada nodo
#define NODE_DESTINO  1   //Identificacion del nodo con el que se desea comunicar (0 to 254, 255 = broadcast)

//Definicion de la frecuencia de nuestro modulo
#define FRECUENCIA     RF69_915MHZ

//Parametros de encriptacion:
#define CLAVE "SETISA-EDUCATION" //Clave de encriptacion, se debe usar la misma en todos los modulos
                      
RFM69 radio;//Definicion del objeto para el control del modulo

//Declaracion de variables globales
volatile unsigned long last_micros; //Variable para guardar el valor de la ultima lectura de tiempo
long tiempo = 100; //Tiempo de espera para evitar los rebotes
int x=0,setpoint=80,Error=0;
bool BanderaEnvio=false,utimaAlarma=false,Conexion=true;



void setup()
{
  // Iniciacion de la LCD
  lcd.begin();
  // registro el nuevo caracter
  lcd.createChar(Antena, AntenaChar);
  lcd.createChar(Muerta, MuertaChar);
  lcd.createChar(Pobre, PobreChar);
  lcd.createChar(Media, MediaChar);
  lcd.createChar(Fuerte, FuerteChar);

  // Configuracion de la LCD y mensaje inicial
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Modulo de monitoreo");
  lcd.setCursor(3, 1);
  lcd.print("de Temperatura.");
  lcd.setCursor(3, 2);
  lcd.print("** SETISAEDU **");
  lcd.setCursor(0, 3);
  lcd.print("Iniciando");
  for (int i = 9; i < 20; i++)
  {
    delay(200);
    lcd.setCursor(i, 3);
    lcd.print(".");
  }
  delay(100);

  // Mensaje de operacion del modulo
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Modulo monitor");
  //Serial.begin(9600); //Solo para depuracion

  // Inicializacion del modulo  RFM69HCW
  radio.initialize(FRECUENCIA, NODE_ID, RED_TRABAJO); // configuracion de los parametros de red
  radio.setHighPower(); // Usar siempre la configuracion de la maxima para el RFM69HCW 
  // Encendido de la encriptacion
  radio.encrypt(CLAVE);
  //Serial.println("Inicializacion terminada"); //Mensaje de finalizacion solo para depuracion

  //impresion de valores iniciales 
  CheckSignal(-100);
  LeerAjuste();
  delay(100);
  PedirTemp();
  
  //Configuracion del pin de interrupcion, con la activacion de la resistencia PULLUP
  pinMode(4, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(4), EnvioSP, FALLING);
}

void loop()
{
  if(x==10) //Verificacion de que han pasado 10 ciclos del Loop
  {
    PedirTemp(); //Se consulta la temperatura
    x=0; //Reinicio de contador de ciclos
  }
  x++;
  // Envio
  LeerAjuste();//Se lee e imprime el nuevo valor de ajuste del potenciometro
  if(BanderaEnvio==true)
  {
    ConfigurarSP(); //Se configura el nuevo Setpoint
    BanderaEnvio=false;  
  }
  
  // Recepcion
  if (radio.receiveDone()) //Espera hasta recibir un dato
  {
    String data = ""; //Cadena de rececion 
    String Temp = ""; //Cadena para la temperatura
    String SetP = "";   //Cadena para el setpoint
    
    for (byte i = 0; i < radio.DATALEN; i++)
    {
      data.concat((char)radio.DATA[i]); //Se guardan los datos recibidos 
    }
    if(data[0]=='A') //Comprobacion si la alarma fue activada
    {
     // Serial.println("\n\nAlarma\n"); //para depuracion
      lcd.setCursor(6, 3);
      lcd.print("Alerta!!!"); //Impresion de mensaje de alerta
      utimaAlarma=true;
    }else
    { //En el caso de que la alarma este apagada
      if(utimaAlarma==true)
      {
        lcd.setCursor(0, 3);
        lcd.print("               ");
        utimaAlarma=false;
      }
    }
    //Separacion de los datos recibidos
    uint8_t p = data.indexOf("P");
    Temp=data.substring(1,p);
    SetP=data.substring(p+1,radio.DATALEN);
    //conversion a valores numericos
    float data_temp = Temp.toFloat();
    int data_setp = SetP.toInt();
    //Serial.println("Temp: "+String(data_temp));//Para depuracion
    // Impresion de la informacion
    lcd.setCursor(0, 1);
    lcd.print("Temperatura:       ");
    lcd.setCursor(13, 1);
    lcd.print(data_temp);
    lcd.setCursor(12, 2);
    lcd.print("SP:     ");
    lcd.setCursor(16, 2);
    lcd.print(data_setp);

    // RSSI "Receive Signal Strength Indicator"
    CheckSignal(radio.RSSI); //Se llama la funcion de verificacion de la CheckSignal

    //Envio de ACK cuando es solicitada
    if (radio.ACKRequested())
    {
      //Serial.println("ACK enviado"); 
      radio.sendACK();
    }
    delay(10);
  }
  delay(10);
}

void EnvioSP()
{
  if ((long)(micros() - last_micros) >= tiempo * 1000) { //Espera de tiempo de estabilizacion del switch
      BanderaEnvio=true; //Activacion de la bandera de envio de set de temperatura
    last_micros = micros();
  }
}
void LeerAjuste()
{
  setpoint = analogRead(A0);
  setpoint = map(setpoint, 0, 1023, -20, 300); //min temp, max temp
  lcd.setCursor(0, 2);
  lcd.print("Ajuste:    ");
  lcd.setCursor(8, 2);
  lcd.print(setpoint);
}

void ConfigurarSP() { //Funcion para configurar un nuevo Setpoint
  String str = 'S' + String(setpoint);
  int longitud = str.length();
  char buffer_envio[longitud];
  str.toCharArray(buffer_envio,sizeof(buffer_envio)+1);
  //Serial.println("Enviado setpoint " + str);//Para depuracion
  radio.sendWithRetry(NODE_DESTINO, buffer_envio, sizeof(buffer_envio));//Envio con solicitud de ACK
  delay(20);
}

void PedirTemp()
{
  //Peticion de temperatura
  char buf[]="T";
  //Serial.println("Consultando temperatura");
  if (radio.sendWithRetry(NODE_DESTINO, buf, sizeof(buf)))
  {
     Error=0;
     if(Conexion==false)
      {
        lcd.setCursor(0, 3);
        lcd.print("                 ");
        ConfigurarSP();
        Conexion=true;
      }
     //Serial.println("ACK recibido");
  }else
  {
     Error++;
     //Serial.println("ACK  norecibido");
  }
  //Comprobacion de la conexion
  if(Error==5)
  {
    CheckSignal(-100);
    lcd.setCursor(4, 3);
    lcd.print("Sin conexion");
    Error=0;
    Conexion=false;
  }
  delay(15);
}

void CheckSignal(int potencia) {
  lcd.setCursor(18, 0);
  lcd.write(Antena);
  if (potencia < -91)
  {
    lcd.write(Muerta);
  }
  if ((potencia >= -90) && (potencia <= -61))
  {
    lcd.write(Pobre);
  }
  if ((potencia >= -60) && (potencia <= -31))
  {
    lcd.write(Media);
  }
  if (potencia >= -30)
  {
    lcd.write(Fuerte);
  }
}
