#  EcoSmart – Simulación IoT

> Simulación Wokwi de un sistema de monitoreo de nivel de llenado en contenedores de reciclaje (tachos) usando un **ESP32** y **5 sensores ultrasónicos HC-SR04**. Los datos se envían vía HTTPS a un backend en Render.

---

##  Componentes

| Componente                  | Cantidad | Descripción                                  |
|-----------------------------|----------|----------------------------------------------|
| ESP32 DevKit V1             | 1        | Microcontrolador con WiFi                    |
| HC-SR04 (ultrasónico)       | 5        | Sensor de distancia para nivel de llenado    |
| Resistor 1 kΩ               | 5        | Divisor de tensión (ECHO → 3.3V)             |
| Resistor 2 kΩ               | 5        | Divisor de tensión (GND)                     |

##  Mapa de pines

| Sensor (HC‑SR04) | TRIG | ECHO | Tacho ID |
|------------------|------|------|----------|
| Ultrasonic #1    | D4   | D2   | 1        |
| Ultrasonic #2    | D13  | D12  | 2        |
| Ultrasonic #3    | D14  | D27  | 3        |
| Ultrasonic #4    | D32  | D33  | 4        |
| Ultrasonic #5    | D25  | D26  | 5        |

###  Divisor de tensión (ECHO → 3.3V)

Cada pin ECHO de los HC-SR04 (5V) se conecta al ESP32 (3.3V tolerante) mediante un divisor resistivo:

```
HC-SR04 ECHO ──┬── R1 (1 kΩ) ──┬── ESP32 GPIO (3.3V)
                │                │
                └── R2 (2 kΩ) ──┴── GND
```

- Vout = 5V × (2k / (1k + 2k)) ≈ 3.33V ✅

##  Arquitectura del firmware

```
┌─────────────────────────────────────────────────────────┐
│                       loop()                            │
│  ┌──────────┐   ┌──────────┐   ┌──────┐   ┌─────────┐ │
│  │ Sensor 1 │   │ Sensor 2 │ … │ Med  │   │ Enviar  │ │
│  │ (3 lect.)│ → │ (3 lect.)│ → │iana  │ → │ JSON vía│ │
│  │          │   │          │   │filtro│   │ HTTPS   │ │
│  └──────────┘   └──────────┘   └──────┘   └─────────┘ │
│  ┌──────────┐                                         │
│  │ Sensor 5 │                                         │
│  └──────────┘                                         │
└─────────────────────────────────────────────────────────┘
```

###  Flujo de operación

1. **Setup** → Configura pines, conecta WiFi (`Wokwi-GUEST`)
2. **Lectura** → Para cada sensor toma **7 muestras** con 20 ms de separación
3. **Filtro** → Ordena las muestras válidas y selecciona la **mediana** (elimina outliers)
4. **JSON** → Construye un payload con los 5 tachos
5. **POST** → Envía vía HTTPS (TLS inseguro para demo) al backend
6. **Espera** → 20 segundos y repite

###  Formato del JSON enviado

```json
{
  "lecturas": [
    { "tacho_id": 1, "distancia": 42 },
    { "tacho_id": 2, "distancia": 88 },
    { "tacho_id": 3, "distancia": 15 },
    { "tacho_id": 4, "distancia": 103 },
    { "tacho_id": 5, "distancia": -1 }
  ]
}
```

> `distancia` = `-1` indica error de lectura (sin eco).  
> La distancia se redondea al entero superior (`ceil`).

## 🛠️ Tecnologías

| Capa       | Herramienta                                          |
|------------|------------------------------------------------------|
| Firmware   | Arduino Framework (C++17)                            |
| IDE        | PlatformIO + VS Code                                 |
| Simulador  | [Wokwi](https://wokwi.com)                           |
| Red        | WiFi + HTTPS (WiFiClientSecure)                      |
| JSON       | [ArduinoJson](https://arduinojson.org) v7            |
| Backend    | [Render](https://render.com) — `ecosmart-backend-mufu` |
| GPIO       | 5× HC-SR04 con divisores resistivos 1kΩ / 2kΩ       |

## Cómo ejecutar

### Opción 1 — Wokwi Online (recomendado)

1. Abre el proyecto en [Wokwi](https://wokwi.com/projects/new/esp32)
2. Copia `diagram.json`, `wokwi.toml` y `src/main.cpp`
3. Presiona **Start Simulation**

### Opción 2 — PlatformIO local

```bash
# Clonar
git clone https://github.com/tu-usuario/Ecosmart-simulacion
cd Ecosmart-simulacion

# Compilar y subir
pio run --target upload

# Monitor serie
pio device monitor
```

## Estructura del proyecto

```
Ecosmart-simulacion/
├── .gitignore                 # Archivos ignorados
├── .pio/                      # Build cache y librerías (ignorado)
├── .vscode/                   # Configuración del IDE
├── diagram.json               # Circuito Wokwi
├── include/                   # Headers (reservado)
├── lib/                       # Librerías locales (reservado)
├── platformio.ini             # Configuración PlatformIO
├── src/
│   └── main.cpp               # Firmware principal
├── test/                      # Tests (reservado)
└── wokwi.toml                 # Configuración del simulador
```

## Backend

El firmware envía las lecturas a:  
`https://ecosmart-backend-mufu.onrender.com/api/mediciones`

## Licencia

MIT
