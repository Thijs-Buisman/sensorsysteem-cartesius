/*
 * =============================================================================
 * Sensor_code_V3.ino
 * Auteur      : Thijs Buisman
 * Versie      : 3.0
 * Datum       : 2026
 * -----------------------------------------------------------------------------
 * Beschrijving:
 *   Geautomatiseerd passantenmeetsysteem voor de buitenruimte van wijk
 *   Cartesius. Het systeem detecteert onderbrekingen van een laserstraal
 *   via een fotodiode en classificeert passanten als voetganger, fietser
 *   of auto op basis van onderbrekingspatronen. Data wordt met tijdstempel
 *   weggeschreven naar een CSV-bestand op een SD-kaart.
 *
 * Hardware:
 *   - Arduino Nano
 *   - Fotodiode op pin A3 met 10kOhm pull-down weerstand
 *   - RTC DS3231 op I2C (SDA=A4, SCL=A5)
 *   - SD-kaartmodule op SPI (MOSI=D11, MISO=D12, SKC=D13, CS=D4)
 *
 * Versiebeheer:
 *   V1 - Eerste werkende integratie, digitalRead op D2
 *   V2 - analogRead op A3, drempelwaarde, dynamische bestandsnaam
 *   V3 - Modulaire opbouw, C-coding richtlijnen, functieprototypes
 * =============================================================================
 */

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

/* --- Pin definities --- */
#define SENSOR_PIN      A3
#define SD_CS_PIN       4

/* --- Sensor drempelwaarden --- */
#define LASER_THRESHOLD     25    /* Analoge waarde waarboven laser aanwezig is  */
#define MIN_BREAK_DURATION  20    /* Minimale breukduur in ms, korter = ruis     */

/* --- Detectie instellingen --- */
#define CAR_MIN_DURATION    800   /* Breukduur > 800ms = auto                    */
#define BIKE_MIN_INTERRUPTS 4     /* 4 of meer breuken = fietser                 */
#define TIMEOUT_MS          1500  /* Wachttijd na laatste breuk voor evaluatie   */

/* --- Overige instellingen --- */
#define SENSOR_ID   "0x001b"
#define CSV_HEADER  "SensorID;Datum;Tijd;GedetecteerdObject"

/* --- Globale variabelen --- */
RTC_DS3231    rtc;
char          gLogFile[13];
bool          gIsMeasuring;
bool          gLastSensorState;
unsigned long gCurrentLowStart;
unsigned long gMaxLowDuration;
unsigned long gLastInterruptTime;
int           gBreakCount;

/* --- Functieprototypes --- */
void   initRTC(void);
void   initSD(void);
void   initLogFile(void);
bool   readSensor(void);
void   handleBreakStart(unsigned long currentTime);
void   handleBreakEnd(unsigned long currentTime);
void   updateMaxDuration(unsigned long currentTime);
bool   checkTimeout(unsigned long currentTime);
String classifyObject(void);
void   logData(String type);
void   resetMeting(void);

/*
 * setup()
 * Initialiseert alle hardware modules bij opstarten.
 * Wordt eenmalig uitgevoerd na inschakelen of reset.
 */
void setup()
{
    Serial.begin(9600);
    gIsMeasuring       = false;
    gLastSensorState   = HIGH;
    gCurrentLowStart   = 0;
    gMaxLowDuration    = 0;
    gLastInterruptTime = 0;
    gBreakCount        = 0;
    initRTC();
    initSD();
    initLogFile();
    Serial.println("Systeem klaar voor detectie.");
}

/*
 * loop()
 * Hoofdlus: leest sensor, verwerkt flanken en evalueert na timeout.
 */
void loop()
{
    bool          currentSensorState = readSensor();
    unsigned long currentTime        = millis();

    if (gLastSensorState == HIGH && currentSensorState == LOW)
        handleBreakStart(currentTime);

    if (gLastSensorState == LOW && currentSensorState == HIGH)
        handleBreakEnd(currentTime);

    if (gIsMeasuring && currentSensorState == LOW)
        updateMaxDuration(currentTime);

    if (gIsMeasuring && currentSensorState == HIGH && checkTimeout(currentTime))
    {
        String objectType = classifyObject();
        logData(objectType);
        resetMeting();
    }

    gLastSensorState = currentSensorState;
}

/*
 * initRTC()
 * Initialiseert de RTC DS3231 klokmodule via I2C.
 * Bij stroomverlies wordt tijd ingesteld op compileer-tijdstip.
 */
void initRTC(void)
{
    if (!rtc.begin())
    {
        Serial.println("FOUT: RTC niet gevonden!");
        while (1);
    }
    if (rtc.lostPower())
    {
        Serial.println("RTC: stroom verloren, tijd hersteld.");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

/*
 * initSD()
 * Initialiseert de SD-kaartmodule via SPI.
 * Stopt het programma als de module niet gevonden wordt.
 */
void initSD(void)
{
    Serial.print("SD-kaart initialiseren...");
    if (!SD.begin(SD_CS_PIN))
    {
        Serial.println(" MISLUKT!");
        while (1);
    }
    Serial.println(" gelukt.");
}

/*
 * initLogFile()
 * Maakt logbestand aan met naam op basis van datum en tijd.
 * Bestandsnaam formaat: MMDDHHII.CSV (FAT32 max 8.3)
 */
void initLogFile(void)
{
    DateTime now = rtc.now();
    sprintf(gLogFile, "%02d%02d%02d%02d.CSV",
        now.month(), now.day(), now.hour(), now.minute());
    Serial.print("Logbestand: ");
    Serial.println(gLogFile);
    if (!SD.exists(gLogFile))
    {
        File logFile = SD.open(gLogFile, FILE_WRITE);
        if (logFile)
        {
            logFile.println(CSV_HEADER);
            logFile.close();
        }
    }
}

/*
 * readSensor()
 * Leest analoge waarde fotodiode, vergelijkt met drempelwaarde.
 * Returnvalue: TRUE als laser aanwezig, FALSE als onderbroken
 */
bool readSensor(void)
{
    int waarde = analogRead(SENSOR_PIN);
    return (waarde > LASER_THRESHOLD) ? HIGH : LOW;
}

/*
 * handleBreakStart()
 * Verwerkt het moment waarop de laserstraal wordt onderbroken.
 * Arguments: currentTime - huidige millis() waarde
 */
void handleBreakStart(unsigned long currentTime)
{
    if (!gIsMeasuring)
    {
        gIsMeasuring    = true;
        gBreakCount     = 0;
        gMaxLowDuration = 0;
    }
    gBreakCount++;
    gCurrentLowStart   = currentTime;
    gLastInterruptTime = currentTime;
}

/*
 * handleBreakEnd()
 * Verwerkt het moment waarop de laserstraal herstelt.
 * Breuken korter dan MIN_BREAK_DURATION worden als ruis genegeerd.
 * Arguments: currentTime - huidige millis() waarde
 */
void handleBreakEnd(unsigned long currentTime)
{
    if (!gIsMeasuring)
        return;

    unsigned long lowDuration = currentTime - gCurrentLowStart;

    if (lowDuration >= MIN_BREAK_DURATION)
    {
        if (lowDuration > gMaxLowDuration)
            gMaxLowDuration = lowDuration;
        gLastInterruptTime = currentTime;
    }
    else
        gBreakCount--;
}

/*
 * updateMaxDuration()
 * Houdt maximale breukduur bij tijdens actieve onderbreking.
 * Arguments: currentTime - huidige millis() waarde
 */
void updateMaxDuration(unsigned long currentTime)
{
    unsigned long currentDuration = currentTime - gCurrentLowStart;
    if (currentDuration > gMaxLowDuration)
        gMaxLowDuration = currentDuration;
}

/*
 * checkTimeout()
 * Controleert of wachttijd na laatste breuk verstreken is.
 * Returnvalue: TRUE als timeout bereikt, anders FALSE
 * Arguments:  currentTime - huidige millis() waarde
 */
bool checkTimeout(unsigned long currentTime)
{
    return (currentTime - gLastInterruptTime > TIMEOUT_MS);
}

/*
 * classifyObject()
 * Classificeert gedetecteerd object op basis van breukpatroon.
 * Returnvalue: String "Auto", "Fietser" of "Voetganger"
 */
String classifyObject(void)
{
    String objectType;

    if (gMaxLowDuration > CAR_MIN_DURATION)
        objectType = "Auto";
    else if (gBreakCount >= BIKE_MIN_INTERRUPTS)
        objectType = "Fietser";
    else
        objectType = "Voetganger";

    Serial.print("Detectie: ");   Serial.print(objectType);
    Serial.print(" | Breuken: "); Serial.print(gBreakCount);
    Serial.print(" | Max duur: "); Serial.print(gMaxLowDuration);
    Serial.println("ms");

    return objectType;
}

/*
 * logData()
 * Schrijft detectieregel naar CSV-logbestand op SD-kaart.
 * Formaat: SensorID;Datum;Tijd;GedetecteerdObject
 * Arguments: type - gedetecteerd objecttype als String
 */
void logData(String type)
{
    DateTime now = rtc.now();
    char dateBuffer[12];
    char timeBuffer[10];

    sprintf(dateBuffer, "%04d-%02d-%02d", now.year(), now.month(), now.day());
    sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    String logString = String(SENSOR_ID) + ";" + dateBuffer + ";" + timeBuffer + ";" + type;

    File logFile = SD.open(gLogFile, FILE_WRITE);
    if (logFile)
    {
        logFile.println(logString);
        logFile.close();
        Serial.println("Opgeslagen in: " + String(gLogFile));
    }
    else
        Serial.println("FOUT: kan bestand niet openen: " + String(gLogFile));
}

/*
 * resetMeting()
 * Reset alle metvariabelen voor de volgende passage.
 */
void resetMeting(void)
{
    gIsMeasuring = false;
}