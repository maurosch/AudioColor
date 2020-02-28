#ifdef ESP8266
#define min _min
#define max _max
#endif
#include <arduinoFFT.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <RCSwitch.h>
//#include <IRremoteESP8266.h>

// NETWORK: Static IP details...
IPAddress ip(192, 168, 0, 112);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

arduinoFFT FFT = arduinoFFT();
const uint16_t samples = 256; //This value MUST ALWAYS be a power of 2
const uint16_t samplesfft = samples >> 1;
const int nextWindowOffset = samples >> 2;

const int cpuFrequency = 160000000;
const double samplingFrequency = 10000;
const double outputFrequency = 100;
const int outputTicks = cpuFrequency / outputFrequency;
const double computeFrequency = samplingFrequency / (samples - nextWindowOffset);

double vReal[samples];
double vImag[samples];
double prevMeasures[samples - nextWindowOffset];
int redLed = 14;//PIN D5;
int greenLed = 0;//PIN D3;
int blueLed = 4;//PIN D2;
int analogPin = A0;//0;
int onLed = 2;//PIN D4 - LED DE PRENDIDO

const double voltageOffset = 488;//561;
const double intensidadLedMaxima = PWMRANGE;
const double momentoSubida = 0.7; //Promedia: Entre 0 y 1
const double momentoBajada = 0.6;

const char* ssid = "";
const char* password = "";
ESP8266WebServer server(80);   //instantiate server at port 80 (http port)
String page = "";

bool modoRitmoMusica = true;

enum enumColor {
  rojo,
  verde,
  azul,
  numColores
};

double pot[numColores];
const double freqMedia[numColores] = {78.12, 440, 1000};
const double decay[numColores] = {20, 20, 20};
double maximo[3] = {0,0,0};
const double decayMaximo = 0.999;
const int pinLed[numColores] = {redLed, greenLed, blueLed};
bool datosNuevos = false;

uint16_t rangoMascara[numColores][2] = {{samplesfft,0}, {samplesfft, 0}, {samplesfft, 0}};
double* mascara[numColores] = {};

void mostrar() {
  const double minOut = 0;//intensidadLedMaxima * 0.1;
  static double ultimoOut[numColores] = {0, 0, 0}; 
  static double out[numColores] = {0, 0, 0};
  static double proximoOut[numColores];
  static int i = 0;
  if(modoRitmoMusica)
  {
    if(datosNuevos) {
      for(uint16_t color = 0; color < numColores; color++) {
        ultimoOut[color] = proximoOut[color];
        proximoOut[color] = pow(pot[color] / maximo[color], 1) * (intensidadLedMaxima);
        datosNuevos = false;
        i = 0;
      }
    }
    else {
      i++;
    }
    for(uint16_t color = 0; color < numColores; color++){
      out[color] = interpolar(ultimoOut[color], proximoOut[color], outputFrequency / computeFrequency, i);
      //out[color] = proximoOut[color];
      if(out[color] <= minOut+1) out[color] = 0;     
      analogWrite(pinLed[color], out[color]);
    }
  }
  timer0_write(ESP.getCycleCount()+outputTicks);
}


double interpolar(double v0, double v1, int N, int i) {
  return (v1 - v0)*(1-cos(min(i/N,1) * 3.14))/ 2 + v0;
}

void InicializarColoreador() {
  double epsilon = 0.0001;
  for(int color = 0; color < numColores; color++) {
    pot[color] = 0;
    for(int i = 0; i < samplesfft; i++) {
      double freq = frecuencia(i);
      double deltaFreq = (log(freq) - log(freqMedia[color]))/(log(9)-log(8));
      double val = exp(-pow(deltaFreq, 2) / decay[color]) / freq;
      if(val >= epsilon) { 
        rangoMascara[color][0] = min(rangoMascara[color][0], i);
        rangoMascara[color][1] = max(rangoMascara[color][1], i);
      }
      delay(0.01);
    }
  }
  
  for(int color = 0; color < numColores; color++) {
    int tamano = rangoMascara[color][1]-rangoMascara[color][0];
    mascara[color] = (double*) malloc(tamano*sizeof(double));
  }
  delay(1000);
  for(int color = 0; color < numColores; color++) {
    pot[color] = 0;
    for(uint16_t i = 0, ifreq = rangoMascara[color][0]; ifreq < rangoMascara[color][1]; i++, ifreq++) {
      double freq = frecuencia(ifreq);
      double deltaFreq = (log(freq) - log(freqMedia[color]))/(log(9)-log(8));
      mascara[color][i] = exp(-pow(deltaFreq, 2) / decay[color]) / freq;
      //Serial.print(frecuencia(ifreq));
      //Serial.print("Hz:");
      //Serial.println(mascara[color][i],6);
    }
  }
}

void InicializarSalida() {
   noInterrupts();
   timer0_isr_init();
   timer0_attachInterrupt(mostrar);
   timer0_write(ESP.getCycleCount()+outputTicks);
   interrupts();
}

double frecuencia(uint16_t i)
{
  return ((i * 1.0 * samplingFrequency) / samples);
}

void setup()
{
  page = " <!DOCTYPE HTML> <html style='font-size:1.2em;'> <head> <meta charset='UTF-8'> </head> <body> <style> .Rainbow { background-image: -webkit-gradient( linear, left top, right top, color-stop(0, #f22), color-stop(0.15, #f2f), color-stop(0.3, #22f), color-stop(0.45, #2ff), color-stop(0.6, #2f2),color-stop(0.75, #2f2), color-stop(0.9, #ff2), color-stop(1, #f22) ); background-image: gradient( linear, left top, right top, color-stop(0, #f22), color-stop(0.15, #f2f), color-stop(0.3, #22f), color-stop(0.45, #2ff), color-stop(0.6, #2f2),color-stop(0.75, #2f2), color-stop(0.9, #ff2), color-stop(1, #f22) ); color:transparent; -webkit-background-clip: text; background-clip: text; filter: drop-shadow(2px 2px rgba(149, 150, 150, 0.3)); style='text-shadow: 4px 4px 2px ;' } .blanco { background-color:#ffffff; -moz-border-radius:28px; -webkit-border-radius:28px; border-radius:28px; border:1px solid #dcdcdc; display:inline-block; cursor:pointer; color:#666666; font-family:Arial; font-size:17px; font-weight:bold; padding:16px 30px; text-decoration:none; text-shadow:0px 1px 0px #ffffff; } .blanco:hover { background-color:#f6f6f6; } .blanco:active { position:relative; top:1px; } .ritmoMusica { background-color:#ff5bb0; -moz-border-radius:28px; -webkit-border-radius:28px; border-radius:28px; border:1px solid #ee1eb5; display:inline-block; cursor:pointer; color:#ffffff; font-family:Arial; font-size:17px; font-weight:bold; padding:16px 30px; text-decoration:none; text-shadow:0px 1px 0px #c70067; } .ritmoMusica:hover { background-color:#ef027d; } .ritmoMusica:active { position:relative; top:1px; } .violeta { background-color:#dfbdfa; -moz-border-radius:28px; -webkit-border-radius:28px; border-radius:28px; border:1px solid #c584f3; display:inline-block; cursor:pointer; color:#ffffff; font-family:Arial; font-size:17px; font-weight:bold; padding:16px 30px; text-decoration:none; text-shadow:0px 1px 0px #9752cc; } .violeta:hover { background-color:#bc80ea; } .violeta:active { position:relative; top:1px; } .apagar { background-color:#f9f9f9; -moz-border-radius:28px; -webkit-border-radius:28px; border-radius:28px; border:1px solid #dcdcdc; display:inline-block; cursor:pointer; color:#666666; font-family:Arial; font-size:17px; font-weight:bold; padding:16px 30px; text-decoration:none; text-shadow:0px 1px 0px #ffffff; } .apagar:hover { background-color:#e9e9e9; } .apagar:active { position:relative; top:1px; } .amarillo { background-color:#ffec64; -moz-border-radius:28px; -webkit-border-radius:28px; border-radius:28px; border:1px solid #ffaa22; display:inline-block; cursor:pointer; color:#333333; font-family:Arial; font-size:17px; font-weight:bold; padding:16px 30px; text-decoration:none; text-shadow:0px 1px 0px #ffee66; } .amarillo:hover { background-color:#ffab23; } .amarillo:active { position:relative; top:1px; } </style> <div style='text-align: center;'> <h1 class='Rainbow'>Luces Led ATR</h1> <p> <a class='ritmoMusica' href='ritmoMusica'> Ritmo Música </a> <a class='blanco' href='estaticoBlanco'> Blanco </a> <a class='violeta' href='estaticoVioleta'> Violeta </a> <a class='amarillo' href='estaticoAmarillo'> Amarillo </a> <a class='apagar' href='apagado'> Apagar </a> </p> <p>Versión: 25/03/2019 Mauro Schiavinato 👩🏽‍💻</p> </div> </body> </html>";
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password); //begin WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }   
  server.on("/", [](){
    server.send(200, "text/html", page);
  });
  server.on("/ritmoMusica", [](){
    server.send(200, "text/html", page);
    modoRitmoMusica = true;
    digitalWrite(onLed, LOW);
  });
  server.on("/estaticoBlanco", [](){
    server.send(200, "text/html", page);
    modoRitmoMusica = false;
    for(uint16_t color = 0; color < numColores; color++){
      analogWrite(pinLed[color], intensidadLedMaxima);
    }
    digitalWrite(onLed, LOW);
  });
  server.on("/estaticoVioleta", [](){
    server.send(200, "text/html", page);
    modoRitmoMusica = false;
    analogWrite(pinLed[0],244*intensidadLedMaxima/255);
    analogWrite(pinLed[1],66*intensidadLedMaxima/255);
    analogWrite(pinLed[2],232*intensidadLedMaxima/255);
    digitalWrite(onLed, LOW);
  });
  server.on("/estaticoAmarillo", [](){
    server.send(200, "text/html", page);
    modoRitmoMusica = false;
    analogWrite(pinLed[0],236*intensidadLedMaxima/255);
    analogWrite(pinLed[1],213*intensidadLedMaxima/255);
    analogWrite(pinLed[2],54*intensidadLedMaxima/255);
    digitalWrite(onLed, LOW);
  });
  server.on("/apagado", [](){
    server.send(200, "text/html", page);
    modoRitmoMusica = false;
    digitalWrite(onLed, HIGH);
    analogWrite(pinLed[0],0);
    analogWrite(pinLed[1],0);
    analogWrite(pinLed[2],0);
  });
  server.begin();

  pinMode(onLed, OUTPUT);
  digitalWrite(onLed, LOW);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  InicializarColoreador();
  InicializarSalida();
}

int FPS = 0;
void loop()
{
  server.handleClient();
  if(modoRitmoMusica)
  {
    for (int i = 0; i < samples - nextWindowOffset; i++) {
      vReal[i] = prevMeasures[i];
      vImag[i] = 0;
    }
    for (uint16_t i = nextWindowOffset; i < samples; i++)
    {
      vReal[i] = analogRead(analogPin) - voltageOffset;
      prevMeasures[i-nextWindowOffset] = vReal[i];
      delay(1 / samplingFrequency);
      vImag[i] = 0.0;
    }
    
    FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, samples, FFT_FORWARD); 
    FFT.ComplexToMagnitude(vReal, vImag, samples); 
    vReal[0] = 0;
    
    for(uint16_t color = 0; color < numColores; color++){
      double oldColor = pot[color];
      pot[color] = 0;
      
      for(uint16_t i = 0, ifreq = rangoMascara[color][0]; ifreq < rangoMascara[color][1]; i++, ifreq++) {
        pot[color] += vReal[ifreq] * mascara[color][i]; //* (1-momento);
      }
      if(pot[color] > oldColor){
        pot[color] *= (1-momentoSubida);
        pot[color] += oldColor * momentoSubida;
      }
      else{
        double momentoBajada = 0.6;
        pot[color] *= (1-momentoBajada);
        pot[color] += oldColor * momentoBajada;
      }
    }
    
    for(uint16_t color = 0; color < numColores; color++){
        if(pot[color] > maximo[color]) { 
          maximo[color] = pot[color];
        }
        else {
          maximo[color] = maximo[color] * decayMaximo;
          if(maximo[color] < 7)
          {
            maximo[color] = 7;  
          }
        }
    }
    datosNuevos = true;
  }
}
