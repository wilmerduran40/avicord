#include <Wire.h>          // 1. Siempre primero para I2C (LCD)
#include <SPI.h>           // 2. Siempre primero para SPI (Ethernet/SD)
#include <DHT.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <Ethernet.h>

#define DHTPIN 22
#define DHTTYPE DHT11
#define bombillo 24
#define ventilador 23
#define humificador 25



byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(172, 16, 108, 51);
IPAddress esp32IP(172, 16, 108, 50);
const int esp32Port = 8888;

hd44780_I2Cexp lcd;
DHT dht(DHTPIN, DHTTYPE);

EthernetClient client;
EthernetServer server(80);
EthernetServer cmdServer(8888);

float temperatura;
float humedad;
int humo;
int nh3;
float voltaje;
int spkPin = 8;

const int humoNivel = A0;
const int nh3Nivel = A1;
int ledYellow = 2;
int ledGreen = 3;
int ledRed = 4;

float tempMin = 34;
float tempMax = 38.2;
float humMax = 60;
float humMin = 40;
float nh3Max=20;
float humoMax=36;
float tempV=36.6;

bool sistemaActivo = true;
bool alertaGases = false;
bool alertaClima = false;

char linea[100];
char url[50];


void setup() {
  Serial.begin(115200);
  Serial.println("Inicio...");

  Serial.print("ETH...");
  Ethernet.begin(mac, ip);
  delay(1000);
  Serial.println(Ethernet.localIP());

  server.begin();
  cmdServer.begin();
  Serial.println("Web OK, CMD OK");

  dht.begin();
  pinMode(humoNivel, INPUT);
  pinMode(nh3Nivel, INPUT);
  lcd.begin(20, 4);
  lcd.backlight();
  pinMode(spkPin, OUTPUT);
  pinMode(ledYellow, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(bombillo, OUTPUT);
  digitalWrite(bombillo, HIGH);
  pinMode(ventilador, OUTPUT);
  digitalWrite(ventilador, HIGH);
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    boolean currentLineIsBlank = true;
    boolean esGet = false;
    int pos = 0;
    linea[0] = '\0';
    url[0] = '\0';
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n' && currentLineIsBlank) {
          if (esGet) {
            if (pos > 0) linea[pos - 1] = '\0';
            procesarSolicitud(client, url);
          }
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
          linea[pos] = '\0';
          if (strncmp(linea, "GET ", 4) == 0) {
            esGet = true;
            int i, j = 0;
            for (i = 4; linea[i] != ' ' && i < 50; i++) {
              url[j++] = linea[i];
            }
            url[j] = '\0';
          }
          pos = 0;
        } else if (c != '\r') {
          currentLineIsBlank = false;
          if (pos < 99) linea[pos++] = c;
        }
      }
    }
    delay(1);
    client.stop();
  }

  EthernetClient cmdClient = cmdServer.available();
  if (cmdClient) {
    unsigned long cmdTimeout = millis() + 2000;
    while (cmdClient.connected() && millis() < cmdTimeout) {
      if (cmdClient.available()) {
        String cmd = cmdClient.readStringUntil('\n');
        cmd.trim();
        if (cmd == "ON") {
          sistemaActivo = true;
        } else if (cmd == "OFF") {
          sistemaActivo = false;
        } else if (cmd == "RST") {
          delay(100);
          asm volatile ("jmp 0");
        }
        cmdClient.stop();
        break;
      }
    }
  }

  if (!sistemaActivo) {
    delay(1000);
    return;
  }

  humedad = dht.readHumidity();
  temperatura = dht.readTemperature();
// Mapeo para calibrar los sensores MQ-135 y Mq-2
  int lecturaHumo = analogRead(A0);
  humo = map(lecturaHumo, 54, 1023, 0, 100);
  humo = constrain(humo, 0, 100);

  int lecturaNH3 = analogRead(A1);
  nh3 = map(lecturaNH3, 225, 1023, 0, 300);
  nh3 = constrain(nh3, 0, 300);

  voltaje = analogRead(A8);
  voltaje = (voltaje / 1023.0) * 5.0 * 1.1;
//Logica del Sistema donde se manejas las variables ambientales 
//aqui se establecio parametros para no usar valores fijos

  alertaGases = (humo > humoMax || nh3 > nh3Max);
  alertaClima = (temperatura > tempMax  || humedad > humMax );
  bool temperatura_minima = (temperatura < tempMin);

  //Aqui controlamos alerta de gases nh3 y humo para controlar el extractor y controlar las variables
  if (alertaGases || alertaClima ) {
    digitalWrite(ventilador, LOW);
  } else if(temperatura>=tempV) {
    digitalWrite(ventilador, HIGH);
  }else if (humedad > humMax || humedad < humMin){     //aqui lo que se hace es que cuando la humedad baje o suba a niveles anormales hace una accion
    digitalWrite(humificador,LOW);
  }
  else{
    digitalWrite(ventilador, HIGH);
    digitalWrite(humificador,HIGH);
  }

 //Aqui controlamos que acciones hara el sistema a la hora de recibir dichas variables

  if (temperatura < tempMin) {
    digitalWrite(ledGreen,LOW);
    digitalWrite(bombillo, LOW);
    digitalWrite(spkPin,HIGH);
    delay(600);
    digitalWrite(spkPin,LOW);
    digitalWrite(ledYellow,HIGH);
  } else if (temperatura > tempMax) {
    digitalWrite(ledGreen,LOW);
    digitalWrite(bombillo,HIGH);
    digitalWrite(spkPin,HIGH);
    delay(600);
    digitalWrite(spkPin,LOW);
    digitalWrite(ledRed,HIGH);

  }else{
    digitalWrite(ledRed,LOW);
    digitalWrite(ledYellow,LOW);
    digitalWrite(spkPin,LOW);
    digitalWrite(ledGreen,HIGH);
  }
lcd.setCursor(0, 0);
lcd.print("T:"); lcd.print(temperatura, 1);
lcd.print(" H:"); lcd.print(humedad, 0); lcd.print("% ");
lcd.print("Ven:");
lcd.print(digitalRead(ventilador) == LOW ? "ON " : "OFF");

lcd.setCursor(0, 1);
lcd.print(" Cal:");
lcd.print(digitalRead(bombillo) == LOW ? "ON " : "OFF");
lcd.print(" Hum:");
lcd.print(digitalRead(humificador) == LOW ? "ON " : "OFF");


  lcd.setCursor(0, 2);
  if (alertaGases) lcd.print("ALERTA GAS/HUMO!   ");
  else lcd.print("Aire Limpio");lcd.print(" Sis:");
lcd.print(sistemaActivo ? "ON" : "OFF");

  lcd.setCursor(0, 3);
  lcd.print("NH3:"); lcd.print(lecturaNH3); lcd.print(" ppm Humo:"); lcd.print(humo); lcd.print("%");

  EthernetClient espClient;
  if (espClient.connect(esp32IP, esp32Port)) {
    String linea = "DATOS:";
    linea += temperatura; linea += ",";
    linea += humedad; linea += ",";
    linea += humo; linea += ",";
    linea += nh3; linea += ",";
    linea += voltaje; linea += ",";
    linea += (alertaGases ? "1" : "0"); linea += ",";
    linea += (alertaClima ? "1" : "0");
    espClient.println(linea);
    espClient.flush();
    delay(500);
    espClient.stop();
  }

  delay(1000);
}

void procesarSolicitud(EthernetClient &client, char* url) {
  if (strcmp(url, "/datos") == 0) {
    String datos = "";
    datos += temperatura; datos += ",";
    datos += humedad; datos += ",";
    datos += humo; datos += ",";
    datos += nh3; datos += ",";
    datos += voltaje; datos += ",";
    datos += alertaGases ? "1" : "0"; datos += ",";
    datos += alertaClima ? "1" : "0";
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    client.print(datos);
    client.flush();
    delay(50);
    return;
  }

}