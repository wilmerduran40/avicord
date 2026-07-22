#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>


// ============ CONFIGURACIÓN WI-FI ============
const char* ssid = "OCULUS";
const char* password = "123456789";
IPAddress esp32IP(172, 16, 108, 50);
IPAddress gateway(172, 16, 108, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(8, 8, 8, 8);
IPAddress dns2(8, 8, 4, 4);

// ============ CONFIGURACIÓN TELEGRAM ============
#define BOT_TOKEN "7127237991:AAHEO6eR5e4yKR3h-RLqFqVblNtDqMNVT-I"
#define CHAT_ID "-1003870314474"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ============ WEBSERVER ============
WebServer server(80);

// ============ VARIABLES GLOBALES ============
String datosMega = "";
bool nuevaAlerta = false;
String mensajeAlerta = "";
bool avisoActivoEnviado = false;

unsigned long ultimaAlerta = 0;
const unsigned long intervaloAlertas = 60000;

unsigned long ultimoCheckTelegram = 0;
const unsigned long intervaloTelegram = 5000;
unsigned long ultimoEnvioWeb = 0;

// ============ TCP SERVER (para Mega) ============
WiFiServer tcpServer(8888);
WiFiClient tcpClient;
WiFiClient megaClient;
IPAddress megaIP(172, 16, 108, 51);
const int megaPort = 8888;

// ============ PAGINA WEB ============
String paginaWeb() {
  String html = R"RAWHTML(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Dashboard Industrial Codornices</title>
  <script src="https://cdn.tailwindcss.com"></script>
  <style>
    body {background-image:url("https://s3.animalia.bio/animals/photos/full/original/shutterstock-2202857131jpg.webp");background-size:cover;background-position:left;background-repeat: no-repeat;font-family: 'Segoe UI', system-ui, sans-serif; }
    .card { background: white; border-radius: 12px; box-shadow: 0 4px 6px -1px rgb(0 0 0 / 0.1); text-align: center;}.titulo{font-size: 30px;text-align: center;font-weight:bolder }
    .sensor-card { border-top: 4px solid #e2e8f0; transition: transform 0.2s; text-align:center; }
    .sensor-card:hover { transform: translateY(-2px); }.panel{color:white;text-align:center;font-size:30px;padding:20px }.box{background-image: url("https://hotmart.com/media/2018/10/BLOG_-Os-x-principais-desafios-da-inovacao-tecnologica-670x4192.png"); background-size:cover;background-position: center;}.botones{display:flex;justify-content:center;}.p{font-size: 20px;font-weight: bolder; color: white;text-align: center;}.value{border: 2px solid black; text-align: center; border-radius:5px;padding:20px;background-color: white;}
  </style>
</head>
<body class="p-4 md:p-6 text-slate-800">

  <div class="max-w-5xl mx-auto">
    <header class="flex justify-between items-center mb-8 border-b pb-4">
      <div>
        <h1 class="titulo">EMPRENDIMIENTO AVITEC</h1>
        <p class="p">Monitoreo en Tiempo Real</p>
      </div>
      <div id="status-badge" class="px-4 py-2 rounded-full font-semibold text-sm bg-slate-200">
        Conectando...
      </div>
    </header>

    <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-6 mb-8">
      <!-- Temp -->
      <div class="value"style="border-top-color: #FDFF76;border-top-width: 10px;">
        <h3 class="titulo">Temperatura</h3>
        <div id="temp" class="text-3xl font-extrabold text-slate-900">--°C</div>
      </div>
      <!-- Humedad -->
      <div class="value" style="border-top-color: #3b82f6;border-top-width: 10px;">
        <h3 class="titulo">Humedad</h3>
        <div id="hum" class="text-3xl font-extrabold text-slate-900">--%</div>
      </div>
      <!-- Amoniaco -->
      <div class="value" style="border-top-color: #84cc16;">
        <h3 class="titulo">NH3</h3>
        <div id="nh3" class="text-3xl font-extrabold text-slate-900">--ppm</div>
      </div>
      <!-- Humo -->
      <div class="value" style="border-top-color: #ef4444;">
        <h3 class="titulo">Humo</h3>
        <div id="humo" class="text-3xl font-extrabold text-slate-900">--%</div>
      </div>
    </div>

    <div class="box">
      <h2 class="panel">Panel de Control</h2>
      <div class="botones">
        <button onclick="control('ON')" class="bg-indigo-600 hover:bg-indigo-700 text-white px-5 py-2.5 rounded-lg font-medium transition shadow-sm">Activar Sistema</button>
        <button onclick="control('OFF')" class="bg-white border border-slate-300 hover:bg-slate-50 text-slate-700 px-5 py-2.5 rounded-lg font-medium transition shadow-sm">Desactivar</button>
        <button onclick="control('RST_ESP32')" class="bg-amber-500 hover:bg-amber-600 text-white px-5 py-2.5 rounded-lg font-medium transition shadow-sm">Reset ESP32</button>
        <button onclick="control('RST_MEGA')" class="bg-amber-500 hover:bg-amber-600 text-white px-5 py-2.5 rounded-lg font-medium transition shadow-sm">Reset Mega</button>
      </div>
    </div>

    <div id="alerta" class="mb-8"></div>

    <footer class="p">
      Última actualización: <span id="timestamp">--</span><p class="p">Pagina Desarrollada por  Duran y Blanco(2026)</p>
    </footer>
  </div>
  <script>
    function actualizar() {
      fetch('/datos')
        .then(function(r) { return r.json(); })
        .then(function(data) {
          if (data.temperatura !== undefined) {
            document.getElementById('temp').innerText = data.temperatura + '°C';
          }
          if (data.humedad !== undefined) {
            document.getElementById('hum').innerText = data.humedad + '%';
          }
          if (data.nh3 !== undefined) {
            document.getElementById('nh3').innerText = data.nh3 + ' ppm';
          }
          if (data.humo !== undefined) {
            document.getElementById('humo').innerText = data.humo + '%';
          }
          document.getElementById('alerta').innerHTML =
            (data.alertaGases === '1' || data.alertaClima === '1')
              ? '<div style="background:#ef4444;color:white;padding:10px;border-radius:8px;text-align:center;margin:10px 0;">ALERTA: Revisar condiciones ambientales</div>'
              : '<div style="background:#22c55e;color:white;padding:10px;border-radius:8px;text-align:center;margin:10px 0;">Condiciones normales</div>';
          document.getElementById('timestamp').innerText = new Date().toLocaleTimeString();
        })
        .catch(function() {
          document.getElementById('timestamp').innerText = 'Sin conexion al ESP32';
        });
    }

    function control(cmd) {
      fetch('/control?cmd=' + cmd).then(function() { actualizar(); });
    }

    setInterval(actualizar, 3000);
    actualizar();
  </script>
</body>
</html>
  )RAWHTML";
  return html;
}

//  RUTAS WEB 
void handleRoot() {
  server.send(200, "text/html", paginaWeb());
}

void handleDatos() {
  StaticJsonDocument<200> doc;
  doc["sistema"] = "1";
  Serial.println("DEBUG /datos solicitado, datosMega=[" + datosMega + "]");
  if (datosMega.length() > 0) {
    int pos[7];
    int idx = 0;
    String datos = datosMega;
    datos.replace("DATOS:", "");
    Serial.println("DEBUG datos parseados=[" + datos + "]");
    for (int i = 0; i < datos.length(); i++) {
      if (datos.charAt(i) == ',') {
        if (idx < 7) pos[idx++] = i;
      }
    }
    Serial.println("DEBUG comas encontradas: " + String(idx));
    if (idx >= 6) {
      doc["temperatura"] = datos.substring(0, pos[0]);
      doc["humedad"] = datos.substring(pos[0] + 1, pos[1]);
      doc["humo"] = datos.substring(pos[1] + 1, pos[2]);
      doc["nh3"] = datos.substring(pos[2] + 1, pos[3]);
      doc["alertaGases"] = datos.substring(pos[4] + 1, pos[5]);
      doc["alertaClima"] = datos.substring(pos[5] + 1);
    }
  }
  
  String respuesta;
  serializeJson(doc, respuesta);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", respuesta);
}

void handleControl() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    
    if (cmd == "RST_ESP32") {
      server.send(200, "text/plain", "Reiniciando ESP32...");
      delay(500);
      ESP.restart();
    } else if (cmd == "RST_MEGA") {
      if (megaClient.connect(megaIP, megaPort)) {
        megaClient.print("RST\n");
        megaClient.stop();
      }
      bot.sendMessage(CHAT_ID, "Reiniciando Arduino Mega...", "");
      server.send(200, "text/plain", "Reiniciando Mega...");
    } else {
      if (megaClient.connect(megaIP, megaPort)) {
        megaClient.print(cmd + "\n");
        megaClient.stop();
      }
      if (cmd == "ON") {
        bot.sendMessage(CHAT_ID, "Sistema reactivado manualmente", "");
      } else if (cmd == "OFF") {
        bot.sendMessage(CHAT_ID, "Sistema desactivado manualmente", "");
      }
      server.send(200, "text/plain", "OK");
    }
  } else {
    server.send(400, "text/plain", "ERROR");
  }
}

// BOT-TELEGRAM 
void handleTelegram() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    
    if (from_name == "") from_name = "Usuario";
    
    if (text == "/start" || text == "/menu") {
      String menu = " *Sistema Avitec*\n\n";
      menu += "_Selecciona una opción:_\n\n";
      menu += "/estado - Ver estado actual\n";
      menu += "/activar - Activar sistema\n";
      menu += "/desactivar - Desactivar sistema\n";
      menu += "/alertas - Ver historial de alertas\n";
      menu += "/ayuda - Ver ayuda";
      bot.sendMessage(chat_id, menu, "Markdown");
    }
    else if (text == "/estado") {
      String estado = "*Estado Actual*\n\n";
      if (datosMega.length() > 0) {
        String d = datosMega;
        d.replace("DATOS:", "");
        int p[7], n = 0;
        for (int i = 0; i < d.length(); i++) {
          if (d.charAt(i) == ',') { if (n < 7) p[n++] = i; }
        }
        if (n >= 5) {
          estado += "Temperatura: " + d.substring(0, p[0]) + "°C\n";
          estado += "Humedad: " + d.substring(p[0] + 1, p[1]) + "%\n";
          estado += "Humo: " + d.substring(p[1] + 1, p[2]) + "%\n";
          estado += "NH3: " + d.substring(p[2] + 1, p[3]) + " ppm\n";
          estado += "Voltaje: " + d.substring(p[3] + 1, p[4]) + "V\n";
        }
        estado += "\n\nWeb: http://" + WiFi.localIP().toString();
      } else {
        estado += " _Esperando datos del Arduino..._";
      }
      bot.sendMessage(chat_id, estado, "Markdown");
    }
    else if (text == "/activar") {
      if (megaClient.connect(megaIP, megaPort)) {
        megaClient.print("ON\n");
        megaClient.stop();
      }
      bot.sendMessage(chat_id, "✅ *Sistema activado*", "Markdown");
    }
    else if (text == "/desactivar") {
      if (megaClient.connect(megaIP, megaPort)) {
        megaClient.print("OFF\n");
        megaClient.stop();
      }
      bot.sendMessage(chat_id, "⛔ *Sistema desactivado*", "Markdown");
    }
    else if (text == "/alertas") {
      if (mensajeAlerta.length() > 0) {
        bot.sendMessage(chat_id, "📋 *Historial de Alertas*\n\n" + mensajeAlerta, "Markdown");
      } else {
        bot.sendMessage(chat_id, "📋 *Historial de Alertas*\n\n✅ No hay alertas registradas", "Markdown");
      }
    }
    else if (text == "/ayuda") {
      String ayuda = "📖 *Ayuda*\n\n";
      ayuda += "*Comandos disponibles:*\n";
      ayuda += "/start - Menú principal\n";
      ayuda += "/estado - Ver sensores\n";
      ayuda += "/activar - Encender sistema\n";
      ayuda += "/desactivar - Apagar sistema\n";
      ayuda += "/alertas - Ver alertas\n";
      ayuda += "/ip - Ver IP del servidor";
      bot.sendMessage(chat_id, ayuda, "Markdown");
    }
    else if (text == "/ip") {
      bot.sendMessage(chat_id, " *IP del servidor:*\n`" + WiFi.localIP().toString() + "`", "Markdown");
    }
    else {
      bot.sendMessage(chat_id, "Comando no reconocido. Usa /ayuda", "");
    }
    
    bot.last_message_received = bot.messages[i].update_id;
  }
}

// ============ ALERTAS ============
void verificarAlertas() {
  if (datosMega.length() == 0) return;
  
  if (datosMega.indexOf("DATOS:") == -1) return;
  
  String datos = datosMega;
  datos.replace("DATOS:", "");
  
  int pos[7];
  int idx = 0;
  for (int i = 0; i < datos.length(); i++) {
    if (datos.charAt(i) == ',') {
      if (idx < 7) pos[idx++] = i;
    }
  }
  
  if (idx >= 6) {
    String alertaGases = datos.substring(pos[4] + 1, pos[5]);
    String alertaClima = datos.substring(pos[5] + 1);
    String temp = datos.substring(0, pos[0]);
    String hum = datos.substring(pos[0] + 1, pos[1]);
    String humo = datos.substring(pos[1] + 1, pos[2]);
    String nh3 = datos.substring(pos[2] + 1, pos[3]);
    
    if (alertaGases == "1" || alertaClima == "1") {
      if (millis() - ultimaAlerta >= intervaloAlertas) {
        nuevaAlerta = true;
        ultimaAlerta = millis();
        String msg = " *ALERTA - Crias Codornices*\n\n";
        msg += " *Valores actuales:*\n";
        msg += " Temperatura: " + temp + "°C\n";
        msg += " Humedad: " + hum + "%\n";
        msg += " Humo: " + humo + "%\n";
        msg += " NH3: " + nh3 + " ppm\n";
        if (alertaClima == "1") {
          msg += "\n Clima fuera de rango";
        }
        if (alertaGases == "1") {
          msg += "\n Gases peligrosos";
        }
        msg += "\n\n http://" + WiFi.localIP().toString();
        
        bot.sendMessage(CHAT_ID, msg, "Markdown");
        mensajeAlerta = "🕐" + String(millis() / 60000) + "min\n" + msg + "\n\n" + mensajeAlerta;
        if (mensajeAlerta.length() > 1500) {
          mensajeAlerta = mensajeAlerta.substring(0, 1500);
          int lastBreak = mensajeAlerta.lastIndexOf("\n\n");
          if (lastBreak > 0) mensajeAlerta = mensajeAlerta.substring(0, lastBreak);
        }
      }
    } else {
      nuevaAlerta = false;
    }
  }
}

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  
  WiFi.config(esp32IP, gateway, subnet, dns1, dns2);
  WiFi.begin(ssid, password);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 40) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nERROR: No se pudo conectar a WiFi");
    return;
  }
  
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  tcpServer.begin();
  Serial.println("TCP server iniciado en puerto 8888");
  
  client.setInsecure();
  
  if (bot.sendMessage(CHAT_ID, "*Sistema ESP32 iniciado*\n\nWiFi: " + WiFi.localIP().toString() + "\nWeb: http://" + WiFi.localIP().toString() + "\nTCP:8888", "Markdown")) {
    Serial.println("Mensaje Telegram enviado OK");
  } else {
    Serial.println("ERROR: No se pudo enviar mensaje Telegram");
  }
  
  Serial.print("Probando DNS api.telegram.org: ");
  IPAddress telegramIP;
  if (WiFi.hostByName("api.telegram.org", telegramIP)) {
    Serial.println(telegramIP.toString());
  } else {
    Serial.println("FALLO DNS");
  }
  
  server.on("/", handleRoot);
  server.on("/datos", handleDatos);
  server.on("/control", handleControl);
  server.begin();
  
  Serial.println("ESP32_LISTO");
}

// ============ LOOP ============
void loop() {
  server.handleClient();
  
  tcpClient = tcpServer.available();
  if (tcpClient) {
    Serial.println("TCP client conectado desde: " + tcpClient.remoteIP().toString());
    delay(100);
    unsigned long tcpTimeout = millis() + 3000;
    while (tcpClient.connected() && millis() < tcpTimeout) {
      if (tcpClient.available()) {
        String incoming = tcpClient.readStringUntil('\n');
        incoming.trim();
        if (incoming.length() > 0) {
          Serial.println("TCP RAW: [" + incoming + "]");
        }
        if (incoming.startsWith("DATOS:")) {
          datosMega = incoming;
          Serial.println("Datos guardados: " + datosMega);
          if (!avisoActivoEnviado) {
            avisoActivoEnviado = true;
            if (bot.sendMessage(CHAT_ID, " *Sistema activo* - Recibiendo datos del Mega correctamente", "Markdown")) {
              Serial.println("Aviso activo enviado OK");
            } else {
              Serial.println("ERROR: No se pudo enviar aviso activo");
            }
          }
        }
        break;
      }
    }
    tcpClient.stop();
  }
  
  if (millis() - ultimoCheckTelegram >= intervaloTelegram) {
    handleTelegram();
    verificarAlertas();
    ultimoCheckTelegram = millis();
  }
}