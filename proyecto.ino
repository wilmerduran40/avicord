#include <DHT.h>
#include <Wire.h>
#include <hd44780.h>                       // Librería principal
#include <hd44780ioClass/hd44780_I2Cexp.h> // Clase para I2C
#define DHTPIN 22
#define DHTTYPE DHT11

hd44780_I2Cexp lcd;
DHT dht(DHTPIN,DHTTYPE);

//variables
float temperatura;
float humedad;
int dt= 5000;
int spkPin=8;
int a=200;
const int humoNivel = A0;
const int nh3Nivel = A1;
int nh3;
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
}

void loop() {
  //Parametros segururos de temperatura,humedad y Nh3
  float temp=36;
  float hum=60;
  //
  humedad = dht.readHumidity();
  temperatura = dht.readTemperature();
  
  //Calibracion 2 sensor mq135 para dectecion de humo
  int humoNivel=analogRead(A0);
  int humo=map(humoNivel, 54, 1023, 0, 100);
  humo= constrain(humo, 0, 100);
  
  // Calibracion sensor mq135 para amoniaco
int  nh3Nivel=analogRead(A1);
  nh3=map(nh3Nivel, 48, 1023, 0, 300);
  nh3=constrain(nh3, 0, 300);
  
  // Limpiar Pantalla LCD
  lcd.clear(); 

  // FILA 0: Temperatura
  lcd.setCursor(0, 0); 
  lcd.print("Temperatura: "); lcd.print(temperatura, 1); lcd.print(" C     "); 

  // FILA 1: Humedad
  lcd.setCursor(0, 1); 
  lcd.print("Humedad: "); lcd.print(humedad, 1); lcd.print(" %     ");

  //FILA 2: HUMO
  lcd.setCursor(0, 2); 
  lcd.print("HUMO/CO2: "); lcd.print(humo); lcd.print(" ppm   ");
// FILA:3 NH3
  lcd.setCursor(0, 3);
  lcd.print("Nh3:"); lcd.print(nh3); lcd.print(" ppm ");
  
  delay(dt);
  lcd.clear();

  
//1. Logica NH3

if (nh3<10){
  lcd.setCursor(0, 3);
  lcd.print("AIRE LIBRE AMONIACO ");
  digitalWrite(ledGreen,HIGH);
  digitalWrite(ledYelow,LOW);
  digitalWrite(ledRed,LOW);
  
  }else if (nh3>10 && nh3<20){
    lcd.setCursor(0, 3);
    lcd.print("NIVEL ESTABLE NH3");
    lcd.print(nh3);
    digitalWrite(ledYelow,HIGH);
    }else{
      lcd.setCursor(0,3);
      lcd.print("Peligro NIVEL ALTO:");
      lcd.print(nh3);
      digitalWrite(ledRed,HIGH);
      }

// 2. Lógica de Humo (Independiente)
//Lectura Analog (0-1023),Porcentaje (%),Estado del Aire,Acción del Sistema 
//10%,Limpio,"Sistema en espera. LCD: ""Normal"""
//11% -30%,Presencia Leve,"Registro en Log. LCD: ""Alerta Leve"""
//31%  70%,Humo Detectado,"Notificación Whatsapp + LCD: ""¡HUMO!"""
//71% -100%,Peligro Crítico,Alarma Sonora (Buzzer) + Alerta Urgente
 
  if (humo >= 0 && humo <10) {
    lcd.setCursor(0,2);
    lcd.print("Estado: Aire Limpio ");
    digitalWrite(spkPin,LOW);
    digitalWrite(ledYelow,LOW);
    digitalWrite(ledGreen,HIGH);
  } 
  //151 a 300,11% - 30%,Presencia Leve,"Registro en Log. LCD: ""Alerta Leve"""
    else if (humo > 10 && humo < 30) {
    lcd.setCursor(0,2);
    lcd.print("Estado: Alerta Leve ");
    digitalWrite(ledYelow,HIGH);

  } else if(humo > 36 && humo <=70) {
        lcd.setCursor(0,2);
    lcd.print("ALERTA: HUMO:     ");
    lcd.print(humo);
    // Aquí activas el bot y el buzzer
digitalWrite(spkPin, HIGH); 
    delay(200);
    digitalWrite(spkPin, LOW);
  } else if(humo > 70 ) {
        lcd.setCursor(0,2);
    lcd.print("Peligro Critico:     ");
    lcd.print(humo);
    // Aquí activas el bot y el buzzer
digitalWrite(spkPin, HIGH); 
    delay(200);
    digitalWrite(spkPin, LOW);
  }

  // 3. Lógica de TEMPERATURA 1 SEMANA
  // Ahora, aunque haya gas, el Arduino llegará a leer esta parte:
  if(temperatura >= 36 && temperatura < 38 ) {
    lcd.setCursor(0, 0);
    lcd.print("T: "); lcd.print(temperatura); lcd.print(" TEMP Normal");
    digitalWrite(ledRed,LOW);


  } 
   else if (temperatura >38){
    lcd.setCursor(0,0);
    lcd.print("!TEMP ALTA!");
    digitalWrite(ledRed,HIGH);
    delay(a);
    digitalWrite(ledRed,LOW);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("T: "); lcd.print(temperatura); lcd.print("!TEMP BAJA!");
    digitalWrite(ledRed,HIGH);
    delay(a);
    digitalWrite(ledRed,LOW);





  }
  
  // 4 . Logica de Humedad
  
    if(humedad > hum) {
    lcd.setCursor(0, 1);
    lcd.print("H: "); lcd.print(humedad); lcd.print(" !ALTA!   ");
   
    }else if(humedad<40){
      lcd.setCursor(0,1);
      lcd.print("H: "); 
      lcd.print(humedad);
      lcd.print("!HUM BAJA!");
      digitalWrite(ledRed,HIGH);
    delay(a);
    digitalWrite(ledRed,LOW);
      
      } 
    
    else{
      lcd.setCursor(0,1);
      lcd.print("H:"); lcd.print(humedad);lcd.print("% Normal");
      digitalWrite(ledRed,LOW);
      }
  
  delay(dt);
}
