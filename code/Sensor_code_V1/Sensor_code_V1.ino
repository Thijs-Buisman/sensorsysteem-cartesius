#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

// --- Pin Configuraties ---
const int SENSOR_PIN = 2; // Anode van fotodiode
const int SD_CS_PIN = 4;  // Chip Select voor SD-kaart

// --- Instellingen ---
const String SENSOR_ID = "0x001b";
const String LOG_FILE = "LOG.CSV";

// --- Drempelwaarden (Thresholds) voor detectie ---
// Pas deze waarden in de praktijk aan op basis van de fysieke opstelling
const unsigned long CAR_MIN_DURATION = 800; // Als de straal >800ms continu breekt -> Auto
const int BIKE_MIN_INTERRUPTS = 4;          // 4 of meer snelle onderbrekingen (spaken) -> Fietser
const unsigned long TIMEOUT_MS = 1500;      // Tijd na de laatste breuk voordat we evalueren

// --- Objecten & Variabelen ---
RTC_DS3231 rtc;
bool isMeasuring = false;
bool lastSensorState = HIGH; // HIGH = laser wordt gezien, LOW = onderbroken

unsigned long currentLowStart = 0;
unsigned long maxLowDuration = 0;
unsigned long lastInterruptTime = 0;
int breakCount = 0;

void setup() {
  Serial.begin(9600);
  pinMode(SENSOR_PIN, INPUT);

  // Initialiseer RTC
  if (!rtc.begin()) {
    Serial.println("RTC niet gevonden!");
    while (1);
  }
  
  if (rtc.lostPower()) {
    Serial.println("RTC heeft stroom verloren, tijd instellen op compileer-tijd!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Initialiseer SD Kaart
  Serial.print("Initialiseren SD-kaart...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(" initialisatie mislukt!");
    while (1);
  }
  Serial.println(" gelukt.");

  // Controleer of logbestand een header heeft, anders aanmaken
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
  bool currentSensorState = digitalRead(SENSOR_PIN);
  unsigned long currentTime = millis();

  // 1. Straal wordt onderbroken (Flank van HIGH naar LOW)
  if (lastSensorState == HIGH && currentSensorState == LOW) {
    if (!isMeasuring) {
      // Start een nieuwe meetsessie
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
      if (lowDuration > maxLowDuration) {
        maxLowDuration = lowDuration;
      }
      lastInterruptTime = currentTime;
    }
  }

  // 3. Meet continu hoe lang een blokkade bezig is (handig voor trage auto's of files)
  if (isMeasuring && currentSensorState == LOW) {
    unsigned long currentDuration = currentTime - currentLowStart;
    if (currentDuration > maxLowDuration) {
      maxLowDuration = currentDuration;
    }
  }

  // 4. Evaluatie en wegschrijven naar SD (Timeout bereikt)
  if (isMeasuring && currentSensorState == HIGH && (currentTime - lastInterruptTime > TIMEOUT_MS)) {
    String objectType = "Onbekend";
    
    // Algoritme logica
    if (maxLowDuration > CAR_MIN_DURATION) {
      objectType = "Auto";
    } else if (breakCount >= BIKE_MIN_INTERRUPTS) {
      objectType = "Fietser";
    } else {
      objectType = "Voetganger";
    }

    logData(objectType);

    // Print naar Serial voor debug
    Serial.print("Detectie voltooid: ");
    Serial.print(objectType);
    Serial.print(" | Breuken: ");
    Serial.print(breakCount);
    Serial.print(" | Max Duur: ");
    Serial.print(maxLowDuration);
    Serial.println("ms");

    // Reset voor de volgende passage
    isMeasuring = false;
  }

  lastSensorState = currentSensorState;
}

// Functie om de data naar de SD kaart te schrijven in CSV formaat
void logData(String type) {
  DateTime now = rtc.now();
  
  // Formatteer de datum
  char dateBuffer[12];
  sprintf(dateBuffer, "%04d-%02d-%02d", now.year(), now.month(), now.day());
  
  // Formatteer de tijd
  char timeBuffer[10];
  sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // Construeer de CSV regel: SensorID; Datum; Tijd; GedetecteerdObject
  String logString = SENSOR_ID + ";" + String(dateBuffer) + ";" + String(timeBuffer) + ";" + type;

  // Schrijf naar SD
  File logFile = SD.open(LOG_FILE, FILE_WRITE);
  if (logFile) {
    logFile.println(logString);
    logFile.close();
  } else {
    Serial.println("Fout bij openen LOG.CSV");
  }
}