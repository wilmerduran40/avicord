
#include <DHT.h>
#include <Wire.h>
#include <hd44780.h>                       // Librería principal
#include <hd44780ioClass/hd44780_I2Cexp.h> // Clase para I2C
#define DHTPIN 22
#define DHTTYPE DHT11
#define bombillo 24
#define ventilador 23

hd44780_I2Cexp lcd;
DHT dht(DHTPIN,DHTTYPE);

//variables
float temperatura;
float humedad;
int humo;
int nh3;
int dt= 5000;
int spkPin=8;
int a=5000;
int b=2000;
const int humoNivel = A0;
const int nh3Nivel = A1;
int ledYelow=2;
int ledGreen=3;
int ledRed=4;



void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
dht.begin();
pinMode(humoNivel,INPUT);
pinMode(nh3Nivel, INPUT);
lcd.begin(20, 4); // Inicia pantalla de 16 columnas y 4 filas
lcd.backlight();  // Enciende la luz de fondo
pinMode(spkPin,OUTPUT);
pinMode(ledYelow,OUTPUT);
pinMode(ledGreen,OUTPUT);
pinMode(ledRed,OUTPUT);
pinMode(bombillo,OUTPUT);
digitalWrite(bombillo,HIGH);
pinMode(ventilador,OUTPUT);
digitalWrite(ventilador,HIGH);
}

//Parametros segururos de temperatura,humedad y Nh3
  float tempMin=34.5;
  float tempMax=37.5;
  float humMax=60;
  float humMin=40;
  float nh3Max=20;

void loop() {
  // --- BLOQUE 1: LECTURAS (Actualiza variables) ---
  humedad = dht.readHumidity();
  temperatura = dht.readTemperature();
  
  // Quitamos el "int" de aquí porque ya están declaradas arriba (Globales)
  int lecturaHumo = analogRead(A0);
  humo = map(lecturaHumo, 54, 1023, 0, 100);
  humo = constrain(humo, 0, 100);
  
  int lecturaNH3 = analogRead(A1);
  nh3 = map(lecturaNH3, 48, 1023, 0, 300);
  nh3 = constrain(nh3, 0, 300);

  // --- BLOQUE 2: TOMA DE DECISIONES (Lógica combinada) ---
  // Creamos estados lógicos (Banderas)
  bool alertaGases = (humo > 36 || nh3 > 20);
  bool alertaClima = (temperatura > tempMax || humedad > 70);
  bool temperatura_minima = (temperatura < tempMin);
  bool temperatura_optima=(temperatura<=36.5);

  // --- BLOQUE 3: ACCIONES DE HARDWARE (Una sola orden por pin) ---
  
  // Ventilador: Se activa si hay gases O si hace mucho calor/humedad
  if (alertaGases || alertaClima) {
    digitalWrite(ventilador, LOW); // ON
  } else {
    digitalWrite(ventilador, HIGH); // OFF
  }
  if (temperatura < 34.5) {
  temperatura_minima = true; 
} 
// Si sube de 36.5, apagamos.
else if (temperatura > 36.5) {
  temperatura_minima = false;
}
  // Bombillo: Solo si hace frío (y no hay alerta de calor)
  if (temperatura_minima) {
    digitalWrite(bombillo, LOW); // ON (Calentar)
  } else {
    digitalWrite(bombillo, HIGH); // OFF
  }

  // --- BLOQUE 4: VISUALIZACIÓN LCD (Sin parpadeos) ---
  // En lugar de lcd.clear(), sobreescribimos los espacios
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temperatura, 1);
  if(temperatura_minima) lcd.print(" !Temp BAJA!    "); else lcd.print(" Normal    ");

  lcd.setCursor(0, 1);
  lcd.print("H:"); lcd.print(humedad, 1);
  if(humedad > 70) lcd.print(" !ALTA!    "); else lcd.print(" % OK      ");

  lcd.setCursor(0, 2);
  if(alertaGases) lcd.print("ALERTA GAS/HUMO!   ");
  else lcd.print("Aire Limpio        ");

  lcd.setCursor(0, 3);
  lcd.print("NH3:"); lcd.print(nh3); lcd.print(" ppm  Humo:"); lcd.print(humo); lcd.print("%  ");

  delay(1000); // Un segundo es suficiente para monitorear
}
