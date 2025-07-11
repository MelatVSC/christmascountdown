/*
 * 🎄 Christmas Countdown - ESP32 Project
 * 
 * Progetto natalizio che mostra i giorni rimanenti a Natale
 * su un display OLED con controllo tramite pulsante e interruttore
 * 
 * Componenti utilizzati:
 * - ESP32 DevKit v1
 * - Display OLED I2C 0.96" (128x64) - SH1106
 * - Modulo RTC DS3231 (I2C)
 * - Pulsante momentaneo
 * - Interruttore a slitta
 * - Portabatterie 3xAA (4.5V)
 * 
 * Connessioni I2C:
 * - SDA: GPIO 21 (ESP32)
 * - SCL: GPIO 22 (ESP32)
 * - Display SH1106: Indirizzo I2C 0x3C
 * - RTC DS3231: Indirizzo I2C 0x68
 * - EEPROM AT24C32: Indirizzo I2C 0x57 (integrato nel modulo DS3231)
 * 
 * Funzionalità DS3231:
 * - Orologio in tempo reale con compensazione temperatura (±2ppm)
 * - Batteria backup CR2032 (8-10 anni di durata)
 * - 2 allarmi programmabili giornalieri
 * - Sensore temperatura integrato (±3°C)
 * - Uscita onda quadra 32kHz programmabile
 * - Correzione automatica anni bisestili
 * - Regolazione automatica fine mese
 * 
 * Autore: [Il tuo nome]
 * Data: Luglio 2025
 */

#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <time.h>
#include <math.h>
#include <RTClib.h>  // Libreria per DS3231
#include <EEPROM.h> // Libreria per gestione EEPROM

// ============== COSTANTI EEPROM ==============
#define EEPROM_ANIMATION_INTERVAL 0x00  // Indirizzo per intervallo animazioni
#define EEPROM_DISPLAY_TIMEOUT 0x01     // Indirizzo per timeout display

// ============== CONFIGURAZIONE DISPLAY SH1106 ==============
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
bool displayAvailable = false; // Flag per verificare se display è disponibile

// ============== CONFIGURAZIONE RTC DS3231 ==============
RTC_DS3231 rtc;
bool rtcAvailable = false; // Flag per verificare se RTC è disponibile

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



// ============== VARIABILI ANIMAZIONI ==============
unsigned long lastAnimationTime = 0; // Timestamp ultima animazione
unsigned long animationInterval = 30000; // Intervallo animazioni (30 secondi)
int currentAnimation = 0;         // Animazione corrente
const int totalAnimations = 4;    // Numero totale di animazioni
bool animationActive = false;     // Flag animazione attiva
unsigned long animationStartTime = 0; // Timestamp inizio animazione
int animationFrame = 0;           // Frame corrente dell'animazione

// ============== VARIABILI STELLE MOBILI ==============
struct MovingStar {
  float x;
  float y;
  float speed;
  int size; // 0=piccola, 1=media, 2=grande
  unsigned long lastUpdate;
};

MovingStar movingStars[8]; // 8 stelle mobili per non intralciare
bool starsInitialized = false;

// Configurazione WiFi per sincronizzazione orario
const char* ssid = "TuaReteWiFi"; // Inserire il nome della rete WiFi 
const char* password = "TuaPassword"; // Inserire la password WiFi

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  Serial.println("🎄 Christmas Countdown - Avvio...");
  
  // Carica configurazioni salvate
  loadConfiguration();
  
  // Inizializzazione pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(DISPLAY_POWER_PIN, OUTPUT);
  
  // Inizializzazione display
  Serial.println("🔍 Tentativo inizializzazione display SH1106...");
  u8g2.begin();
  displayAvailable = true;
  Serial.println("✅ Display SH1106 inizializzato correttamente!");
  
  // Inizializzazione RTC DS3231
  Serial.println("🔍 Tentativo inizializzazione RTC DS3231...");
  if (rtc.begin()) {
    rtcAvailable = true;
    Serial.println("✅ RTC DS3231 inizializzato correttamente!");
    
    // Controlla se il RTC ha perso l'alimentazione o se è il primo avvio
    if (rtc.lostPower()) {
      Serial.println("⚠️ RTC ha perso l'alimentazione - Impostazione orario corrente");
      setRTCFromCompileTime();
    } else {
      // Controlla se l'orario del RTC è ragionevole (non troppo vecchio o futuro)
      DateTime now = rtc.now();
      DateTime compileTime = getCompileDateTime();
      
      // Se la differenza è maggiore di 1 settimana, aggiorna con l'orario di compilazione
      TimeSpan diff = now - compileTime;
      int32_t daysDiff = diff.days();
      if ((daysDiff > 7) || (daysDiff < -7)) {
        Serial.println("⚠️ Orario RTC non aggiornato - Impostazione orario di compilazione");
        setRTCFromCompileTime();
      } else {
        Serial.println("✅ Orario RTC già aggiornato");
      }
    }
    
    // Mostra orario corrente del RTC
    DateTime now = rtc.now();
    Serial.print("📅 Data RTC: ");
    Serial.print(now.day());
    Serial.print("/");
    Serial.print(now.month());
    Serial.print("/");
    Serial.print(now.year());
    Serial.print(" ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
    
    // Opzione per impostare manualmente l'orario
    Serial.println("💡 Per impostare manualmente l'orario RTC:");
    Serial.println("   Invia: SET_TIME,YYYY,MM,DD,HH,MM,SS");
    Serial.println("   Esempio: SET_TIME,2025,7,10,14,30,0");
    
    // Controllo automatico stato batteria
    checkRTCBatteryStatus();
    
    // Configurazione avanzata DS3231
    configureDS3231Advanced();
    
    // Imposta allarme per Natale
    setChristmasAlarm();
  } else {
    rtcAvailable = false;
    Serial.println("❌ RTC DS3231 non trovato - Usando orario interno");
  }
  
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
  } else {
    // Interruttore chiuso (NC) = Modalità pulsante
    switchMode = false;
    turnOffDisplay();
    Serial.println("🔄 Interruttore NC chiuso - Modalità pulsante");
  }
  
  // Inizializzazione stelle mobili
  initializeMovingStars();
  
  // Configurazione avanzata DS3231
  configureDS3231Advanced();
  
  // Carica configurazioni salvate
  loadConfiguration();
  
  Serial.println("✅ Setup completato!");
}

// ============== LOOP PRINCIPALE ==============
void loop() {
  // Controllo comandi seriali per impostazione orario
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    handleSerialCommand(command);
  }
  
  // Controllo allarme Natale
  if (checkChristmasAlarm()) {
    // Forza visualizzazione messaggio Natale
    turnOnDisplay();
    animationActive = false; // Interrompi eventuali animazioni
  }
  
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
    // Controllo se è ora di avviare un'animazione
    if (millis() - lastAnimationTime > animationInterval && !animationActive) {
      startAnimation();
    }
    
    // Gestione animazioni
    if (animationActive) {
      handleAnimation();
    } else {
      updateDisplay();
    }
  }
  
  // Aggiornamento stelle mobili (anche se display è spento)
  updateMovingStars();
  
  // Disegno stelle mobili solo se display è acceso (per risparmio energetico)
  if (displayOn) {
    drawMovingStars();
  }
  
  // Controllo allarme di Natale
  checkChristmasAlarm();
  
  // Aggiornamento stati precedenti
  lastButtonState = currentButtonState;
  lastSwitchState = currentSwitchState;
  
  delay(100); // Pausa ridotta per fluidità delle stelle mobili
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
  // Inizializza le stelle mobili se non già fatto
  if (!starsInitialized) {
    initializeMovingStars();
  }
  
  // Calcolo giorni rimanenti per debug
  int daysToChristmas = calculateDaysToChristmas();
  
  // Debug sempre attivo
  Serial.print("📊 Countdown: ");
  Serial.print(daysToChristmas);
  Serial.println(" giorni a Natale");
  
  // 🔍 Debug della logica di visualizzazione
  Serial.print("🔍 Display Logic: ");
  if (daysToChristmas > 0) {
    Serial.println("Modalità countdown normale");
  } else if (daysToChristmas == 0) {
    Serial.println("Modalità 'OGGI È NATALE!'");
  } else {
    Serial.println("Modalità errore (giorni negativi)");
  }
  
  // Aggiorna display solo se disponibile
  if(!displayAvailable) {
    Serial.println("⚠️ Display non disponibile - Solo debug seriale");
    return;
  }
  
  // Inizia nuovo frame U8g2
  u8g2.clearBuffer();
  
  // Decorazioni natalizie con stelle mobili
  drawChristmasDecorations();
  
  // Visualizzazione giorni rimanenti
  if (daysToChristmas > 0) {
    // Gestione singolare/plurale per "manca/mancano"
    String headerText = (daysToChristmas == 1) ? "manca solo" : "mancano solo";
    String footerText = (daysToChristmas == 1) ? "giorno a Natale" : "giorni a Natale";
    
    // Scritta "manca solo" o "mancano solo" in alto
    u8g2.setFont(u8g2_font_9x15_tr);
    int textWidth = u8g2.getStrWidth(headerText.c_str());
    u8g2.drawStr((128 - textWidth) / 2, 15, headerText.c_str());
    
    // Numero più piccolo al centro (per non toccare "mancano")
    u8g2.setFont(u8g2_font_fub20_tn);
    String daysStr = String(daysToChristmas);
    int numberWidth = u8g2.getStrWidth(daysStr.c_str());
    u8g2.drawStr((128 - numberWidth) / 2, 40, daysStr.c_str());
    
    // Scritta "giorno a Natale" o "giorni a Natale" più piccola in basso
    u8g2.setFont(u8g2_font_6x10_tr);
    textWidth = u8g2.getStrWidth(footerText.c_str());
    u8g2.drawStr((128 - textWidth) / 2, 55, footerText.c_str());
    
  } else if (daysToChristmas == 0) {
    // È Natale oggi!
    u8g2.setFont(u8g2_font_10x20_tr);
    int textWidth = u8g2.getStrWidth("OGGI E'");
    u8g2.drawStr((128 - textWidth) / 2, 25, "OGGI E'");
    
    textWidth = u8g2.getStrWidth("NATALE!");
    u8g2.drawStr((128 - textWidth) / 2, 50, "NATALE!");
    
    // Stella sopra
    u8g2.drawStr(60, 10, "*");
    
  } else {
    // Caso di errore (non dovrebbe succedere con la logica attuale)
    u8g2.setFont(u8g2_font_6x10_tr);
    int textWidth = u8g2.getStrWidth("Errore calcolo!");
    u8g2.drawStr((128 - textWidth) / 2, 25, "Errore calcolo!");
    
    String debugStr = "Giorni: " + String(daysToChristmas);
    textWidth = u8g2.getStrWidth(debugStr.c_str());
    u8g2.drawStr((128 - textWidth) / 2, 40, debugStr.c_str());
  }
  
  // Indicatore modalità in basso al centro (solo se modalità pulsante)
  if (!switchMode) {
    u8g2.setFont(u8g2_font_4x6_tr);
    String modeText = "PULSANTE";
    int modeWidth = u8g2.getStrWidth(modeText.c_str());
    u8g2.drawStr((128 - modeWidth) / 2, 63, modeText.c_str());
  }
  
  // Invia il buffer al display
  u8g2.sendBuffer();
  Serial.println("📺 Display aggiornato con U8g2");
}

// ============== CALCOLO GIORNI A NATALE ==============
int calculateDaysToChristmas() {
  DateTime now;
  
  // Usa il RTC DS3231 se disponibile, altrimenti usa l'orario di sistema
  if (rtcAvailable) {
    now = rtc.now();
    Serial.print("📅 RTC: ");
    Serial.print(now.day());
    Serial.print("/");
    Serial.print(now.month());
    Serial.print("/");
    Serial.print(now.year());
    Serial.print(" ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
  } else {
    // Fallback all'orario di sistema
    time_t nowTime;
    struct tm timeinfo;
    time(&nowTime);
    localtime_r(&nowTime, &timeinfo);
    
    now = DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                   timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    Serial.println("📅 Usando orario di sistema (RTC non disponibile)");
  }
  
  // Controllo specifico per il 25 dicembre
  if (now.month() == 12 && now.day() == 25) {
    Serial.println("🔍 Debug: È il 25 dicembre - OGGI È NATALE!");
    return 0; // È Natale oggi!
  }
  
  // Data di Natale dell'anno corrente (ora 00:00:00)
  DateTime christmas(now.year(), 12, 25, 0, 0, 0);
  
  // Se Natale è già passato (dopo il 25 dicembre), calcola per l'anno prossimo
  if ((now.month() == 12 && now.day() > 25) || 
      (now.month() > 12)) {
    christmas = DateTime(now.year() + 1, 12, 25, 0, 0, 0);
  }
  
  // Calcolo differenza in giorni - usa la data senza considerare l'ora
  DateTime nowDate(now.year(), now.month(), now.day(), 0, 0, 0);
  DateTime christmasDate(christmas.year(), christmas.month(), christmas.day(), 0, 0, 0);
  
  TimeSpan diff = christmasDate - nowDate;
  int daysLeft = diff.days();
  
  // Debug del calcolo
  Serial.print("🔍 Debug: Oggi ");
  Serial.print(now.day());
  Serial.print("/");
  Serial.print(now.month());
  Serial.print(" vs Natale ");
  Serial.print(christmas.day());
  Serial.print("/");
  Serial.print(christmas.month());
  Serial.print("/");
  Serial.print(christmas.year());
  Serial.print(" = ");
  Serial.print(daysLeft);
  Serial.println(" giorni");
  
  return daysLeft;
}

// ============== CONFIGURAZIONE WIFI E SINCRONIZZAZIONE ORARIO ==============
void setupWiFiTime() {
  // Se il RTC è disponibile, prova a sincronizzare l'orario via WiFi
  if (rtcAvailable) {
    Serial.println("🔄 RTC DS3231 disponibile - Tentativo sincronizzazione WiFi opzionale");
    
    // Se le credenziali WiFi sono vuote, usa solo il RTC
    if (strlen(ssid) == 0) {
      Serial.println("⚠️ WiFi non configurato - Usando solo RTC DS3231");
      return;
    }
    
    Serial.println("📡 Connessione WiFi per sincronizzazione...");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ WiFi connesso! Sincronizzazione RTC...");
      
      // Configura NTP
      configTime(3600, 3600, "pool.ntp.org"); // Fuso orario italiano
      
      // Attendi sincronizzazione NTP
      struct tm timeinfo;
      int ntpAttempts = 0;
      while (!getLocalTime(&timeinfo) && ntpAttempts < 10) {
        delay(1000);
        ntpAttempts++;
        Serial.print(".");
      }
      
      if (getLocalTime(&timeinfo)) {
        // Sincronizza il RTC con l'orario NTP
        DateTime ntpTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        rtc.adjust(ntpTime);
        Serial.println("\n✅ RTC sincronizzato con NTP!");
        
        Serial.print("📅 Nuovo orario RTC: ");
        DateTime now = rtc.now();
        Serial.print(now.day());
        Serial.print("/");
        Serial.print(now.month());
        Serial.print("/");
        Serial.print(now.year());
        Serial.print(" ");
        Serial.print(now.hour());
        Serial.print(":");
        Serial.print(now.minute());
        Serial.print(":");
        Serial.println(now.second());
      } else {
        Serial.println("\n❌ Sincronizzazione NTP fallita - Mantengo orario RTC");
      }
      
      // Disconnetti WiFi per risparmiare energia
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      Serial.println("📡 WiFi disconnesso per risparmio energia");
    } else {
      Serial.println("\n❌ Connessione WiFi fallita - Usando orario RTC");
    }
  } else {
    // Se il RTC non è disponibile, usa il sistema precedente
    Serial.println("⚠️ RTC non disponibile - Usando orario di sistema");
    
    if (strlen(ssid) == 0) {
      Serial.println("⚠️ WiFi non configurato - Usando orario interno");
      setTime(2025, 7, 10, 12, 0, 0); // 10 luglio 2025, 12:00
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
      
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        Serial.println("✅ Orario sincronizzato!");
      }
    } else {
      Serial.println("\n❌ Connessione WiFi fallita - Usando orario interno");
      setTime(2025, 7, 10, 12, 0, 0); // 10 luglio 2025, 12:00
    }
  }
}

// ============== GESTIONE COMANDI SERIALI ==============
void handleSerialCommand(String command) {
  // Comando per impostare orario: SET_TIME,YYYY,MM,DD,HH,MM,SS
  if (command.startsWith("SET_TIME,")) {
    command.remove(0, 9); // Rimuovi "SET_TIME,"
    
    // Parsing dei parametri
    int year = command.substring(0, command.indexOf(',')).toInt();
    command.remove(0, command.indexOf(',') + 1);
    
    int month = command.substring(0, command.indexOf(',')).toInt();
    command.remove(0, command.indexOf(',') + 1);
    
    int day = command.substring(0, command.indexOf(',')).toInt();
    command.remove(0, command.indexOf(',') + 1);
    
    int hour = command.substring(0, command.indexOf(',')).toInt();
    command.remove(0, command.indexOf(',') + 1);
    
    int minute = command.substring(0, command.indexOf(',')).toInt();
    command.remove(0, command.indexOf(',') + 1);
    
    int second = command.toInt();
    
    // Validazione parametri
    if (year >= 2025 && month >= 1 && month <= 12 && day >= 1 && day <= 31 &&
        hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
      setRTCTime(year, month, day, hour, minute, second);
    } else {
      Serial.println("❌ Parametri non validi. Formato: SET_TIME,YYYY,MM,DD,HH,MM,SS");
    }
  }
  // Comando per mostrare orario corrente
  else if (command.equals("SHOW_TIME")) {
    if (rtcAvailable) {
      DateTime now = rtc.now();
      Serial.print("📅 Orario RTC corrente: ");
      Serial.print(now.day());
      Serial.print("/");
      Serial.print(now.month());
      Serial.print("/");
      Serial.print(now.year());
      Serial.print(" ");
      Serial.print(now.hour());
      Serial.print(":");
      Serial.print(now.minute());
      Serial.print(":");
      Serial.println(now.second());
      
      int days = calculateDaysToChristmas();
      Serial.print("🎄 Giorni a Natale: ");
      Serial.println(days);
    } else {
      Serial.println("❌ RTC non disponibile");
    }
  }
  // Comando per help
  else if (command.equals("HELP")) {
    Serial.println("🎄 COMANDI DISPONIBILI:");
    Serial.println("   SET_TIME,YYYY,MM,DD,HH,MM,SS - Imposta orario RTC");
    Serial.println("   SHOW_TIME - Mostra orario corrente");
    Serial.println("   CHECK_BATTERY - Verifica stato batteria RTC");
    Serial.println("   SAVE_CONFIG - Salva configurazione in EEPROM");
    Serial.println("   LOAD_CONFIG - Carica configurazione da EEPROM");
    Serial.println("   SYNC_NTP - Forza sincronizzazione con NTP");
    Serial.println("   NOW - Mostra orario compilazione e RTC");
    Serial.println("   TEST - Esegue test con date specifiche");
    Serial.println("   HELP - Mostra questo messaggio");
  }
  // Comando per verificare batteria
  else if (command.equals("CHECK_BATTERY")) {
    checkRTCBatteryStatus();
  }
  // Comando per salvare configurazione
  else if (command.equals("SAVE_CONFIG")) {
    saveConfiguration();
  }
  // Comando per caricare configurazione
  else if (command.equals("LOAD_CONFIG")) {
    loadConfiguration();
  }
  // Comando per eseguire test con date specifiche
  else if (command.equals("TEST")) {
    Serial.println("🧪 Eseguendo test scenari date...");
    testDateScenarios();
  }
  // Comando per sincronizzare con NTP
  else if (command.equals("SYNC_NTP")) {
    Serial.println("🔄 Forzando sincronizzazione NTP...");
    syncWithNTP();
  }
  // Comando per ottenere orario attuale
  else if (command.equals("NOW")) {
    Serial.print("⏰ Orario di compilazione: ");
    DateTime compileTime = getCompileDateTime();
    Serial.print(compileTime.day());
    Serial.print("/");
    Serial.print(compileTime.month());
    Serial.print("/");
    Serial.print(compileTime.year());
    Serial.print(" ");
    Serial.print(compileTime.hour());
    Serial.print(":");
    Serial.print(compileTime.minute());
    Serial.print(":");
    Serial.println(compileTime.second());
    
    if (rtcAvailable) {
      DateTime now = rtc.now();
      Serial.print("⏰ Orario RTC: ");
      Serial.print(now.day());
      Serial.print("/");
      Serial.print(now.month());
      Serial.print("/");
      Serial.print(now.year());
      Serial.print(" ");
      Serial.print(now.hour());
      Serial.print(":");
      Serial.print(now.minute());
      Serial.print(":");
      Serial.println(now.second());
    }
  }
  else {
    Serial.println("❌ Comando non riconosciuto. Scrivi HELP per la lista comandi.");
  }
}

// ============== FUNZIONE PER IMPOSTARE ORARIO RTC MANUALMENTE ==============
void setRTCTime(int year, int month, int day, int hour, int minute, int second) {
  if (rtcAvailable) {
    DateTime newTime(year, month, day, hour, minute, second);
    rtc.adjust(newTime);
    Serial.print("✅ Orario RTC impostato: ");
    Serial.print(day);
    Serial.print("/");
    Serial.print(month);
    Serial.print("/");
    Serial.print(year);
    Serial.print(" ");
    Serial.print(hour);
    Serial.print(":");
    Serial.print(minute);
    Serial.print(":");
    Serial.println(second);
  } else {
    Serial.println("❌ RTC non disponibile - Impossibile impostare orario");
  }
}

// ============== FUNZIONE AUSILIARIA PER IMPOSTARE ORARIO (FALLBACK) ==============
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

// ============== DECORAZIONI NATALIZIE ==============
void drawChristmasDecorations() {
  // Disegna le stelle mobili di sfondo
  drawMovingStars();
  
  // Alberello di sinistra (corretto e abbassato)
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(6, 20, "*");     // Stella in cima (spostata a destra)
  u8g2.drawStr(2, 26, "/|\\");   // Parte alta albero
  u8g2.drawStr(0, 32, "/|||\\");  // Parte media albero
  u8g2.drawStr(0, 38, "/||||\\"); // Parte bassa albero
  u8g2.drawStr(3, 44, "|||");    // Tronco
  
  // Alberello di destra (spostato più a sinistra)
  u8g2.drawStr(121, 20, "*");    // Stella in cima (spostata a sinistra)
  u8g2.drawStr(117, 26, "/|\\");  // Parte alta albero
  u8g2.drawStr(116, 32, "/|||\\"); // Parte media albero
  u8g2.drawStr(115, 38, "/||||\\"); // Parte bassa albero
  u8g2.drawStr(118, 44, "|||");   // Tronco
  
  // Piccole stelle decorative statiche (meglio posizionate)
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(8, 50, ".");     // Stella piccola sinistra
  u8g2.drawStr(110, 50, ".");   // Stella piccola destra
  u8g2.drawStr(12, 58, ".");    // Stella piccola sinistra bassa
  u8g2.drawStr(106, 58, ".");   // Stella piccola destra bassa
}

// ============== GESTIONE ANIMAZIONI ==============
void startAnimation() {
  // Animazione di transizione da countdown ad animazione
  transitionToAnimation();
  
  animationActive = true;
  animationStartTime = millis();
  animationFrame = 0;
  lastAnimationTime = millis();
  
  Serial.print("🎬 Avvio animazione ");
  Serial.println(currentAnimation + 1);
}

void handleAnimation() {
  unsigned long animationDuration = 12000; // 12 secondi per animazione (ancora più lunga)
  
  // Controlla se l'animazione è terminata
  if (millis() - animationStartTime > animationDuration) {
    animationActive = false;
    currentAnimation = (currentAnimation + 1) % totalAnimations;
    Serial.println("🎬 Animazione terminata");
    
    // Animazione di transizione da animazione a countdown
    transitionToCountdown();
    return;
  }
  
  // Esegue l'animazione corrente
  switch (currentAnimation) {
    case 0:
      animationSnowfall();
      break;
    case 1:
      animationSantaFace();
      break;
    case 2:
      animationStars();
      break;
    case 3:
      animationSanta();
      break;
  }
}

// ============== ANIMAZIONE 1: NEVE CHE CADE ==============
void animationSnowfall() {
  u8g2.clearBuffer();
  
  // Fiocchi di neve che cadono a velocità diverse
  u8g2.setFont(u8g2_font_6x10_tr);
  int frameSpeed = (millis() - animationStartTime) / 100; // Frame ogni 100ms (più veloce)
  
  // Neve veloce
  for (int i = 0; i < 15; i++) {
    int x = (i * 23 + frameSpeed * 3) % 128;
    int y = (i * 17 + frameSpeed * 4) % 64;
    u8g2.drawStr(x, y, "*");
  }
  
  // Neve più lenta
  for (int i = 0; i < 10; i++) {
    int x = (i * 31 + frameSpeed * 2) % 128;
    int y = (i * 19 + frameSpeed * 2) % 64;
    u8g2.drawStr(x, y, ".");
  }
  
  // Neve piccolissima veloce
  u8g2.setFont(u8g2_font_4x6_tr);
  for (int i = 0; i < 20; i++) {
    int x = (i * 19 + frameSpeed * 5) % 128;
    int y = (i * 11 + frameSpeed * 6) % 64;
    u8g2.drawStr(x, y, ".");
  }
  
  // Neve accumulata che cresce
  u8g2.setFont(u8g2_font_4x6_tr);
  int snowLevel = min(frameSpeed / 10, 3); // Cresce nel tempo
  for (int level = 0; level < snowLevel; level++) {
    for (int i = 0; i < 128; i += 6) {
      u8g2.drawStr(i, 63 - level, ".");
    }
  }
  
  u8g2.sendBuffer();
}

// ============== ANIMAZIONE 2: FACCIA DI BABBO NATALE ==============
void animationSantaFace() {
  u8g2.clearBuffer();
  
  int frameSpeed = (millis() - animationStartTime) / 200; // Frame ogni 200ms
  
  // Faccia di Babbo Natale grande al centro
  u8g2.setFont(u8g2_font_10x20_tr);
  
  // Cappello
  u8g2.drawStr(50, 15, "^^^^");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(45, 20, "------");
  
  // Occhi che lampeggiano
  u8g2.setFont(u8g2_font_8x13_tr);
  if (frameSpeed % 8 < 7) {
    u8g2.drawStr(50, 30, "O   O");  // Occhi aperti
  } else {
    u8g2.drawStr(50, 30, "-   -");  // Occhi chiusi (ammicca)
  }
  
  // Naso
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(62, 35, "o");
  
  // Bocca che si muove
  u8g2.setFont(u8g2_font_8x13_tr);
  if (frameSpeed % 6 < 2) {
    u8g2.drawStr(55, 45, "---");    // Bocca normale
  } else if (frameSpeed % 6 < 4) {
    u8g2.drawStr(55, 45, "OOO");    // Dice "OH"
  } else {
    u8g2.drawStr(55, 45, "ooo");    // Dice "oh"
  }
  
  // Barba
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(45, 50, "vvvvvvv");
  u8g2.drawStr(48, 55, "vvvvv");
  u8g2.drawStr(51, 60, "vvv");
  
  // HO HO HO che appare e scompare
  if (frameSpeed % 6 >= 2 && frameSpeed % 6 < 5) {
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(5, 25, "HO");
    u8g2.drawStr(100, 35, "HO");
    u8g2.drawStr(10, 50, "HO");
  }
  
  // Stelline che scintillano intorno
  if (frameSpeed % 4 < 2) {
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(20, 15, "*");
    u8g2.drawStr(100, 20, "*");
    u8g2.drawStr(15, 40, "*");
    u8g2.drawStr(110, 45, "*");
  }
  
  u8g2.sendBuffer();
}

// ============== ANIMAZIONE 3: STELLE CADENTI ==============
void animationStars() {
  u8g2.clearBuffer();
  
  // Stelle cadenti diagonali più lente
  u8g2.setFont(u8g2_font_6x10_tr);
  int frameSpeed = (millis() - animationStartTime) / 120; // Frame ogni 120ms (più lento)
  
  // Stelle grandi più lente
  for (int i = 0; i < 8; i++) {
    int x = (i * 40 + frameSpeed * 4) % 160 - 30; // Velocità ridotta da 6 a 4
    int y = (i * 20 + frameSpeed * 3) % 64; // Velocità ridotta da 4 a 3
    
    if (x >= 0 && x < 128) {
      u8g2.drawStr(x, y, "*");
      // Scia lunga
      if (x > 6) u8g2.drawStr(x - 6, y, ".");
      if (x > 12) u8g2.drawStr(x - 12, y, ".");
      if (x > 18) {
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(x - 18, y, ".");
      }
    }
  }
  
  // Stelle piccole più lente
  u8g2.setFont(u8g2_font_4x6_tr);
  for (int i = 0; i < 12; i++) {
    int x = (i * 25 + frameSpeed * 5) % 150 - 20; // Velocità ridotta da 8 a 5
    int y = (i * 15 + frameSpeed * 3) % 64; // Velocità ridotta da 5 a 3
    
    if (x >= 0 && x < 128) {
      u8g2.drawStr(x, y, "*");
      if (x > 4) u8g2.drawStr(x - 4, y, ".");
      if (x > 8) u8g2.drawStr(x - 8, y, ".");
    }
  }
  
  // Stelle che lampeggiano
  if (frameSpeed % 3 == 0) {
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(20, 15, "*");
    u8g2.drawStr(100, 25, "*");
    u8g2.drawStr(40, 50, "*");
  }
  
  u8g2.sendBuffer();
}

// ============== ANIMAZIONE 4: BABBO NATALE ==============
void animationSanta() {
  u8g2.clearBuffer();
  
  // Babbo Natale che attraversa lo schermo velocemente
  int frameSpeed = (millis() - animationStartTime) / 150; // Frame ogni 150ms (più veloce)
  int santaX = -20 + (frameSpeed * 4) % 170; // Attraversa tutto lo schermo
  
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(santaX, 25, "^");      // Cappello
  u8g2.drawStr(santaX, 31, "O");      // Testa
  u8g2.drawStr(santaX - 2, 37, "|||"); // Corpo
  
  // Animazione gambe che camminano
  if (frameSpeed % 2 == 0) {
    u8g2.drawStr(santaX - 2, 43, "/ \\"); // Gambe posizione 1
  } else {
    u8g2.drawStr(santaX - 2, 43, "\\ /"); // Gambe posizione 2
  }
  
  // Sacco regalo che rimbalza
  u8g2.setFont(u8g2_font_4x6_tr);
  int saccoY = 37 + (frameSpeed % 4 < 2 ? 0 : 2); // Rimbalza
  u8g2.drawStr(santaX + 6, saccoY, "[]");
  
  // HO HO HO che segue Babbo Natale
  if (frameSpeed % 6 < 3) {
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(santaX + 12, 28, "HO");
    u8g2.drawStr(santaX + 12, 35, "HO");
  }
  
  // Regali che cadono dietro
  for (int i = 0; i < 5; i++) {
    int giftX = santaX - 10 - (i * 8);
    int giftY = 45 + (frameSpeed + i) % 6;
    if (giftX >= 0 && giftX < 128) {
      u8g2.setFont(u8g2_font_4x6_tr);
      u8g2.drawStr(giftX, giftY, "o");
    }
  }
  
  // Neve che si muove velocemente
  for (int i = 0; i < 10; i++) {
    int snowX = (i * 20 + frameSpeed * 6) % 128;
    int snowY = (i * 13 + frameSpeed * 2) % 64;
    u8g2.drawStr(snowX, snowY, ".");
  }
  
  u8g2.sendBuffer();
}

// ============== ANIMAZIONI DI TRANSIZIONE ==============
void transitionToAnimation() {
  Serial.println("🔄 Transizione da countdown ad animazione");
  
  // Effetto di stelle che si moltiplicano e si muovono
  for (int frame = 0; frame < 20; frame++) {
    u8g2.clearBuffer();
    
    // Stelle che si espandono dal centro verso l'esterno
    int numStars = frame + 5;
    for (int i = 0; i < numStars && i < 30; i++) {
      int x = 64 + (i * 7 + frame * 3) % 120 - 60;
      int y = 32 + (i * 5 + frame * 2) % 60 - 30;
      
      // Mantieni le coordinate nei limiti
      if (x < 0) x = 128 + x;
      if (y < 0) y = 64 + y;
      if (x >= 128) x = x - 128;
      if (y >= 64) y = y - 64;
      
      u8g2.setFont(u8g2_font_4x6_tr);
      u8g2.drawStr(x, y, "*");
    }
    
    // Puntini che si muovono in spirale
    for (int i = 0; i < 15; i++) {
      int radius = (frame + i) * 2;
      int x = 64 + (radius % 50) - 25;
      int y = 32 + ((radius + i * 3) % 40) - 20;
      
      if (x >= 0 && x < 128 && y >= 0 && y < 64) {
        u8g2.drawPixel(x, y);
      }
    }
    
    u8g2.sendBuffer();
    delay(60);
  }
}

void transitionToCountdown() {
  Serial.println("🔄 Transizione da animazione a countdown");
  
  // Effetto di stelle che si raccolgono al centro
  for (int frame = 20; frame >= 0; frame--) {
    u8g2.clearBuffer();
    
    // Stelle che si muovono verso il centro
    int numStars = frame + 5;
    for (int i = 0; i < numStars && i < 30; i++) {
      int x = 64 + (i * 7 + frame * 3) % 120 - 60;
      int y = 32 + (i * 5 + frame * 2) % 60 - 30;
      
      // Mantieni le coordinate nei limiti
      if (x < 0) x = 128 + x;
      if (y < 0) y = 64 + y;
      if (x >= 128) x = x - 128;
      if (y >= 64) y = y - 64;
      
      u8g2.setFont(u8g2_font_4x6_tr);
      u8g2.drawStr(x, y, "*");
    }
    
    u8g2.sendBuffer();
    delay(60);
  }
  
  delay(200); // Pausa finale per stabilizzare
}

// ============== FUNZIONI STELLE MOBILI ==============
void initializeMovingStars() {
  for (int i = 0; i < 8; i++) {
    // Posiziona le stelle solo nelle zone laterali per non intralciare il testo
    if (i < 4) {
      movingStars[i].x = random(5, 25); // Lato sinistro
    } else {
      movingStars[i].x = random(103, 123); // Lato destro
    }
    
    movingStars[i].y = random(0, 64);
    movingStars[i].speed = random(30, 100) / 100.0; // Velocità tra 0.3 e 1.0 pixel/secondo
    movingStars[i].size = random(0, 3); // 0=piccola, 1=media, 2=grande
    movingStars[i].lastUpdate = millis();
  }
  starsInitialized = true;
}

void updateMovingStars() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < 8; i++) {
    // Calcola il tempo trascorso dall'ultimo aggiornamento
    unsigned long deltaTime = currentTime - movingStars[i].lastUpdate;
    
    // Aggiorna posizione ogni 100ms per movimento fluido
    if (deltaTime >= 100) {
      // Movimento verso il basso con leggera oscillazione orizzontale
      movingStars[i].x += random(-1, 2) * 0.3; // Oscillazione molto leggera
      movingStars[i].y += movingStars[i].speed;
      
      // Mantieni la stella nelle zone laterali
      if (i < 4) {
        // Lato sinistro
        if (movingStars[i].x < 5) movingStars[i].x = 5;
        if (movingStars[i].x > 25) movingStars[i].x = 25;
      } else {
        // Lato destro
        if (movingStars[i].x < 103) movingStars[i].x = 103;
        if (movingStars[i].x > 123) movingStars[i].x = 123;
      }
      
      // Se la stella esce dal basso, riposizionala in alto
      if (movingStars[i].y > 64) {
        movingStars[i].y = -5;
        movingStars[i].speed = random(30, 100) / 100.0;
        movingStars[i].size = random(0, 3);
      }
      
      movingStars[i].lastUpdate = currentTime;
    }
  }
}

void drawMovingStars() {
  updateMovingStars();
  
  for (int i = 0; i < 8; i++) {
    int x = (int)movingStars[i].x;
    int y = (int)movingStars[i].y;
    
    // Disegna stelle di diverse dimensioni
    switch (movingStars[i].size) {
      case 0: // Stella piccola (1 pixel)
        u8g2.drawPixel(x, y);
        break;
      case 1: // Stella media (croce 3x3)
        u8g2.drawPixel(x, y);
        u8g2.drawPixel(x-1, y);
        u8g2.drawPixel(x+1, y);
        u8g2.drawPixel(x, y-1);
        u8g2.drawPixel(x, y+1);
        break;
      case 2: // Stella grande (asterisco)
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(x-1, y, "*");
        break;
    }
  }
}

// ============== CONFIGURAZIONE AVANZATA DS3231 ==============
void configureDS3231Advanced() {
  if (!rtcAvailable) return;
  
  Serial.println("🔧 Configurazione avanzata DS3231...");
  
  // Disabilita uscita 32kHz per risparmiare energia (se non usata)
  rtc.disable32K();
  Serial.println("   32kHz output disabilitato per risparmio energia");
  
  // Configura l'uscita SQW per fornire 1Hz (utile per debug)
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
  Serial.println("   SQW output configurato a 1Hz");
  
  // Cancella eventuali allarmi esistenti
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  Serial.println("   Allarmi cancellati e disabilitati");
  
  Serial.println("✅ Configurazione avanzata DS3231 completata");
}

// ============== GESTIONE ALLARMI DS3231 ==============
void setChristmasAlarm() {
  if (!rtcAvailable) return;
  
  DateTime now = rtc.now();
  DateTime christmas(now.year(), 12, 25, 0, 0, 0);
  
  // Se Natale è già passato, imposta per l'anno prossimo
  if (now > christmas) {
    christmas = DateTime(now.year() + 1, 12, 25, 0, 0, 0);
  }
  
  // Imposta allarme per la mezzanotte di Natale usando l'API corretta
  // setAlarm1(date, hour, minute, second, mode)
  rtc.setAlarm1(christmas, DS3231_A1_Date);
  
  Serial.print("🔔 Allarme Natale impostato per: ");
  Serial.print(christmas.day());
  Serial.print("/");
  Serial.print(christmas.month());
  Serial.print("/");
  Serial.println(christmas.year());
}

// ============== CONTROLLO ALLARMI ==============
bool checkChristmasAlarm() {
  if (!rtcAvailable) return false;
  
  if (rtc.alarmFired(1)) {
    rtc.clearAlarm(1);
    Serial.println("🎄🔔 ALLARME NATALE ATTIVATO! È NATALE! 🎄🔔");
    return true;
  }
  return false;
}

// ============== VERIFICA STATO BATTERIA RTC ==============
void checkRTCBatteryStatus() {
  if (rtcAvailable) {
    // Leggi il registro di stato del DS3231
    Wire.beginTransmission(0x68); // Indirizzo DS3231
    Wire.write(0x0F); // Registro Status
    Wire.endTransmission();
    
    Wire.requestFrom(0x68, 1);
    if (Wire.available()) {
      uint8_t status = Wire.read();
      
      // Bit 7: Oscillator Stop Flag (OSF)
      if (status & 0x80) {
        Serial.println("⚠️ ATTENZIONE: RTC ha perso l'alimentazione - Verificare batteria!");
      } else {
        Serial.println("✅ RTC: Batteria OK, nessuna perdita di alimentazione");
      }
      
      // Bit 2: 32kHz Enable
      if (status & 0x08) {
        Serial.println("📶 RTC: Uscita 32kHz attiva");
      }
      
      // Mostra temperatura del DS3231 (indicatore di funzionamento)
      float temp = rtc.getTemperature();
      Serial.print("🌡️ Temperatura DS3231: ");
      Serial.print(temp);
      Serial.println(" °C");
    }
  } else {
    Serial.println("❌ RTC non disponibile - Impossibile verificare batteria");
  }
}

// ============== GESTIONE EEPROM AT24C32 ==============
void saveConfiguration() {
  if (!rtcAvailable) return;
  
  // Salva configurazioni nell'EEPROM del modulo
  Serial.println("💾 Salvataggio configurazioni...");
  
  // Nota: Il modulo ha un AT24C32 separato all'indirizzo 0x57
  // Per semplicità, usiamo l'EEPROM interno dell'ESP32
  EEPROM.begin(512);
  
  // Salva intervallo animazioni (in secondi / 1000)
  EEPROM.write(EEPROM_ANIMATION_INTERVAL, animationInterval / 1000);
  
  // Salva timeout display (in secondi / 1000) 
  EEPROM.put(EEPROM_DISPLAY_TIMEOUT, displayTimeout / 1000);
  
  EEPROM.commit();
  Serial.println("✅ Configurazioni salvate");
}

void loadConfiguration() {
  EEPROM.begin(512);
  
  // Carica configurazioni
  uint8_t animInterval = EEPROM.read(EEPROM_ANIMATION_INTERVAL);
  if (animInterval != 0xFF && animInterval > 0) {
    animationInterval = animInterval * 1000;
    Serial.print("📖 Intervallo animazioni caricato: ");
    Serial.print(animationInterval / 1000);
    Serial.println(" secondi");
  }
  
  uint32_t dispTimeout;
  EEPROM.get(EEPROM_DISPLAY_TIMEOUT, dispTimeout);
  if (dispTimeout != 0xFFFFFFFF && dispTimeout > 0 && dispTimeout < 300) {
    displayTimeout = dispTimeout * 1000;
    Serial.print("📖 Timeout display caricato: ");
    Serial.print(displayTimeout / 1000);
    Serial.println(" secondi");
  }
}

// ============== FUNZIONI HELPER PER RTC E DATA/ORA DI COMPILAZIONE ==============

// Funzione per ottenere la data/ora di compilazione
DateTime getCompileDateTime() {
  // Macro predefinite del compilatore per data e ora di compilazione
  // __DATE__ formato: "Jul 10 2025"
  // __TIME__ formato: "12:00:00"
  
  const char* date = __DATE__;
  const char* time = __TIME__;
  
  // Array dei nomi dei mesi
  const char monthNames[12][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  
  // Parsing della data
  char monthStr[4];
  int day, year;
  sscanf(date, "%3s %d %d", monthStr, &day, &year);
  
  // Trova il numero del mese
  int month = 1;
  for (int i = 0; i < 12; i++) {
    if (strncmp(monthStr, monthNames[i], 3) == 0) {
      month = i + 1;
      break;
    }
  }
  
  // Parsing dell'ora
  int hour, minute, second;
  sscanf(time, "%d:%d:%d", &hour, &minute, &second);
  
  // Crea oggetto DateTime
  return DateTime(year, month, day, hour, minute, second);
}

// Funzione per impostare l'RTC con la data/ora di compilazione
void setRTCFromCompileTime() {
  if (!rtcAvailable) {
    Serial.println("❌ RTC non disponibile - Impossibile impostare orario");
    return;
  }
  
  DateTime compileTime = getCompileDateTime();
  
  Serial.print("⏰ Impostazione RTC con orario di compilazione: ");
  Serial.print(compileTime.day());
  Serial.print("/");
  Serial.print(compileTime.month());
  Serial.print("/");
  Serial.print(compileTime.year());
  Serial.print(" ");
  Serial.print(compileTime.hour());
  Serial.print(":");
  if (compileTime.minute() < 10) Serial.print("0");
  Serial.print(compileTime.minute());
  Serial.print(":");
  if (compileTime.second() < 10) Serial.print("0");
  Serial.println(compileTime.second());
  
  // Imposta l'orario nel RTC
  rtc.adjust(compileTime);
  
  // Verifica che l'impostazione sia andata a buon fine
  delay(100);
  DateTime now = rtc.now();
  
  // Calcola la differenza assoluta tra i timestamp
  uint32_t nowTime = now.unixtime();
  uint32_t compileTimeUnix = compileTime.unixtime();
  uint32_t timeDiff = (nowTime > compileTimeUnix) ? (nowTime - compileTimeUnix) : (compileTimeUnix - nowTime);
  
  if (timeDiff < 2) {
    Serial.println("✅ RTC impostato correttamente con orario di compilazione");
  } else {
    Serial.println("⚠️ Possibile errore nell'impostazione del RTC");
  }
}

// ============== FUNZIONE PER SINCRONIZZAZIONE NTP FORZATA ==============
void syncWithNTP() {
  if (!rtcAvailable) {
    Serial.println("❌ RTC non disponibile per sincronizzazione");
    return;
  }
  
  // Se le credenziali WiFi sono vuote, chiedi all'utente
  if (strlen(ssid) == 0 || strcmp(ssid, "TuaReteWiFi") == 0) {
    Serial.println("⚠️ WiFi non configurato!");
    Serial.println("💡 Modifica le credenziali WiFi nel codice:");
    Serial.println("   const char* ssid = \"NomeDellatuaRete\";");
    Serial.println("   const char* password = \"PasswordDellatuaRete\";");
    Serial.println("💡 Oppure usa: SET_TIME,2025,7,10,18,5,0");
    return;
  }
  
  Serial.println("📡 Connessione WiFi per sincronizzazione NTP...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connesso! Sincronizzazione con NTP...");
    
    // Configura NTP per fuso orario italiano (UTC+1, DST +1)
    configTime(3600, 3600, "pool.ntp.org", "time.google.com");
    
    // Attendi sincronizzazione NTP
    struct tm timeinfo;
    int ntpAttempts = 0;
    while (!getLocalTime(&timeinfo) && ntpAttempts < 15) {
      delay(1000);
      ntpAttempts++;
      Serial.print(".");
    }
    
    if (getLocalTime(&timeinfo)) {
      // Sincronizza il RTC con l'orario NTP
      DateTime ntpTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      rtc.adjust(ntpTime);
      Serial.println("\n✅ RTC sincronizzato con NTP!");
      
      Serial.print("📅 Nuovo orario RTC: ");
      DateTime now = rtc.now();
      Serial.print(now.day());
      Serial.print("/");
      Serial.print(now.month());
      Serial.print("/");
      Serial.print(now.year());
      Serial.print(" ");
      Serial.print(now.hour());
      Serial.print(":");
      if (now.minute() < 10) Serial.print("0");
      Serial.print(now.minute());
      Serial.print(":");
      if (now.second() < 10) Serial.print("0");
      Serial.println(now.second());
      
      // Mostra countdown aggiornato
      int days = calculateDaysToChristmas();
      Serial.print("🎄 Giorni a Natale: ");
      Serial.println(days);
    } else {
      Serial.println("\n❌ Sincronizzazione NTP fallita");
    }
    
    // Disconnetti WiFi per risparmiare energia
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    Serial.println("📡 WiFi disconnesso");
  } else {
    Serial.println("\n❌ Connessione WiFi fallita");
  }
}

// ============== FUNZIONI DI TEST DATE ==============
void testDateScenarios() {
  Serial.println("🧪 === TEST SCENARI DATE ===");
  
  // Test con data 24 dicembre
  Serial.println("\n📅 TEST: 24 dicembre 2024");
  DateTime test24Dec(2024, 12, 24, 15, 30, 0);
  testCountdownWithDate(test24Dec);
  
  // Test con data 25 dicembre
  Serial.println("\n📅 TEST: 25 dicembre 2024");
  DateTime test25Dec(2024, 12, 25, 10, 0, 0);
  testCountdownWithDate(test25Dec);
  
  // Test con data 26 dicembre
  Serial.println("\n📅 TEST: 26 dicembre 2024");
  DateTime test26Dec(2024, 12, 26, 12, 0, 0);
  testCountdownWithDate(test26Dec);
  
  // Test con data gennaio
  Serial.println("\n📅 TEST: 15 gennaio 2025");
  DateTime testJan(2025, 1, 15, 8, 0, 0);
  testCountdownWithDate(testJan);
  
  Serial.println("\n🧪 === FINE TEST ===\n");
}

void testCountdownWithDate(DateTime testDate) {
  // Salva la data corrente del RTC se disponibile
  DateTime originalTime;
  bool hadRTC = rtcAvailable;
  
  if (rtcAvailable) {
    originalTime = rtc.now();
    // Imposta temporaneamente la data di test
    rtc.adjust(testDate);
  }
  
  Serial.print("📅 Data test: ");
  Serial.print(testDate.day());
  Serial.print("/");
  Serial.print(testDate.month());
  Serial.print("/");
  Serial.print(testDate.year());
  Serial.print(" ");
  Serial.print(testDate.hour());
  Serial.print(":");
  Serial.print(testDate.minute());
  Serial.print(":");
  Serial.println(testDate.second());
  
  // Calcola i giorni
  int days = calculateDaysToChristmas();
  
  Serial.print("📊 Risultato: ");
  Serial.print(days);
  Serial.print(" giorni - Messaggio: ");
  
  if (days > 1) {
    Serial.print("mancano solo ");
    Serial.print(days);
    Serial.println(" giorni a Natale");
  } else if (days == 1) {
    Serial.println("manca solo 1 giorno a Natale");
  } else if (days == 0) {
    Serial.println("OGGI È NATALE!");
  } else {
    Serial.print("Natale è passato, prossimo tra ");
    Serial.print(365 + days);
    Serial.println(" giorni");
  }
  
  // Ripristina la data originale se aveva RTC
  if (hadRTC && rtcAvailable) {
    rtc.adjust(originalTime);
  }
}
