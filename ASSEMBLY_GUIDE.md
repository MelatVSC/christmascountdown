# 🔧 Guida Assemblaggio - Giorno 1

## 📋 Checklist Componenti

Prima di iniziare, assicurati di avere:
- [ ] ESP32 DevKit v1
- [ ] Display OLED I2C 0.96" (128x64)
- [ ] Pulsante momentaneo
- [ ] Interruttore a slitta
- [ ] Breadboard (almeno 400 tie-points)
- [ ] Cavi jumper maschio-maschio (almeno 10 pezzi)
- [ ] Portabatterie 3xAA con interruttore
- [ ] 3 batterie AA

## 🔌 Passo 1: Preparazione Breadboard

```
    A B C D E   F G H I J
  ┌─────────────────────────┐
1 │ + + + + +   + + + + + │  ← Rail positivo (+)
2 │ - - - - -   - - - - - │  ← Rail negativo (-)
3 │                       │
4 │ ○ ○ ○ ○ ○   ○ ○ ○ ○ ○ │
5 │ ○ ○ ○ ○ ○   ○ ○ ○ ○ ○ │
  │        ...             │
  └─────────────────────────┘
```

1. Posiziona la breadboard con i rail di alimentazione in alto
2. Collega il rail positivo (+) al pin **3.3V** dell'ESP32
3. Collega il rail negativo (-) al pin **GND** dell'ESP32

## 🔌 Passo 2: Montaggio ESP32

```
ESP32 DevKit v1 - Posizionamento sulla breadboard:

     3.3V  GND  G22  G21  G19  G18  G5   G17  G16  G4   G0   G2   G15  G8   G7   G6
      │    │    │    │    │    │    │    │    │    │    │    │    │    │    │    │
    ┌─○────○────○────○────○────○────○────○────○────○────○────○────○────○────○────○─┐
    │                                                                              │
    │                        ESP32 DevKit v1                                      │
    │                                                                              │
    └─○────○────○────○────○────○────○────○────○────○────○────○────○────○────○────○─┘
      │    │    │    │    │    │    │    │    │    │    │    │    │    │    │    │
     VIN  GND  G13  G12  G14  G27  G26  G25  G33  G32  G35  G34  VN   VP   EN   RST
```

1. Inserisci l'ESP32 sulla breadboard a cavallo del canale centrale
2. Assicurati che i pin siano ben inseriti nei fori
3. Verifica che non ci siano cortocircuiti

## 🔌 Passo 3: Collegamento Display OLED

**Display OLED pinout:**
```
OLED Module:
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

**Collegamenti:**
1. **VCC (Display)** → **3.3V** (ESP32) - Cavo rosso
2. **GND (Display)** → **GND** (ESP32) - Cavo nero
3. **SCL (Display)** → **GPIO 22** (ESP32) - Cavo giallo
4. **SDA (Display)** → **GPIO 21** (ESP32) - Cavo blu

## 🔌 Passo 4: Collegamento Pulsante

**Pulsante momentaneo:**
```
     GPIO 25
        │
        │
    ┌───┴───┐
    │       │
    │   ●   │ ← Pulsante
    │   │   │
    └───┼───┘
        │
       GND
```

**Collegamenti:**
1. Un terminale del pulsante → **GPIO 25** (ESP32)
2. L'altro terminale del pulsante → **GND** (ESP32)

## 🔌 Passo 5: Collegamento Interruttore

**Interruttore a slitta:**
```
     GPIO 26
        │
        │
    ┌───┴───┐
    │  ─╱─  │ ← Interruttore
    │   │   │
    └───┼───┘
        │
       GND
```

**Collegamenti:**
1. Pin centrale dell'interruttore → **GPIO 26** (ESP32)
2. Un pin laterale dell'interruttore → **GND** (ESP32)

## 🔌 Passo 6: Collegamento Alimentazione

**Portabatterie 3xAA:**
```
Portabatterie → ESP32
    +4.5V    →  VIN
    GND      →  GND
```

**Collegamenti:**
1. Filo rosso (+) del portabatterie → **VIN** (ESP32)
2. Filo nero (-) del portabatterie → **GND** (ESP32)

## 🔌 Passo 7: Controllo Alimentazione Display (Opzionale)

Per un controllo più preciso dell'alimentazione:

```
GPIO 27 → Transistor → Display VCC
```

**Implementazione semplificata:**
1. **GPIO 27** → Direttamente a un LED per test
2. Quando il display deve essere "spento", il pin va LOW
3. Quando il display deve essere "acceso", il pin va HIGH

## 🔍 Verifica Collegamenti

### Schema Finale:
```
ESP32 DevKit v1:
├── 3.3V → OLED VCC (rosso)
├── GND → OLED GND (nero)
├── GPIO 21 → OLED SDA (blu)
├── GPIO 22 → OLED SCL (giallo)
├── GPIO 25 → Pulsante → GND (verde)
├── GPIO 26 → Interruttore → GND (bianco)
├── GPIO 27 → LED test (arancione)
├── VIN → Batterie + (rosso)
└── GND → Batterie - (nero)
```

## ✅ Test di Continuità

Prima di alimentare il circuito:

1. **Verifica alimentazione:**
   - Non ci devono essere cortocircuiti tra VIN e GND
   - La tensione delle batterie deve essere ~4.5V

2. **Verifica I2C:**
   - SDA e SCL non devono essere cortocircuitati
   - Devono avere continuità con i rispettivi pin ESP32

3. **Verifica input:**
   - I pulsanti devono essere collegati correttamente
   - Non ci devono essere cortocircuiti sui pin GPIO

## 🚀 Primo Avvio

1. **Installa le librerie** (vedi README.md)
2. **Carica il codice** sull'ESP32
3. **Apri il Serial Monitor** (115200 baud)
4. **Inserisci le batterie** e accendi il portabatterie
5. **Verifica i messaggi** di debug

### Messaggi attesi:
```
🎄 Christmas Countdown - Avvio...
📡 Connessione WiFi...
⚠️ WiFi non configurato - Usando orario interno
✅ Setup completato!
```

## 🔧 Prossimi Passi

Se tutto funziona correttamente:
1. Prova il pulsante (deve accendere il display)
2. Prova l'interruttore (deve mantenere il display acceso)
3. Verifica il countdown dei giorni a Natale
4. Preparati per aggiungere RTC e buzzer domani!

## 🆘 Problemi Comuni

### Display non si accende:
- Verifica i collegamenti I2C
- Controlla l'alimentazione 3.3V
- Verifica l'indirizzo I2C nel codice

### Pulsante non funziona:
- Verifica il collegamento a massa
- Controlla che il pin sia configurato correttamente

### ESP32 non si avvia:
- Verifica l'alimentazione VIN/GND
- Controlla che non ci siano cortocircuiti
- Prova a resettare premendo il pulsante EN
