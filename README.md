# Sensorsysteem – Wijk Cartesius, Utrecht

> Automatisch passantentelsysteem op basis van een lasersensor met fotodiode.  
> Ontwikkeld in het kader van het QUEST-project **"Gezond leven in Cartesius"** – Hogeschool Utrecht, 2026.

---

## Wat doet dit systeem?

Dit systeem telt automatisch hoeveel voetgangers, fietsers en auto's er door een doorgang in de wijk Cartesius (Utrecht) passeren. De data wordt lokaal opgeslagen op een SD-kaart als CSV-bestand, voorzien van een tijdstempel via een RTC-module.

Het systeem bestaat uit twee onderdelen:
- **Zender** – een lasermodule die een onzichtbare laserstraal uitzendt
- **Ontvanger** – een fotodiode die registreert wanneer de laserstraal wordt onderbroken

Wanneer iemand de straal onderbreekt, classificeert het systeem de passage als voetganger, fietser of auto op basis van de duur en het patroon van de onderbreking.

---

## Resultaten veldtest (21 mei 2026)

| Gebruikerstype | Sensor | Handmatig | Afwijking |
|----------------|--------|-----------|-----------|
| Voetganger     | 50     | 47        | 6,4%      |
| Fietser        | 28     | 32        | 12,5%     |
| Auto           | 3      | 0         | –         |
| **Totaal**     | **81** | **79**    | **2,5%**  |

Totale afwijking: **2,5%** — ruim binnen de gestelde eis van maximaal 15%.

---

## Inhoud van deze repository

```
sensorsysteem-cartesius/
│
├── code/
│   ├── Sensor_code_V3.ino       # Definitieve sensorcode (gebruik deze)
│   └── Sensor_code_V2.ino       # Vorige versie, ter referentie
│
├── kicad/
│   ├── PCB_sensor/              # KiCad 9.0 project – sensorprintplaat (ontvanger)
│   └── PCB_laser/               # KiCad 9.0 project – laserprintplaat (zender)
│
├── 3d-ontwerp/                    # STL-bestanden en .itp autodesk inventor van alle 7 behuizingsonderdelen
│
├── testdata/
│   ├── 05201342.CSV        # Ruwe sensordata van de veldtest
│   └── Sensor_validatie_test.xlsx     # Vergelijking sensor vs. handmatige telling
│
└── docs/
    ├── Overdracht_Sensorsysteem_Cartesius.pdf   # Overdrachts document (begin hier)
    ├── Handleiding_Sensorprototype.pdf
    ├── Proof_of_Concept_Sensorsysteem.pdf
    └── PCB-ontwerp_sensor_V1.pdf
```

---

## Benodigde hardware

| Component | Type | Kosten (ca.) |
|-----------|------|-------------|
| Microcontroller | Arduino Nano | €4,00 |
| Lasermodule | KY-008 | €1,50 |
| Fotodiode | SGPD5051C6 | €3,50 |
| RTC-module | DS3231 | €4,00 |
| SD-kaartmodule | standaard SPI | €2,50 |
| Protoboard + bedrading | – | €5,00 |
| 3D-print materiaal | PLA, 573 gram | €12,00 |
| Powerbanks (2x) | 5V USB | €20,00 |
| **Totaal** | | **~€83,92** |

---

## Benodigde software

- [Arduino IDE](https://www.arduino.cc/en/software) – voor de sensorcode
- [KiCad 9.0](https://www.kicad.org/) – voor de PCB-schema's en layouts
- [PrusaSlicer](https://www.prusa3d.com/page/prusaslicer_424/) of [Cura](https://ultimaker.com/software/ultimaker-cura/) – voor de 3D-print bestanden
- [Autodesk Inventor](https://www.autodesk.com/nl/products/inventor/overview) - 3D-modelleringssoftware voor ontwerpers en ingenieurs

---

## Snel aan de slag

1. Download of clone deze repository
2. Open `docs/Overdrachts_document_Sensorsysteem_Cartesius.docx` — dit is het startpunt
3. Upload `code/Sensor_code_V3.ino` via Arduino IDE naar een Arduino Nano
4. Volg de plaatsingsinstructies in `docs/Handleiding_Sensorprototype.docx`

---

## Project & opleiding

| | |
|---|---|
| **Project** | Gezond leven in Cartesius |
| **Opdrachtgever** | Hanneke Kruize – Lectoraat Building Future Cities (RAAK PRO) |
| **Opleiding** | GCL/IDE · QUEST 30EC · Hogeschool Utrecht |
| **Auteur** | Thijs Buisman · Studentnummer 1855662 |
| **Groep** | Groep 8 (Boas Janssen, Joost, Kim, Melisa, Stef, Thijs Buisman) |
| **Docentcoach** | Marit Béguin |
| **Jaar** | 2025–2026 |
