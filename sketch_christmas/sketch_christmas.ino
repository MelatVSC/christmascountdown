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

// ============== VARIABILI ANIMAZIONI ==============
unsigned long lastAnimationTime = 0; // Timestamp ultima animazione
unsigned long animationInterval = 15000; // Intervallo animazioni (15 secondi)
int currentAnimation = 0;         // Animazione corrente
const int totalAnimations = 4;    // Numero totale di animazioni
bool animationActive = false;     // Flag animazione attiva
unsigned long animationStartTime = 0; // Timestamp inizio animazione
int animationFrame = 0;           // Frame corrente dell'animazione

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
  
  // Decorazioni natalizie sui lati
  drawChristmasDecorations();
  
  // Visualizzazione giorni rimanenti
  if (daysToChristmas > 0) {
    // Scritta "mancano solo" in alto
    u8g2.setFont(u8g2_font_9x15_tr);
    int textWidth = u8g2.getStrWidth("mancano solo");
    u8g2.drawStr((128 - textWidth) / 2, 15, "mancano solo");
    
    // Numero più piccolo al centro (per non toccare "mancano")
    u8g2.setFont(u8g2_font_fub20_tn);
    String daysStr = String(daysToChristmas);
    int numberWidth = u8g2.getStrWidth(daysStr.c_str());
    u8g2.drawStr((128 - numberWidth) / 2, 40, daysStr.c_str());
    
    // Scritta "giorni a Natale" più piccola in basso
    u8g2.setFont(u8g2_font_6x10_tr);
    textWidth = u8g2.getStrWidth("giorni a Natale");
    u8g2.drawStr((128 - textWidth) / 2, 55, "giorni a Natale");
    
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
    // Natale è passato
    u8g2.setFont(u8g2_font_6x10_tr);
    int textWidth = u8g2.getStrWidth("Natale e' passato!");
    u8g2.drawStr((128 - textWidth) / 2, 25, "Natale e' passato!");
    
    String nextYear = "Prossimo tra " + String(365 + daysToChristmas) + " giorni";
    textWidth = u8g2.getStrWidth(nextYear.c_str());
    u8g2.drawStr((128 - textWidth) / 2, 40, nextYear.c_str());
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

// ============== DECORAZIONI NATALIZIE ==============
void drawChristmasDecorations() {
  // Alberello di sinistra (corretto e abbassato)
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2, 20, "*");     // Stella in cima
  u8g2.drawStr(1, 26, "/|\\");   // Parte alta albero
  u8g2.drawStr(0, 32, "/|||\\");  // Parte media albero
  u8g2.drawStr(0, 38, "/||||\\"); // Parte bassa albero
  u8g2.drawStr(3, 44, "|||");    // Tronco
  
  // Alberello di destra (spostato più a sinistra)
  u8g2.drawStr(118, 20, "*");    // Stella in cima
  u8g2.drawStr(117, 26, "/|\\");  // Parte alta albero
  u8g2.drawStr(116, 32, "/|||\\"); // Parte media albero
  u8g2.drawStr(115, 38, "/||||\\"); // Parte bassa albero
  u8g2.drawStr(118, 44, "|||");   // Tronco
  
  // Piccole stelle decorative (meglio posizionate)
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(8, 50, ".");     // Stella piccola sinistra
  u8g2.drawStr(110, 50, ".");   // Stella piccola destra (più a sinistra)
  u8g2.drawStr(12, 58, ".");    // Stella piccola sinistra bassa
  u8g2.drawStr(106, 58, ".");   // Stella piccola destra bassa (più a sinistra)
}

// ============== GESTIONE ANIMAZIONI ==============
void startAnimation() {
  animationActive = true;
  animationStartTime = millis();
  animationFrame = 0;
  lastAnimationTime = millis();
  
  Serial.print("🎬 Avvio animazione ");
  Serial.println(currentAnimation + 1);
}

void handleAnimation() {
  unsigned long animationDuration = 5000; // 5 secondi per animazione (più lunga)
  
  // Controlla se l'animazione è terminata
  if (millis() - animationStartTime > animationDuration) {
    animationActive = false;
    currentAnimation = (currentAnimation + 1) % totalAnimations;
    Serial.println("🎬 Animazione terminata");
    return;
  }
  
  // Esegue l'animazione corrente
  switch (currentAnimation) {
    case 0:
      animationSnowfall();
      break;
    case 1:
      animationFireworks();
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

// ============== ANIMAZIONE 2: FUOCHI D'ARTIFICIO ==============
void animationFireworks() {
  u8g2.clearBuffer();
  
  int frameSpeed = (millis() - animationStartTime) / 80; // Frame ogni 80ms (molto veloce)
  
  // Fuochi d'artificio che esplodono in punti diversi
  for (int firework = 0; firework < 3; firework++) {
    int centerX = 30 + firework * 35;
    int centerY = 20 + (firework * 15) % 25;
    int explosionRadius = (frameSpeed + firework * 10) % 30;
    
    if (explosionRadius > 5 && explosionRadius < 25) {
      // Particelle che si espandono
      for (int angle = 0; angle < 8; angle++) {
        int x = centerX + (explosionRadius * cos(angle * 0.785)) / 10; // 0.785 = 45°
        int y = centerY + (explosionRadius * sin(angle * 0.785)) / 10;
        
        if (x >= 0 && x < 128 && y >= 0 && y < 64) {
          u8g2.setFont(u8g2_font_6x10_tr);
          if (explosionRadius < 15) {
            u8g2.drawStr(x, y, "*");
          } else {
            u8g2.setFont(u8g2_font_4x6_tr);
            u8g2.drawStr(x, y, ".");
          }
        }
      }
    }
  }
  
  // Stelle che cadono dai fuochi
  for (int i = 0; i < 15; i++) {
    int x = (i * 19 + frameSpeed * 2) % 128;
    int y = (i * 7 + frameSpeed * 4) % 64;
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(x, y, ".");
  }
  
  u8g2.sendBuffer();
}

// ============== ANIMAZIONE 3: STELLE CADENTI ==============
void animationStars() {
  u8g2.clearBuffer();
  
  // Stelle cadenti diagonali molto veloci
  u8g2.setFont(u8g2_font_6x10_tr);
  int frameSpeed = (millis() - animationStartTime) / 60; // Frame ogni 60ms (molto veloce)
  
  // Stelle grandi veloci
  for (int i = 0; i < 8; i++) {
    int x = (i * 40 + frameSpeed * 6) % 160 - 30;
    int y = (i * 20 + frameSpeed * 4) % 64;
    
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
  
  // Stelle piccole velocissime
  u8g2.setFont(u8g2_font_4x6_tr);
  for (int i = 0; i < 12; i++) {
    int x = (i * 25 + frameSpeed * 8) % 150 - 20;
    int y = (i * 15 + frameSpeed * 5) % 64;
    
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
