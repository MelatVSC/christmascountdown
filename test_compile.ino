/*
 * Test di compilazione per verificare API RTC
 */
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  
  if (rtc.begin()) {
    Serial.println("RTC OK");
    
    // Test API corretta
    DateTime christmas(2025, 12, 25, 0, 0, 0);
    rtc.setAlarm1(christmas, DS3231_A1_Date);
    
    Serial.println("API test OK");
  }
}

void loop() {
  delay(1000);
}
