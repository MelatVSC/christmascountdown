# 🧪 Test di Funzionamento - Giorno 1

## 🎯 Obiettivi Test

Verificare che tutti i componenti funzionino correttamente prima di procedere con l'integrazione completa.

## 📋 Test Preliminari

### ✅ Test 1: Verifica Alimentazione

**Obiettivo:** Verificare che l'ESP32 si avvii correttamente

**Procedura:**
1. Collega solo l'alimentazione (VIN/GND)
2. Apri Arduino IDE → Tools → Serial Monitor (115200 baud)
3. Inserisci le batterie e accendi il portabatterie

**Risultato atteso:**
```
🎄 Christmas Countdown - Avvio...
📡 Connessione WiFi...
⚠️ WiFi non configurato - Usando orario interno
✅ Setup completato!
```

**❌ Troubleshooting:**
- Se non appare nulla: Verifica collegamenti alimentazione
- Se caratteri strani: Controlla baud rate (115200)
- Se riavvi continui: Verifica cortocircuiti

---

### ✅ Test 2: Display OLED

**Obiettivo:** Verificare che il display OLED funzioni

**Procedura:**
1. Collega il display secondo lo schema
2. Carica il codice completo
3. Verifica che appaia la schermata iniziale

**Risultato atteso:**
- Display acceso con "🎄 Christmas Countdown"
- Messaggio "Inizializzazione..."
- Dopo pochi secondi il display si spegne

**❌ Troubleshooting:**
- Display nero: Verifica collegamenti I2C (SDA/SCL)
- Caratteri strani: Controlla indirizzo I2C (0x3C)
- Non si accende: Verifica alimentazione 3.3V

---

### ✅ Test 3: Pulsante Momentaneo

**Obiettivo:** Verificare controllo display tramite pulsante

**Procedura:**
1. Collega il pulsante al pin 25
2. Premi il pulsante
3. Osserva il comportamento del display

**Risultato atteso:**
- Display si accende quando premi il pulsante
- Mostra il countdown dei giorni a Natale
- Dopo 30 secondi si spegne automaticamente

**Monitor seriale:**
```
🔘 Pulsante premuto - Display acceso
⏰ Timeout display - Spegnimento
```

**❌ Troubleshooting:**
- Pulsante non risponde: Verifica collegamento a GND
- Display non si spegne: Controlla timeout nel codice
- Attivazione accidentale: Verifica pull-up resistor

---

### ✅ Test 4: Interruttore a Slitta

**Obiettivo:** Verificare modalità "sempre acceso"

**Procedura:**
1. Collega l'interruttore al pin 26
2. Attiva l'interruttore (posizione ON)
3. Verifica che il display resti acceso

**Risultato atteso:**
- Display si accende quando attivi l'interruttore
- Rimane acceso finché l'interruttore è ON
- Si spegne quando riporti l'interruttore su OFF

**Monitor seriale:**
```
🔄 Interruttore ON - Display sempre acceso
🔄 Interruttore OFF - Display spento
```

**❌ Troubleshooting:**
- Interruttore non riconosciuto: Verifica collegamenti
- Comportamento invertito: Cambia logica nel codice
- Instabilità: Aggiungi debouncing

---

### ✅ Test 5: Countdown Natalizio

**Obiettivo:** Verificare calcolo corretto dei giorni

**Procedura:**
1. Attiva il display (pulsante o interruttore)
2. Verifica il numero mostrato
3. Controlla che sia corretto per la data odierna

**Risultato atteso:**
- Numero corretto di giorni fino al 25 dicembre
- Formato chiaro e leggibile
- Indicazione modalità nella parte bassa

**Per testare casi speciali:**
```cpp
// Nel codice, modifica la funzione setTime:
setTime(2025, 12, 25, 12, 0, 0); // Test "OGGI E' NATALE!"
setTime(2025, 12, 26, 12, 0, 0); // Test "Natale è passato"
```

**❌ Troubleshooting:**
- Numero errato: Verifica sincronizzazione orario
- Display tagliato: Controlla dimensioni font
- Crash del sistema: Verifica gestione date

---

## 🔬 Test Avanzati

### ✅ Test 6: Consumo Energetico

**Obiettivo:** Verificare il risparmio energetico

**Procedura:**
1. Misura corrente con display acceso
2. Misura corrente con display spento
3. Testa la durata delle batterie

**Strumenti necessari:**
- Multimetro con funzione amperometro
- Batterie fresche

**Risultati attesi:**
- Display ON: ~50-80mA
- Display OFF: ~10-20mA
- Autonomia stimata: 48-72 ore con display spento

---

### ✅ Test 7: Stabilità e Affidabilità

**Obiettivo:** Verificare funzionamento prolungato

**Procedura:**
1. Lascia il sistema acceso per 1 ora
2. Prova tutti i controlli ripetutamente
3. Verifica che non ci siano crash

**Risultato atteso:**
- Nessun riavvio spontaneo
- Risposta costante ai comandi
- Countdown che si aggiorna correttamente

---

## 📊 Checklist Finale

### ✅ Hardware:
- [ ] ESP32 si avvia correttamente
- [ ] Display OLED funziona
- [ ] Pulsante risponde
- [ ] Interruttore funziona
- [ ] Alimentazione stabile
- [ ] Nessun cortocircuito

### ✅ Software:
- [ ] Codice si carica senza errori
- [ ] Serial Monitor mostra messaggi corretti
- [ ] Countdown calcola giorni giusti
- [ ] Display si accende/spegne correttamente
- [ ] Timeout funziona
- [ ] Modalità interruttore OK

### ✅ Funzionalità:
- [ ] Pressione pulsante → Display ON per 30s
- [ ] Interruttore ON → Display sempre acceso
- [ ] Interruttore OFF → Torna a modalità pulsante
- [ ] Countdown mostra giorni corretti
- [ ] Indicatore modalità visibile
- [ ] Casi speciali gestiti (Natale, post-Natale)

## 🎯 Prossimi Passi

Se tutti i test sono superati:

1. **✅ Sistema base funzionante!**
2. **📝 Annota eventuali problemi** per miglioramenti futuri
3. **🔋 Ottimizza consumi** se necessario
4. **📦 Prepara componenti** per domani (RTC + Buzzer)

## 🔧 Modifiche Consigliate

### Per migliorare l'affidabilità:
```cpp
// Aggiungi debouncing ai pulsanti
#define DEBOUNCE_DELAY 50

// Aggiungi controllo errori I2C
if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Errore display - Tentativo recupero...");
    delay(1000);
    // Retry logic
}
```

### Per debug avanzato:
```cpp
// Aggiungi più informazioni di debug
Serial.printf("Batteria: %.2fV\n", analogRead(A0) * 3.3/4095 * 2);
Serial.printf("Memoria libera: %d bytes\n", ESP.getFreeHeap());
```

## 📝 Note per Domani

Preparare per l'integrazione:
- **RTC DS3231**: Pin SDA/SCL già utilizzati (I2C)
- **Buzzer KY-012**: Useremo pin GPIO 32
- **Melodie natalizie**: Preparare array di note
- **Sincronizzazione**: RTC + suono + display

Il sistema base è pronto per l'espansione! 🎄
