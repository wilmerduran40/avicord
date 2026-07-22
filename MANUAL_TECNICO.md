# MANUAL TÉCNICO - Sistema de Monitoreo de Codornices

## 1. Descripción del Sistema

Sistema de monitoreo y control automático de variables ambientales para crias de codornices, con interfaz web y alertas por Telegram.

## 2. Hardware Utilizado

### 2.1 Arduino Mega 2560
- **Microcontrolador**: ATmega2560 (16MHz, 256KB flash)
- **Sensores conectados**:
  - DHT11 (temperatura y humedad)
  - MQ-2 (detección de humo)
  - MQ-135 (detección de NH3/amoníaco)
- ** actuadores**:
  - Bombillo (calefacción) - Pin 24
  - Ventilador (refrigeración) - Pin 23
  - Buzzer - Pin 8
  - LEDs indicadores

### 2.2 ESP32
- **Microcontrolador**: Xtensa dual-core (240MHz, 4MB flash)
- **Conectividad**: WiFi 802.11 b/g/n
- **Funciones**: WebServer, Telegram Bot, TCP Cliente

### 2.3 Ethernet Shield W5100
- **Conexión**: Ethernet cable al router
- **IP asignada**: 192.168.0.120
- **Puerto**: 80 (HTTP)

## 3. Parametros Ambientales

| Variable | Mínimo | Máximo | Acción |
|---------|--------|--------|--------|
| Temperatura | 35°C | 38°C | Bombillo ON si <35°C |
| Humedad | 40% | 60% | Ventilador ON si >70% |
| NH3 | 0 ppm | 20 ppm | Ventilador ON si >20 ppm |
| Humo | 0% | 36% | Ventilador ON si >36% |

## 4. Comunicación

### 4.1 Mega → ESP32 (TCP)
- **Protocolo**: TCP socket
- **Puerto**: 8888
- **IP ESP32**: 192.168.0.1
- **Formato de datos**:
  ```
  DATOS:temperatura,humedad,humo,nh3,alertaGases,alertaClima
  ```
  Ejemplo: `DATOS:36.5,55,10,5,0,0`

### 4.2 ESP32 → Mega (TCP)
- **Protocolo**: TCP socket
- **Comandos**:
  - `ON` - Activar sistema
  - `OFF` - Desactivar sistema
  - `RST` - Reiniciar Mega

### 4.3 Web (Mega)
- **URL**: http://192.168.0.120
- **Endpoint datos**: http://192.168.0.120/datos
- **Formato**: texto plano separado por comas

## 5. Arquitectura del Sistema

```
+----------+         +----------+
| Sensores | -----> |   Mega   |
| DHT11   |       | Ethernet| -----> Router -----> Web (PC/Cel)
| MQ-2    |       |         |                |
| MQ-135  |       |  TCP    | +----> ESP32 -----> Telegram
+--------+       |  8888  |                |
                 +----------+                |
                                   +----+
                                   |Web |
                                   +----+
```

## 6. Código del Mega

### 6.1 Bibliotecas
```cpp
#include <DHT.h>           // Sensor de temperatura/humedad
#include <hd44780.h>      // LCD I2C
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <Ethernet.h>      // Ethernet shield
#include <SPI.h>         // Comunicación SPI
```

### 6.2 pines
- DHTPIN 22 (DHT11)
- bombillo 24
- ventilador 23
- A0 (MQ-2)
- A1 (MQ-135)

### 6.3 Funciones principales
- `setup()`: Inicializa Ethernet, sensores, LCD
- `loop()`: Lee sensores, toma decisiones, sirve web
- `procesarSolicitud()`: Maneja peticiones HTTP

### 6.4 Lógica de control
```
1. Leer sensores (DHT, MQ-2, MQ-135)
2. Verificar umbrales
3. Activar actuadores (bombillo/ventilador)
4. Mostrar en LCD
5. Enviar datos al ESP32
6. Atender peticiones web
```

## 7. Código del ESP32

### 7.1 Bibliotecas
```cpp
#include <WiFi.h>
#include <WebServer.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
```

### 7.2 Configuración WiFi
```cpp
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";
```

### 7.3 Telegram
- **Bot Token**: Del BotFather (@BotFather)
- **Chat ID**: Tu ID de Telegram

### 7.4 Comandos Telegram
- `/start` - Menú principal
- `/estado` - Ver estado actual
- `/activar` - Activar sistema
- `/desactivar` - Desactivar sistema
- `/alertas` - Ver historial de alertas
- `/ip` - Ver IP del servidor
- `/ayuda` - Ver ayuda

## 8. Página Web

### 8.1 Características
- Diseño responsivo (móvil y PC)
- Actualización automática cada 3 segundos
- Muestra: temperatura, humedad, NH3, humo
- Indicador de alerta

### 8.2 Tecnologías
- HTML5 + CSS3
- JavaScript (fetch API)
- Google Fonts (Segoe UI)

## 9. Conexiones Físicas

### 9.1 Mega
| Pin | Componente |
|-----|------------|
| 22 | DHT11 (DATA) |
| 23 | Ventilador |
| 24 | Bombillo |
| A0 | MQ-2 (AO) |
| A1 | MQ-135 (AO) |
| I2C | LCD (SDA, SCL) |

### 9.2 Ethernet
- Cable Ethernet al router
- IP reservada: 192.168.0.120

### 9.3 ESP32
- WiFi al router
- Serial1 pines 9/10 para comunicación (actualmente no usado)

## 10. Funcionamiento del Sistema

### 10.1 Inicio
1. Mega inicia Ethernet → obtiene IP
2. Servidor web comienza en puerto 80
3. Sensores se inicializan
4. LCD muestra datos

### 10.2 Ciclo principal (1 segundo)
1. Leer temperatura (DHT11)
2. Leer humedad (DHT11)
3. Leer humo (MQ-2, mapear a %)
4. Leer NH3 (MQ-135, mapear a ppm)
5. Evaluar alertas
6. Controlar actuadores
7. Actualizar LCD
8. Enviar datos al ESP32

### 10.3 Peticiones web
- GET `/` → Servir página HTML
- GET `/datos` → Servir datos en texto

## 11. Solución de Problemas

### 11.1 Sin acceso web
- Verificar conexión física Ethernet
- Verificar IP en router
- Confirmar que servidor inicié

### 11.2 Sensores no leen
- Verificar conexiones
- Verificar alimentación (5V)
- Probar sensores individualmente

### 11.3 LCD no muestra
- Verificar conexión I2C
- Ajustar contraste

### 11.4 ESP32 no recibe datos
- Verificar IP correcta
- Verificar puerto 8888
- Revisar firewall

## 12. Posibles Mejoras Futuras

1. Grabar datos en SD (log histórico)
2. Gráficos de temperatura en web
3. Notificaciones push
4. Control desde app móvil
5. Cámara para ver las codornices
6. alarmsSMS como respaldo
7.Base de datos externa
8. Integración con Alexa/Google Home

## 13. Créditos y Referencias

- Bibliotecas: DHT, hd44780, Ethernet, UniversalTelegramBot
- Sensores: DHT11, MQ-2, MQ-135
- Hardware: Arduino Mega 2560, ESP32, Ethernet Shield W5100