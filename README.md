# 🎄 Christmas Countdown ESP32 Project

## Descrizione del Progetto

Questo progetto natalizio utilizza un ESP32 per mostrare su un display OLED quanti giorni mancano a Natale. Il sistema include controlli tramite pulsante e interruttore per gestire l'accensione del display in modo intelligente, ottimizzando il consumo energetico.

## 🔧 Componenti Necessari

### Giorno 1 (Disponibili oggi):
- ✅ ESP32 DevKit v1
- ✅ Display OLED I2C 0.96" (128x64)
- ✅ Pulsante momentaneo
- ✅ Interruttore a slitta
- ✅ Breadboard e cavi jumper
- ✅ Portabatterie 3xAA (4.5V)

### Giorno 2 (Aggiungeremo domani):
- ⏳ Modulo RTC DS3231
- ⏳ Buzzer attivo KY-012

## 📋 Schema di Collegamento

```
ESP32 DevKit v1 Connections:
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  ESP32 Pin    │  Componente     │  Note                     │
│  ─────────────┼─────────────────┼─────────────────────────  │
│  GPIO 21      │  OLED SDA       │  I2C Data                 │
│  GPIO 22      │  OLED SCL       │  I2C Clock                │
│  3.3V         │  OLED VCC       │  Alimentazione display    │
│  GND          │  OLED GND       │  Massa                    │
│  ─────────────┼─────────────────┼─────────────────────────  │
│  GPIO 25      │  Pulsante       │  INPUT_PULLUP             │
│  GND          │  Pulsante       │  Massa                    │
│  ─────────────┼─────────────────┼─────────────────────────  │
│  GPIO 26      │  Interruttore   │  INPUT_PULLUP             │
│  GND          │  Interruttore   │  Massa                    │
│  ─────────────┼─────────────────┼─────────────────────────  │
│  GPIO 27      │  Display Power  │  Controllo alimentazione  │
│  ─────────────┼─────────────────┼─────────────────────────  │
│  VIN          │  Batterie +     │  4.5V (3xAA)             │
│  GND          │  Batterie -     │  Massa                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 🔌 Diagramma di Collegamento Dettagliato

```
         ESP32 DevKit v1
    ┌─────────────────────────┐
    │                         │
    │  3.3V ●─────────────────┼─── VCC (OLED)
    │  GND  ●─────────────────┼─── GND (OLED)
    │  G22  ●─────────────────┼─── SCL (OLED)
    │  G21  ●─────────────────┼─── SDA (OLED)
    │                         │
    │  G25  ●─────────────────┼─── Pulsante ─── GND
    │  G26  ●─────────────────┼─── Interruttore ─── GND
    │  G27  ●─────────────────┼─── (Display Power Control)
    │                         │
    │  VIN  ●─────────────────┼─── + Batterie (4.5V)
    │  GND  ●─────────────────┼─── - Batterie
    │                         │
    └─────────────────────────┘

Display OLED (128x64 I2C):
┌─────────────────┐
│  ●●●●            │
│  │││└─ GND       │
│  ││└── VCC       │
│  │└─── SCL       │
│  └──── SDA       │
│                 │
│ [Display Area]  │
│                 │
└─────────────────┘
```

## 🔄 Logica di Funzionamento

### Modalità Pulsante (Default):
1. **Stato iniziale**: Display spento per risparmiare energia
2. **Pressione pulsante**: Display si accende e mostra il countdown
3. **Timeout**: Dopo 30 secondi, display si spegne automaticamente

### Modalità Interruttore:
1. **Interruttore ON**: Display sempre acceso
2. **Interruttore OFF**: Ritorna alla modalità pulsante

### Visualizzazione:
- **Titolo**: "🎄 NATALE"
- **Countdown**: Giorni rimanenti in grande
- **Stato**: Indica se è in modalità "SEMPRE ACCESO" o "PULSANTE"
- **Casi speciali**: 
  - Se oggi è Natale: "OGGI E' NATALE!"
  - Se Natale è passato: Countdown per l'anno prossimo

## 📚 Librerie Necessarie

Installa queste librerie tramite Arduino IDE -> Sketch -> Include Library -> Manage Libraries:

```cpp
1. Adafruit GFX Library
2. Adafruit SSD1306
3. Wire (inclusa con ESP32)
4. WiFi (inclusa con ESP32)
5. Time (inclusa con ESP32)
```

## 🔧 Configurazione

### 1. WiFi (Opzionale):
```cpp
const char* ssid = "TuaReteWiFi";
const char* password = "TuaPassword";
```

### 2. Timeout Display:
```cpp
unsigned long displayTimeout = 30000; // 30 secondi
```

### 3. Pin Configuration:
```cpp
#define BUTTON_PIN 25
#define SWITCH_PIN 26
#define DISPLAY_POWER_PIN 27
```

## 🚀 Come Testare

### Test 1: Display OLED
1. Collega solo il display OLED
2. Carica il codice
3. Verifica che mostri "🎄 Christmas Countdown"

### Test 2: Pulsante
1. Collega il pulsante al pin 25
2. Premi il pulsante
3. Verifica che il display si accenda per 30 secondi

### Test 3: Interruttore
1. Collega l'interruttore al pin 26
2. Attiva l'interruttore
3. Verifica che il display resti sempre acceso

### Test 4: Countdown
1. Verifica che mostri i giorni corretti a Natale
2. Modifica la data nel codice per testare casi speciali

## 🔋 Ottimizzazioni Energetiche

- **Display off**: Spegnimento completo quando non in uso
- **Polling ottimizzato**: Delay di 100ms nel loop principale
- **Controllo alimentazione**: Pin dedicato per controllo display
- **Deep sleep**: Pronto per implementazione futura

## 🛠️ Prossimi Sviluppi (Giorno 2)

- ✅ Aggiunta modulo RTC DS3231 per orario preciso
- ✅ Implementazione buzzer per melodie natalizie
- ✅ Sincronizzazione melodia con eventi
- ✅ Modalità deep sleep avanzata

## 🐛 Troubleshooting

### Display non si accende:
- Verifica collegamenti I2C (SDA/SCL)
- Controlla indirizzo I2C (0x3C)
- Verifica alimentazione 3.3V

### Pulsante non funziona:
- Verifica collegamento a massa
- Controlla resistenza pull-up
- Verifica pin GPIO

### Countdown errato:
- Verifica sincronizzazione orario
- Controlla connessione WiFi
- Verifica impostazione fuso orario

## 📞 Supporto

Per domande o problemi, verifica:
1. Connessioni hardware
2. Versione librerie
3. Configurazione pin
4. Monitor seriale per debug