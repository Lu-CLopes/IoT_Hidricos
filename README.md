# 💧 IoT - Monitoramento de Qualidade de Recursos Hídricos

Projeto da disciplina **Sistemas Embarcados e IoT** — PUC-Campinas, Engenharia de Computação, 7º Período, 1º Semestre 2026.

## 👥 Grupo
- Luiza Lopes — 23013823
- Maria Paula Manzini — 23003063
- Rafael Ramos — 23001236
- Pedro Castro — 23003429

---

## 📌 Sobre o Projeto

Sistema de monitoramento contínuo da qualidade da água utilizando ESP32 e sensores de baixo custo, com transmissão de dados em tempo real para a nuvem via protocolo MQTT e visualização em dashboard no ThingsBoard.

O sistema coleta dados a cada 500ms, calcula a média aritmética de 1 minuto (~120 amostras) e publica os resultados automaticamente na plataforma IoT.

---

## 🏗️ Arquitetura

```
[Sensores] → [ADS1115 I²C] → [ESP32] → [Wi-Fi] → [MQTT] → [ThingsBoard Cloud]
```

### Componentes de hardware
| Componente | Função |
|---|---|
| ESP32 (uPesy WROOM) | Microcontrolador principal, Wi-Fi, processamento |
| ADS1115 | Conversor analógico-digital 16-bit via I²C |
| PH-4502C + eletrodo BNC | Leitura de pH (canal A2) |
| Sensor de turbidez SEN0189 | Leitura de turbidez em NTU (canal A1) |
| Sensor TDS/EC | Leitura de condutividade elétrica em ppm (canal A0) |

### Dados publicados no ThingsBoard
A cada 1 minuto o ESP32 publica no tópico `v1/devices/me/telemetry`:
```json
{
  "tds_ppm": 114.5,
  "turbidity_ntu": 2805.0,
  "ph": 7.021,
  "samples": 120
}
```

---

## 🗂️ Estrutura do Repositório

```
IoT_Hidricos/
├── src/
│   ├── main.cpp          # Código principal do ESP32
│   ├── config.h          # Credenciais (não versionado)
│   └── config.h.example  # Modelo de configuração
├── platformio.ini         # Configuração do projeto PlatformIO
├── .gitignore
└── README.md
```

---

## ⚙️ Como Rodar

### Pré-requisitos
- [VS Code](https://code.visualstudio.com/) com extensão [PlatformIO](https://platformio.org/)
- ESP32 conectado via USB
- Conta no [ThingsBoard Cloud](https://thingsboard.cloud)

### 1. Clonar o repositório
```bash
git clone https://github.com/SEU_USER/IoT_Hidricos.git
cd IoT_Hidricos
```

### 2. Configurar credenciais
```bash
cp src/config.h.example src/config.h
```

Edite o `src/config.h` com seus dados:
```cpp
#define WIFI_SSID     "sua_rede_wifi"
#define WIFI_PASSWORD "sua_senha_wifi"
#define TB_TOKEN      "seu_token_thingsboard"
```

### 3. Upload para o ESP32
1. Feche o Serial Monitor se estiver aberto
2. Clique em **→ (Upload)** no PlatformIO
3. Reabra o Serial Monitor (115200 baud)
4. Pressione **EN** no ESP32

### 4. Output esperado no Serial Monitor
```
[WiFi] Conectando a MinhaRede.....
[WiFi] Conectado! IP: 192.168.0.59
[MQTT] Conectando ao ThingsBoard... conectado!
[OK] Iniciado. Coletando amostras...
[500ms] TDS=114 ppm | Turb=2810 NTU | pH=7.021
[1000ms] TDS=116 ppm | Turb=2808 NTU | pH=7.018
...
=== MEDIAS (120 amostras) ===
  TDS:      115.1 ppm
  Turbidez: 2809.0 NTU
  pH:       7.019
[MQTT] OK -> {"tds_ppm":115.1,"turbidity_ntu":2809.0,"ph":7.019,"samples":120}
```

---

## 🌡️ Calibração dos Sensores

### pH (PH-4502C)
Calibração por dois pontos definida em `config.h`:
```cpp
#define PH_V7  2.2820f  // tensão lida para pH 7 (água neutra)
#define PH_V9  2.1753f  // tensão lida para pH 9 (ou solução ácida conhecida)
#define PH_PHn 9.0f     // valor de pH da segunda solução
```
Fórmula aplicada no firmware:
```
C   = (V9×1.5 - V7×1.5) / (7.0 - pHn)
pH  = 7.0 - ((V×1.5 - V7×1.5) / C)
```

### Turbidez
Calibração relativa: 0V ≈ opaco (0 NTU), 3.3V ≈ límpido (3000 NTU).

### TDS / Condutividade
Escala linear: `TDS = V × 1000 / 2.3` (ajuste conforme tensão de alimentação).

---

## ☁️ Configuração do ThingsBoard

### Device
- Nome: `esp32-water-monitor`
- Credencial: Access Token

### Dashboard — o que criar
| Widget | Tipo | Key |
|---|---|---|
| pH | Gauge (0–14) | `ph` |
| Turbidez | Gauge (0–3000) | `turbidity_ntu` |
| TDS | Value card | `tds_ppm` |
| Histórico | Time-series chart | `ph`, `turbidity_ntu`, `tds_ppm` |

Para cada widget: **Datasource → Device → esp32-water-monitor → key = nome acima**

### Alarmes recomendados
| Parâmetro | Condição | Severidade |
|---|---|---|
| pH | < 6.0 ou > 9.0 | Warning |
| Turbidez | > 2500 NTU | Warning |

---

## 📦 Dependências

```ini
lib_deps =
    adafruit/Adafruit ADS1X15@^2.5.0
    knolleary/PubSubClient@^2.8
    bblanchon/ArduinoJson@^7.0.0
```

---
- [Random Nerd Tutorials — ESP32](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- [Espressif ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ThingsBoard MQTT API](https://thingsboard.io/docs/reference/mqtt-api/)
- [Eletrogate — Sensor PH-4502C com ESP32](https://blog.eletrogate.com)
