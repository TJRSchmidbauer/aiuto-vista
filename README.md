# Aiuto-Vista

Aiuto-Vista verwandelt ein Cheap Yellow Display (CYD, ESP32-2432S028) in einen
praktischen Oszilloskop-Trainer. Es erzeugt echte 3,3-V-Digitalsignale, stellt
zufällige Messaufgaben und zeigt die erwarteten Werte auf Wunsch an.

**Ursprung:** Aiuto-Vista ist ein **Fork von ScopeBuddy** von Johannes Börnsen
(https://github.com/johannesboernsen/ScopeBuddy). Die Idee, die Lektionen, die
Messaufgaben und große Teile des Codes stammen aus dem Originalprojekt, das
für das Elecrow CrowPanel Advance 5″ (ESP32-P4) entwickelt wurde. Dieser Fork
portiert ScopeBuddy auf das günstige CYD-Touchdisplay (ESP32) und ist nur
gemeinsam mit dem Original zu verstehen.

Firmware 0.6.0 bietet 15 Ein- und Zweikanal-Lektionen, Touch-Kalibrierung und
die Bedienung über den CYD-Touchscreen.

## Was du brauchst

| Komponente | Anforderung | Hinweise |
|---|---|---|
| Display | Cheap Yellow Display (CYD) `ESP32-2432S028`, 320 × 240, Touchscreen | USB-C oder Micro-USB je nach Revision |
| Oszilloskop | Ein Kanal für die Grundlektionen, zwei Kanäle für alle Lektionen | Hochohmige Eingänge und Tastköpfe für 3,3-V-Logik verwenden |
| USB-Kabel | Datenkabel zum Flashen der Firmware | Reine Ladekabel funktionieren nicht |

## Anschluss

- **CH1-Signal:** `GPIO26` (Audio-In-/Links-Pin der 3,5-mm-Buchse)
- **CH2-Signal:** `GPIO27` (CN1-Header)
- **Masse:** `GND`-Pin am CN1-Header (die Buchsenhülse führt ebenfalls GND)

GPIO26 und GPIO27 sind 3,3-V-Logikausgänge, die nur für hochohmige
Oszilloskop- oder Logikanalysator-Eingänge gedacht sind. Schließe keine
Lasten oder externen Spannungen an diese Pins an.

## Installieren

Der einfachste Weg ist die vorgefertigte Firmware. Danach muss der
Serienport deines Geräts bekannt sein (z. B. `/dev/ttyUSB0` unter Linux):

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  write-flash -z 0x0 docs/firmware/aiuto-vista-v0.6.0.bin
```

Das gemergte Image enthält Bootloader, Partitions-Tabelle und Anwendung in
einem Schritt. Die Firmware-Datei liegt in `docs/firmware/`. Der Web-Installer
in `docs/` wird über GitHub Pages ausgeliefert.

## Erster Hardware-Check

Über **Einstellungen → Diagnose** stehen zwei Tests zur Verfügung:

1. **Einkanal-Test:** `GPIO26` erzeugt 1 kHz bei 50 % Tastgrad.
2. **Zweikanal-Test:** `GPIO26` und `GPIO27` erzeugen 1 kHz bei 50 %, wobei
   `GPIO27` dem Kanal 1 mit 100 µs Verzögerung folgt.

Details zu den erwarteten Messwerten stehen in der
[Hardware-Validierung](docs/HARDWARE_VALIDATION.md).

## Touch-Kalibrierung

Da die XPT2046-Touchscreens der CYD-Boards von Exemplar zu Exemplar abweichen,
kann der Touch unter **Einstellungen → Touch-Kalibrierung** individuell
abgeglichen werden: Fünf Kreuze nacheinander antippen, die Kalibrierung wird
gerätespezifisch im NVS-Speicher des ESP32 abgelegt.

## Lektionen

Eine Lektion startet eine offene Serie zufälliger Aufgaben. Aiuto-Vista
wiederholt die zehn zuletzt erzeugten Signalkonfigurationen einer Serie nicht.
Die Rückfrage vor dem Zurückkehren zur Startseite lässt sich in den
Einstellungen abschalten.

Einkanal-Lektionen auf GPIO26:

- Periodisches Signal
- Pulsbreiten
- Burst
- Pulslücke
- Servosignal
- Tachosignal
- Tasterprellen
- UART 8N1
- Zustandswechsel

Zweikanal-Lektionen auf GPIO26 und GPIO27:

- Trigger-Antwort
- Phasenverschiebung
- Frequenzteiler
- Ultraschall-Echo
- Freigegebene PWM
- Quadraturgeber

## Aus dem Quellcode bauen

Aiuto-Vista 0.6.0 ist mit ESP-IDF 5.4.2 gebaut und auf dem CYD getestet.
ESP-IDF installieren und aktivieren, dann:

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

Der Component Manager von ESP-IDF lädt die benötigten LVGL-, Display- und
Touch-Komponenten automatisch herunter. `dependencies.lock` fixiert die
Versionen für Release-Builds.

## Lizenz und rechtliche Hinweise

Dieser Fork basiert auf den **Original-Beiträgen von ScopeBuddy**, die unter
der [ScopeBuddy Community License 1.0](LICENSE.md) stehen. Demnach darfst du
den Code bauen, studieren, verändern und veröffentlichen, auch für Bildung und
Forschung. Veröffentlichte veränderte Versionen müssen den zugehörigen
Quellcode bereitstellen, einen **Verweis auf das Originalprojekt** enthalten
und einen **eigenen Namen** tragen — dieser Fork erfüllt das mit dem Namen
Aiuto-Vista und den Verweisen auf
https://github.com/johannesboernsen/ScopeBuddy.

ScopeBuddy-Geräte und -Kits dürfen zum Selbstkostenpreis weitergegeben werden.
Ein gewinnorientierter Verkauf erfordert eine separate kommerzielle Lizenz vom
Inhaber des Original-Repositorys. ScopeBuddy — und damit auch dieser Fork —
ist *source-available*, aber nicht Open Source im Sinne der Open Source
Initiative.

Von Elecrow abgeleitetes Board-Support-Material und andere
Drittanbieter-Komponenten werden durch ScopeBuddy nicht um-lizenziert. Deren
Status und die genauen Lizenzgrenzen sind in
[LICENSE.md](LICENSE.md) und [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
dokumentiert.

## Ursprung und Danksagung

Alle Lektionen, Messaufgaben, das UI-Konzept und wesentliche
Software-Teile stammen aus **ScopeBuddy von Johannes Börnsen**
(https://github.com/johannesboernsen/ScopeBuddy). Ohne dieses Projekt gäbe es
Aiuto-Vista nicht. Bitte unterstütze das Originalprojekt; Beiträge richten
sich an das Original und folgen dessen [CONTRIBUTING.md](CONTRIBUTING.md) mit
dem dortigen [CLA](CLA.md).

## Weitere Dokumentation

- [Hardware-Validierung und Diagnose](docs/HARDWARE_VALIDATION.md)
- [Wartung und Release-Prozess](docs/MAINTAINING.md)
- [Drittanbieter-Hinweise](THIRD_PARTY_NOTICES.md)
