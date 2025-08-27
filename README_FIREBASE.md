# Firebase Database Structuur voor ESP32 Poolcontroller

Deze README beschrijft de structuur van de Firebase Realtime Database zoals gebruikt door de ESP32 poolcontroller. Dit overzicht is bedoeld als referentie voor het ontwikkelen van een Flutter app die met deze database communiceert.

## Hoofdstructuur
De database is opgebouwd rond een hoofdknooppunt per apparaat (device). Elk apparaat heeft een unieke ID (bijvoorbeeld het serienummer of een door de ESP32 gegenereerde naam).

```
root
│
├── devices
│   ├── <device_id_1>
│   │    ├── status: {...}
│   │    ├── relays: {...}
│   │    ├── sensors: {...}
│   │    ├── settings: {...}
│   │    └── lastUpdate: <timestamp>
│   └── <device_id_2>
│        └── ...
└── users
    └── ...
```

## Devices
Onder `devices` staat per apparaat een eigen knooppunt. Bijvoorbeeld:

```
devices/
  esp32pool01/
    status/
    relays/
    sensors/
    settings/
    lastUpdate
```

### status
Bevat algemene statusinformatie van het apparaat:
```json
{
  "online": true,
  "firmwareVersion": "1.2",
  "uptime": 123456,
  "ip": "192.168.1.100"
}
```

### relays
De huidige status van de relais:
```json
{
  "relay1": true,
  "relay2": false,
  "relay3": true,
  ...
}
```

### sensors
De actuele waarden van de analoge en digitale ingangen:
```json
{
  "temperature": 23.5,
  "ph": 7.1,
  "orp": 650,
  "waterlevel": 1,
  ...
}
```

### settings
Instellingen die door de gebruiker of app kunnen worden aangepast:
```json
{
  "targetTemperature": 28.0,
  "phSetpoint": 7.0,
  "orpSetpoint": 700,
  ...
}
```

### lastUpdate
Tijdstip (timestamp) van de laatste update door het apparaat.

## users
Optioneel: gebruikersstructuur voor authenticatie of rechtenbeheer.

## Opmerkingen
- Alle knooppunten en veldnamen zijn voorbeelden; pas deze aan aan je eigen project.
- Gebruik altijd authenticatie en beveiliging in je Firebase project.
- De ESP32 schrijft en leest alleen in zijn eigen device-knooppunt.

## Voorbeeld database (JSON export)
```json
{
  "devices": {
    "esp32pool01": {
      "status": {
        "online": true,
        "firmwareVersion": "1.2",
        "uptime": 123456,
        "ip": "192.168.1.100"
      },
      "relays": {
        "relay1": true,
        "relay2": false
      },
      "sensors": {
        "temperature": 23.5,
        "ph": 7.1
      },
      "settings": {
        "targetTemperature": 28.0
      },
      "lastUpdate": 1692950400
    }
  }
}
```

---

Gebruik deze structuur als basis voor je Flutter app. Je kunt eenvoudig per device de status, sensoren, relais en instellingen uitlezen en aanpassen.
