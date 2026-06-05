#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

// --- Pin Configuraties ---
const int SENSOR_PIN = A3;
const int SD_CS_PIN = 4;

// --- Sensor Drempelwaarden ---
const int LASER_THRESHOLD = 30;  // Boven deze waarde = laser gedetecteerd
                                  // Binnen: omgeving ~12-14, laser ~70-200
                                  // Pas aan als buiten te veel storing is

// --- Detectie Instellingen ---
const unsigned long CAR_MIN_DURATION = 800;  // >800ms continu onderbroken -> Auto
const int BIKE_MIN_INTERRUPTS = 3;           // 4+ snelle onderbrekingen (spaken) -> Fietser
const unsigned long TIMEOUT_MS = 1000;       // Tijd na laatste breuk voordat we evalueren
const unsigned long MIN_BREAK_DURATION = 5; // Korter dan dit = negeren (ruis/trilling)

// --- Overige Instellingen ---
const String SENSOR_ID = "0x001b";
String LOG_FILE = "";  // Wordt aangemaakt in setup() met datum en tijd

// --- Objecten & Variabelen ---
RTC_DS3231 rtc;
bool isMeasuring = false;
bool lastSensorState = HIGH;

unsigned long currentLowStart = 0;
unsigned long maxLowDuration = 0;
unsigned long lastInterruptTime = 0;
int breakCount = 0;

void setup() {
  Serial.begin(9600);

  if (!rtc.begin()) {
    Serial.println("RTC niet gevonden!");
    while (1);
  }
  
  if (rtc.lostPower()) {
    Serial.println("RTC heeft stroom verloren, tijd instellen op compileer-tijd!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Maak bestandsnaam met datum en tijd van inschakelen
DateTime now = rtc.now();
char fileName[13];
sprintf(fileName, "%02d%02d%02d%02d.CSV",
  now.month(),  // Maand
  now.day(),    // Dag
  now.hour(),   // Uur
  now.minute()  // Minuut
);
  LOG_FILE = String(fileName);
  Serial.print("Logbestand: ");
  Serial.println(LOG_FILE);

  Serial.print("Initialiseren SD-kaart...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(" initialisatie mislukt!");
    while (1);
  }
  Serial.println(" gelukt.");

  // Maak bestand aan met kolomkoppen
  if (!SD.exists(LOG_FILE)) {
    File logFile = SD.open(LOG_FILE, FILE_WRITE);
    if (logFile) {
      logFile.println("SensorID;Datum;Tijd;GedetecteerdObject");
      logFile.close();
    }
  }
  Serial.println("Systeem klaar voor detectie.");
}

void loop() {
  int sensorWaarde = analogRead(SENSOR_PIN);
  bool currentSensorState = (sensorWaarde > LASER_THRESHOLD) ? HIGH : LOW;
  unsigned long currentTime = millis();

  // 1. Straal wordt onderbroken (Flank van HIGH naar LOW)
  if (lastSensorState == HIGH && currentSensorState == LOW) {
    if (!isMeasuring) {
      isMeasuring = true;
      breakCount = 0;
      maxLowDuration = 0;
    }
    breakCount++;
    currentLowStart = currentTime;
    lastInterruptTime = currentTime;
  }

  // 2. Straal is weer hersteld (Flank van LOW naar HIGH)
  if (lastSensorState == LOW && currentSensorState == HIGH) {
    if (isMeasuring) {
      unsigned long lowDuration = currentTime - currentLowStart;
      
      if (lowDuration >= MIN_BREAK_DURATION) {
        if (lowDuration > maxLowDuration) {
          maxLowDuration = lowDuration;
        }
        lastInterruptTime = currentTime;
      } else {
        breakCount--;
      }
    }
  }

  // 3. Meet continu hoe lang een blokkade bezig is
  if (isMeasuring && currentSensorState == LOW) {
    unsigned long currentDuration = currentTime - currentLowStart;
    if (currentDuration > maxLowDuration) {
      maxLowDuration = currentDuration;
    }
  }

  // 4. Evaluatie na timeout
  if (isMeasuring && currentSensorState == HIGH && (currentTime - lastInterruptTime > TIMEOUT_MS)) {
    String objectType = "Onbekend";
    
    if (maxLowDuration > CAR_MIN_DURATION) {
      objectType = "Auto";
    } else if (breakCount >= BIKE_MIN_INTERRUPTS) {
      objectType = "Fietser";
    } else {
      objectType = "Voetganger";
    }

    logData(objectType);

    Serial.print("Detectie voltooid: ");
    Serial.print(objectType);
    Serial.print(" | Breuken: ");
    Serial.print(breakCount);
    Serial.print(" | Max Duur: ");
    Serial.print(maxLowDuration);
    Serial.println("ms");
    Serial.print("Sensorwaarde: ");
    Serial.println(sensorWaarde);

    isMeasuring = false;
  }

  lastSensorState = currentSensorState;
}

void logData(String type) {
  DateTime now = rtc.now();
  
  char dateBuffer[12];
  sprintf(dateBuffer, "%04d-%02d-%02d", now.year(), now.month(), now.day());
  
  char timeBuffer[10];
  sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  String logString = SENSOR_ID + ";" + String(dateBuffer) + ";" + String(timeBuffer) + ";" + type;

  File logFile = SD.open(LOG_FILE, FILE_WRITE);
  if (logFile) {
    logFile.println(logString);
    logFile.close();
    Serial.println("Opgeslagen in: " + LOG_FILE);
  } else {
    Serial.println("Fout bij openen: " + LOG_FILE);
  }
}