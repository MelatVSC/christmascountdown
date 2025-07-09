/*
 * 🎄 Christmas Countdown - ESP32 Project
 * 
 * Progetto natalizio che mostra i giorni rimanenti a Natale
 * su un display OLED con controllo tramite pulsante e interruttore
 * 
 * Componenti utilizzati:
 * - ESP32 DevKit v1
 * - Display OLED I2C 0.96" (128x64)
 * - Pulsante momentaneo
 * - Interruttore a slitta
 * - Portabatterie 3xAA (4.5V)
 * 
 * Autore: [Il tuo nome]
 * Data: Luglio 2025
 */

#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <time.h>

// ============== CONFIGURAZIONE DISPLAY SH1106 ==============
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
bool displayAvailable = false; // Flag per verificare se display è disponibile

// ============== CONFIGURAZIONE PIN ==============
#define BUTTON_PIN 25        // Pin per il pulsante momentaneo
#define SWITCH_PIN 26        // Pin per l'interruttore a slitta
#define DISPLAY_POWER_PIN 27 // Pin per controllo alimentazione display (opzionale)

// ============== VARIABILI GLOBALI ==============
bool displayOn = false;           // Stato del display
bool lastButtonState = HIGH;      // Ultimo stato del pulsante
bool lastSwitchState = HIGH;      // Ultimo stato dell'interruttore (NC = HIGH quando aperto)
bool switchMode = true;           // Modalità interruttore attiva (default ON per NC)
unsigned long buttonPressTime = 0; // Timestamp pressione pulsante
unsigned long displayTimeout = 30000; // Timeout display (30 secondi)

// Configurazione WiFi per sincronizzazione orario
const char* ssid = ""; // Inserire il nome della rete WiFi
const char* password = ""; // Inserire la password WiFi

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  Serial.println("🎄 Christmas Countdown - Avvio...");
  
  // Inizializzazione pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(DISPLAY_POWER_PIN, OUTPUT);
  
  // Inizializzazione display
  Serial.println("🔍 Tentativo inizializzazione display SH1106...");
  u8g2.begin();
  displayAvailable = true;
  Serial.println("✅ Display SH1106 inizializzato correttamente!");
  
  // Configurazione iniziale display
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 15, "Christmas Countdown");
  u8g2.drawStr(0, 30, "Inizializzazione...");
  u8g2.sendBuffer();
  
  Serial.println("📺 Messaggio di avvio mostrato sul display");
  delay(2000); // Mostra il messaggio per 2 secondi
  
  // Connessione WiFi per sincronizzazione orario
  setupWiFiTime();
  
  // Controllo iniziale interruttore e accensione display
  bool currentSwitchState = digitalRead(SWITCH_PIN);
  if (currentSwitchState == HIGH) {
    // Interruttore aperto (NC) = Display sempre acceso
    switchMode = true;
    turnOnDisplay();
    Serial.println("🔄 Interruttore NC aperto - Display sempre acceso");
    
    // Test display semplice
    if(displayAvailable) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_10x20_tr);
      u8g2.drawStr(20, 20, "NATALE");
      
      u8g2.setFont(u8g2_font_fub25_tn);
      u8g2.drawStr(40, 50, "168");
      
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(10, 63, "giorni rimanenti");
      
      u8g2.sendBuffer();
      Serial.println("📺 Test display SH1106 fisso mostrato");
    }
  } else {
    // Interruttore chiuso (NC) = Modalità pulsante
    switchMode = false;
    turnOffDisplay();
    Serial.println("🔄 Interruttore NC chiuso - Modalità pulsante");
  }
  
  Serial.println("✅ Setup completato!");
}

// ============== LOOP PRINCIPALE ==============
void loop() {
  // Lettura stati pulsante e interruttore
  bool currentButtonState = digitalRead(BUTTON_PIN);
  bool currentSwitchState = digitalRead(SWITCH_PIN);
  
  // Gestione interruttore
  handleSwitch(currentSwitchState);
  
  // Gestione pulsante (solo se interruttore è OFF)
  if (!switchMode) {
    handleButton(currentButtonState);
  }
  
  // Gestione timeout display
  handleDisplayTimeout();
  
  // Aggiornamento display se acceso
  if (displayOn) {
    updateDisplay();
  }
  
  // Aggiornamento stati precedenti
  lastButtonState = currentButtonState;
  lastSwitchState = currentSwitchState;
  
  delay(2000); // Pausa più lunga per evitare spam
}

// ============== GESTIONE INTERRUTTORE ==============
void handleSwitch(bool currentState) {
  // Interruttore NC: HIGH = aperto (display sempre acceso), LOW = chiuso (modalità pulsante)
  
  // Interruttore cambiato da chiuso a aperto (OFF a ON)
  if (currentState == HIGH && lastSwitchState == LOW) {
    switchMode = true;
    turnOnDisplay();
    Serial.println("🔄 Interruttore NC aperto - Display sempre acceso");
  }
  
  // Interruttore cambiato da aperto a chiuso (ON a OFF)
  if (currentState == LOW && lastSwitchState == HIGH) {
    switchMode = false;
    turnOffDisplay();
    Serial.println("🔄 Interruttore NC chiuso - Modalità pulsante");
  }
}

// ============== GESTIONE PULSANTE ==============
void handleButton(bool currentState) {
  // Pulsante premuto (transizione HIGH -> LOW)
  if (currentState == LOW && lastButtonState == HIGH) {
    buttonPressTime = millis();
    turnOnDisplay();
    Serial.println("🔘 Pulsante premuto - Display acceso");
  }
}

// ============== GESTIONE TIMEOUT DISPLAY ==============
void handleDisplayTimeout() {
  // Timeout solo se non in modalità interruttore
  if (!switchMode && displayOn) {
    if (millis() - buttonPressTime > displayTimeout) {
      turnOffDisplay();
      Serial.println("⏰ Timeout display - Spegnimento");
    }
  }
}

// ============== ACCENSIONE DISPLAY ==============
void turnOnDisplay() {
  if (!displayOn) {
    digitalWrite(DISPLAY_POWER_PIN, HIGH);
    
    if(displayAvailable) {
      u8g2.setPowerSave(0); // Accendi display
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      Serial.println("🔆 Display fisico acceso");
    } else {
      Serial.println("🔆 Display logico acceso (fisico non disponibile)");
    }
    
    displayOn = true;
  }
}

// ============== SPEGNIMENTO DISPLAY ==============
void turnOffDisplay() {
  if (displayOn) {
    if(displayAvailable) {
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      u8g2.setPowerSave(1); // Spegni display
      Serial.println("🔅 Display fisico spento");
    } else {
      Serial.println("🔅 Display logico spento (fisico non disponibile)");
    }
    
    digitalWrite(DISPLAY_POWER_PIN, LOW);
    displayOn = false;
  }
}

// ============== AGGIORNAMENTO DISPLAY ==============
void updateDisplay() {
  // Calcolo giorni rimanenti per debug
  int daysToChristmas = calculateDaysToChristmas();
  
  // Debug sempre attivo
  Serial.print("📊 Countdown: ");
  Serial.print(daysToChristmas);
  Serial.println(" giorni a Natale");
  
  // Aggiorna display solo se disponibile
  if(!displayAvailable) {
    Serial.println("⚠️ Display non disponibile - Solo debug seriale");
    return;
  }
  
  // Inizia nuovo frame U8g2
  u8g2.clearBuffer();
  
  // Titolo principale
  u8g2.setFont(u8g2_font_10x20_tr);
  u8g2.drawStr(25, 18, "NATALE");
  
  // Visualizzazione giorni rimanenti
  if (daysToChristmas > 0) {
    // Numero grande per i giorni
    u8g2.setFont(u8g2_font_fub25_tn);
    
    // Centratura del numero in base alle cifre
    String daysStr = String(daysToChristmas);
    int x_pos = 64 - (daysStr.length() * 15); // Centratura approssimativa
    u8g2.drawStr(x_pos, 45, daysStr.c_str());
    
    // Testo descrittivo
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(10, 58, "giorni rimanenti");
    
  } else if (daysToChristmas == 0) {
    // È Natale oggi!
    u8g2.setFont(u8g2_font_10x20_tr);
    u8g2.drawStr(15, 35, "OGGI E'");
    u8g2.drawStr(15, 55, "NATALE!");
    
  } else {
    // Natale è passato
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(5, 35, "Natale e' passato!");
    
    String nextYear = "Prossimo tra " + String(365 + daysToChristmas) + " giorni";
    u8g2.drawStr(5, 50, nextYear.c_str());
  }
  
  // Indicatore modalità in basso
  u8g2.setFont(u8g2_font_4x6_tr);
  if (switchMode) {
    u8g2.drawStr(85, 63, "SEMPRE ON");
  } else {
    u8g2.drawStr(95, 63, "PULSANTE");
  }
  
  // Invia il buffer al display
  u8g2.sendBuffer();
  Serial.println("📺 Display aggiornato con U8g2");
}

// ============== CALCOLO GIORNI A NATALE ==============
int calculateDaysToChristmas() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  
  // Data di Natale dell'anno corrente
  struct tm christmas = timeinfo;
  christmas.tm_mon = 11;  // Dicembre (0-based)
  christmas.tm_mday = 25; // 25 dicembre
  christmas.tm_hour = 0;
  christmas.tm_min = 0;
  christmas.tm_sec = 0;
  
  time_t christmasTime = mktime(&christmas);
  
  // Se Natale è già passato, calcola per l'anno prossimo
  if (christmasTime < now) {
    christmas.tm_year++;
    christmasTime = mktime(&christmas);
  }
  
  // Calcolo differenza in giorni
  double diff = difftime(christmasTime, now);
  return (int)(diff / (24 * 60 * 60));
}

// ============== CONFIGURAZIONE WIFI E ORARIO ==============
void setupWiFiTime() {
  // Se le credenziali WiFi sono vuote, usa l'orario interno
  if (strlen(ssid) == 0) {
    Serial.println("⚠️ WiFi non configurato - Usando orario interno");
    // Impostazione manuale per test (modificare se necessario)
    // Formato: anno, mese (1-12), giorno, ora, minuto, secondo
    setTime(2025, 7, 9, 12, 0, 0); // 9 luglio 2025, 12:00
    return;
  }
  
  Serial.println("📡 Connessione WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connesso!");
    configTime(3600, 3600, "pool.ntp.org"); // Fuso orario italiano
    
    // Attesa sincronizzazione
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      Serial.println("✅ Orario sincronizzato!");
    }
  } else {
    Serial.println("\n❌ Connessione WiFi fallita - Usando orario interno");
    setTime(2025, 7, 9, 12, 0, 0); // Fallback
  }
}

// ============== FUNZIONE AUSILIARIA PER IMPOSTARE ORARIO ==============
void setTime(int year, int month, int day, int hour, int minute, int second) {
  struct tm timeinfo;
  timeinfo.tm_year = year - 1900;
  timeinfo.tm_mon = month - 1;
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;
  
  time_t t = mktime(&timeinfo);
  struct timeval tv = { .tv_sec = t };
  settimeofday(&tv, NULL);
}
