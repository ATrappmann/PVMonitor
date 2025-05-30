# PVMonitor

**PVMonitor** ist ein *ESP-Projekt* zur Optimierung der Null-Einspeisung eines *Balkonkraftwerks* mit Wechselrichter und Akkuspeicher.

*Null-Einspeisung* bedeutet, dass die gesamte PV-Leistung selbst verbraucht wird und nicht für kleines Geld an den 
Energieversorger abgegeben wird. Dazu steuert der **PVMonitor** die Einspeiseleistung so, dass der Wechselrichter 
rund um die Uhr laufen kann und mit Hilfe des Akkuspeichers auch nachts mindestens die Grundversorgung sicherstellt.
Die dynamische Steuerung der Einspeiseleistung sorgt außerdem auch dafür, dass tagsüber der Akkuspeicher mindestens 
so weit geladen wird, bis die Grundversorgung für die kommende Nacht sichergestellt ist.

## Einführung

Seit 2024 sind in Deutschland bei einem *Balkonkraftwerk* eine maximale PV-Leistung von 2000Wp und 
eine maximale Einspeisung von 800W zugelassen. Der PV-Leistung ist gegenüber der Einspeiseleistung so groß, 
dass man die PV-Module in eine Ost-/West-Ausrichtung aufteilen kann. Dadurch hat man 
im Durchschnitt 1000Wp zur Verfügung und verlängert gleichzeitig die Nutzungsdauer gegenüber einer
reinen Süd-Ausrichtung mit 2000Wp. Ab dem Frühjahr startet die PV-Produktion mit dem Sonnenaufgang im Osten und produziert 
nach ca. 1 Stunde bereits akzeptable Werte. Mittags haben die nach Ost/West ausgerichteten Module durch den schrägen
Sonneneinfall einen schlechten Wirkungsgrad, dafür addieren sich die Erträge auf beiden Seiten. Die West-Seite produziert
dann bis in den späten Abend hinein noch Strom. Da nur eine maximale Einspeisung von 800W erlaubt sind, werden alle
Überschüsse zur Ladung des Akkus verwendet. Der Akku dient dann dazu, die ganze Nacht über den Ruhestrom abzudecken
und tagsüber bei vorbeiziehender Bewölkung als Puffer zu wirken.

Betreibt man ein *Balkonkraftwerk* ohne den **PVMonitor**, kann man mit einem intelligenten Wechselrichter und einem 
Akkuspeicher zunächst einmal auch eine *Null-Einspeisung* realisieren. Das wird im Sommer auch ausreichend sein, denn die
hohe Sonneneinstrahlung und die langen Tage sorgen für ausreichend Überschuss, um den Akkuspeicher immer wieder aufzufüllen.
Im Winter hingegen versucht der Wechselrichter beim Einschalten jedes Großverbrauchers, wie beispielsweise eines Wasserkochers, 
mit der maximalen Einspeiseleistung den Netzbezug zu minimieren. Die führt dazu, dass wegen der geringen PV-Leistung aufgrund 
von reduzierter Sonneneinstrahlung, der Akkuspeicher sofort leergezogen wird. Dies zwingt das Batterie-Management-System (BMS) 
des Akkuspeichers dazu diesen abzuschalten. 
Für den Akkuspeicher ist eine Lagerung im leeren Zustand ungünstig. Trennt man den Wechselrichter vom Netz, dauert der anschließende 
Ladeprozess des Akkuspeichers über die PV-Module bei der geringen Sonneneinstrahlung 3-5 Tage, bevor der Wechselrichter wieder in 
Betrieb genommen werden kann. Innerhalb von kürzester Zeit wiederholt sich dieser Vorgang. Dies kann man nur dadurch abmildern, 
dass man die maximale Einspeiseleistung des Wechselrichters manuell herabsetzt.
Diesen Eingriff in die Konfiguration schließt der **PVMonitor**. Er sammelt die Daten der einzelnen Komponenten des *Balkonkraftwerks* ein und steuert
die maximale Einspeiseleistung des Wechselrichters in Abhängigkeit von PV-Leistung, Verbrauch und Ladezustand des Akkuspeichers
dynamisch.

Zusätzlich stellt der **PVMonitor** über einen *Web-Server* eine Status-Übersicht, eine PV-Ertragshistorie der letzten 30 Tage und über eine JSON-Schnittstelle 
ein Interface bereit. 

![History](/docs/History.png)

Über ein kleines Display wird vom **PVMonitor** separat für die Ost- und West-Seite die aktuelle PV-Eingangsleistung in Watt [W] und die kumulierte Tagesleistung in Wattstunden [Wh] dargestellt.
Außerdem die Batteriespannung und der *State-of-Charge (SOC)* des Akkuspeichers in Wattstunden [Wh].

![OLED](/docs/PVMonitor-OLED.png)

Die JSON-Schnittstelle wird in einem weiteren Projekt **PVMonitor-Display** dazu verwendet, auf einem Display die aktuellen Daten 
an einer beliebigen Stelle im Haus zu visualisieren. So entscheiden wir flexibel, ob es sich lohnt die Spülmaschine oder die Waschmaschine
sofort zu starten oder erst auf die Mittagszeit zu warten, wo tendenziell die höchsten Tageserträge erzielt werden. Die Taste zur Zeitvorwahl
an beiden Geräten hat bei uns auf einmal stark an Bedeutung gewonnen. Auf jeden Fall laufen beide Geräte wegen der begrenzten Einspeiseleistung
des *Balkonkraftwerks* von maximal 800W nur noch nacheinander.

![PVMonitor-Display](/docs/PVMonitor-Display.png)

**Fazit**: Mit einem über den **PVMonitor** gesteuertem *Balkonkraftwerk* erreicht man zwar keine vollständige Autonomie vom Netzbetreiber, 
kann aber gegenüber einer großen PV-Anlage mit einem Bruchteil der Investitionskosten den Netzbezug um bis zu 30-50% senken. Gleichzeitig
wird jede selbst erzeugte KWh auch selbst verwertet, was die Amortisierungszeit deutlich verkürzt.

## Verwendete Komponenten

Für die PV-Module in Ost-/West-Ausrichtung werden zwei MPPT-Laderegler von [Victron](https://www.victronenergy.de/solar-charge-controllers/) 
eingesetzt. Das genaue Modell wird in Abhängigkeit der verwendeten PV-Module über die Seite https://www.victronenergy.com/mppt-calculator
ermittelt.

Als Wechselrichter habe ich den [800W Lumentree](https://www.lumentree-portal.de/produkt/lumentree-sun-800/) Wechselrichter 
mit [Trucki-Stick](https://www.lumentree-shop.de/produkt/trucki-t2sg-stick-fuer-lumentree-sun/) verwendet. Der Lumentree basiert 
auf den blauen Sun-Wechselrichtern und enthält bereits die Steuerungsplatinen und ein WiFi-Gateway. Über den YouTube-Kanal
von [@DerKanal](https://www.youtube.com/@DerKanal) bin ich auf diese Geräte aufmerksam geworden. In dem Video [Nulleinspeisung mit Lumentree feat. Trucki](https://www.youtube.com/watch?v=SNDrvWKNmys) wird die Funktion ausführlich erklärt. 
Die Funktion des *Trucki-Stick* ist zudem unter https://github.com/trucki-eu/Trucki2Shelly-Gateway ausführlich beschrieben.

Zur Energiemessung des Hausverbrauchs wird ein [Shelly 3EM](https://www.shelly.com/de/products/shelly-3em) verwendet. 
Die Einspeiseleistung wird mit einem [Shelly Plug S](https://www.shelly.com/de/products/shelly-plug-s-gen3) gemessen. 
Der Ladezustand des Akkus wird mit dem Hall-Effekt Stromsensor [WCS1800](https://www.google.com/search?q=wcs1800) überwacht.

Durch die aktive Steuerung des **PVMonitor** werden all diese Komponenten miteinander verbunden und dafür
gesorgt, dass der Akku nicht leerläuft. Tagsüber wird dynamisch so lange Strom von den PV-Modulen zur Ladung des Akkus
abgezweigt, bis der Ladezustand ausreichend ist, um nachts mindestens die Ruheleistung der Verbraucher abzudecken.
Dies geschieht durch aktive Steuerung des Trucki-Sticks, der ansonsten dafür verantwortlich ist, immer nur genau den Anteil über den 
Inverter einzuspeisen, der im Haus gerade verbraucht wird.

Als Akkuspeicher werden zwei [50Ah 51.2V LiFePO4-Akkus](https://glceenergy.com/de/products/48v-50ah-lifepo4-battery-mini) verwendet.
Diese sind parallel angeschlossen, so dass sich eine Gesamtkapazität von 5,12 KWh ergibt. Im Sommer werden bei gutem Wetter täglich ca. 10KWh 
durch die PV-Module erzeugt, von denen ungefähr die Hälfte direkt verbraucht werden kann. Der Überschuss kann somit vollständig eingelagert werden.

Seit der Inbetriebnahme habe ich es so geschafft, keine einzige KWh mehr einzuspeisen!

## Schaltplan

Der Anschluss der PV-Komponenten, des Wechselrichters und Akkuspeichers erfolgt in einer kleinen Unterverteilung mit Leitungsschutzschaltern und Überspannungsschutz.
Der Schaltplan dazu sieht wie folgt aus:

![Schaltplan](/docs/Schaltplan.png)

Der **PVMonitor** ist auf einem Steckbrett schnell montiert:

![PVMonitor](/docs/PVMonitor.png)

Es werden folgende Komponenten benötigt:
* ESP8266 (D1 mini clone)
* OLED, 128x32 pixel, I2C, 0,96"
* 4 Kanal bidirektionale 3.3V-5V Pegelwandler
* WCS1800
* kurze Jumper-Kabel (male-male)
* (optional) 1 Mikro-Taster als Restart-Button

Über Jumper-Kabel werden folgende Verbindungen hergestellt:

![Connections](/docs/Connections.png)
  
## Funktion der aktiven Komponenten

### Shelly 3EM
Mit dem Einbau des 3-Phasen Energiemesser *Shelly 3EM* in den Zählerschrank der Hauptverteilung beginnt alles. Hier lohnt es sich auch gleich
festzuhalten, welche Verbraucher/Räume auf welcher Phase liegen. Über die Shelly-App oder das [Shelly Cloud-Interface](https://control.shelly.cloud/) kann man so
zunächst einmal den Grundverbrauch ermitteln und den ein oder anderen Großverbraucher ausfindig machen. Eine vergessene alte 100W-Glühlampe im Keller oder
in der Garage ist so schnell gefunden und ausgetauscht. Bei einer Tiefkühltruhe, die sich regelmäßig an- und ausschaltet, lohnt es sich den Verbrauch
zu notieren und diese dann einmal vollständig abzutauen und das Gitter des Wärmetauschers zu säubern. So spart man schnell 100W in den Spitzen ein.

Ist das *Balkonkraftwerk* dann wie hier beschrieben vollständig installiert, wird der *Shelly 3EM* vom **PVMonitor** dazu verwendet, den aktuellen
Hausverbrauch zu ermitteln. Dieser wird über die URL `http://<IP-Adresse>/status` im Sekundentakt abgefragt. Aus der Antwort im JSON-Format wird der Parameter *total_power* 
extrahiert, welcher den aktuellen Gesamtverbrauch des Hauses widerspiegelt. 

### MPPT-Laderegler
Die beiden MPPT-Laderegler von Victron werden vom **PVMonitor** über die seriellen [*VE.direct*-Schnittstelle](https://www.victronenergy.com/upload/documents/VE.Direct-Protocol-3.34.pdf)
ausgelesen. Die passenden Stecker für die Schnittstelle findet man, wenn man nach [JST PH2.0 4-pin](https://de.aliexpress.com/w/wholesale-jst-PH2.0-4pin.html) sucht.
Ich habe gleich fertig konfektionierte Stecker mit Kabel verwendet, um mir das crimpen zu sparen.

Von den so bereitgestellten Daten werden folgende Informationen ausgewertet:
* Der *Tracker Mode* (OFF, MPPT) zur Tag/Nacht-Erkennung verwendet. 
* Der *Operation State* (Bulk, Absorption, Float) wird zur Kontrolle des Ladezustands der Batterie verwendet.
* Die Summe der *Panel Power* von beiden Reglern ergibt die maximale Einspeiseleistung, die aktuell zur Verfügung steht.

### Shelly Plug S
Der Wechselrichter ist über einen *Shelly Plug S* ans Stromnetz angeschlossen.  Dieser wird vom **PVMonitor** über die 
URL `http://<IP-Adresse>/status` im Sekundentakt abgefragt. Aus der Antwort im JSON-Format wird der Parameter *power* 
extrahiert, welcher der aktuellen Einspeiseleistung des Inverters entspricht.
Weiterhin wird der Parameter *ison* extrahiert, um zu prüfen, ob das Relay im Shelly den Inverter vom Stromnetz getrennt hat.
Dieses Relay wird außerhalb dieses Projektes von der Hausautomation gesteuert. Die Überwachung des Relais dient zur 
Optimierung des Algorithmus des **PVMonitor**, da die PV-Leistung nun vollständig zur Ladung des Akkuspeichers verwendet wird.

### WCS1800
Über den Hall-Effekt Stromsensor *WCS1800* wird permanent der Strom gemessen, mit der der Akku geladen bzw. entladen wird.
Durch Multiplikation der Batteriespannung mit dem Strom und der vergangenen Zeit zwischen zwei Messungen wird durch Aufsummierung
der Ladezustand des Akkus ermittelt. Die Genauigkeit des Sensors ist nicht so gut wie die eines aktiven Mess-Shunts, dieser
kostet dafür aber auch nur einen Bruchteil. Der Messfehler wird in Abhängigkeit von der Batteriespannung und des *Operation State* der MPPT-Laderegler
bei der Über- und Unterschreitung von Schwellwerten korrigiert, was für den hier vorgesehenen Einsatz vollkommen ausreichend ist.

### Trucki-Stick (T2SG)
Aus dem *Trucki-Stick* wird über die URL `http://<IP-Adresse>/json` der Parameter *MAXPOWER* ausgelesen. Dieser 
Wert gibt die maximale Einspeiseleistung des Wechselrichters an.

Über die URL `http://<IP-Adresse>/?maxPower=<wert>` kann beim *Trucki-Stick* die maximal vom Wechselrichter erzeugte
Einspeiseleistung vorgegeben werden. Dieser Aufruf wird vom **PVMonitor** verwendet, um über den Tag hinweg dafür zu sorgen, dass von 
der *Panel Power* immer etwas abgezweigt wird, bis eine Ladekapazität des Akkus erreicht wurde, die die Grundversorgung der Verbraucher 
über die Nacht hinweg abdeckt.


## Die Software
Die aktiven Komponenten werden über separate Klassen und Funktionen gesteuert und verwaltet.

### Die Klasse *Victron*
Die Klasse *Victron* ist in der Datei `Victorn.h` definiert und verwaltet die Abfrage der MPPT-Laderegler und dekodiert das serielle [*VE.direct*-Protokoll](https://www.victronenergy.com/upload/documents/VE.Direct-Protocol-3.34.pdf).

Für jeden Laderegler wird separat ein Objekt mit der Nummer des Empfangspins für die serielle
Kommunikation mit dem Laderegler angelegt:
```#include "Victron.h"
Victron victron1(VICTRON_WEST_RXPIN, -1);  // RX, no TX - WEST side
Victron victron2(VICTRON_EAST_RXPIN, -1);  // RX, no TX - EAST side
```

In der Funktion `setup()` werden diese Objekte dann wie folgt initalisiert:
```
  victron1.init();
  while (!victron1.update()); // wait for initial values
  veLastTrackerModeWest = victron1.getTrackerMode();
  printf("Victron1 (West): %s\n", victron1.getTrackerModeStr());
  
  victron2.init();
  while (!victron2.update()); // wait for initial values
  veLastTrackerModeEast = victron2.getTrackerMode();
  printf("Victron2 (East): %s\n", victron2.getTrackerModeStr());
```

In der Funktion `loop()` werden die Attribute kontinuierlich durch fortlaufende Aufrufe der Methode `update()` aktualisiert.

#### float getPanelPower()
Die Methode `getPanelPower()` liefert die aktuelle PV-Leistung in Watt [W] der angeschlossenen PV-Module zurück.

#### VictronTrackerMode getTrackerMode()
Die Methode `getTrackerMode()` liefert einen *enum*-Wert für den *Tracker Mode* des Ladereglers zurück. 

Gültige Werte sind:
* VTM_OFF
* VTM_LIMITED_VA
* VTM_MPPT

#### VictronOperationState getOperationState()
Die Methode `getOperationState` liefert einen *enum*-Wert für den *Operation State* des Ladereglers zurück.

Gültige Werte sind:
* VOS_OFF
* VOS_FAULT
* VOS_BULK
* VOS_ABSORPTION
* VOS_FLOAT
* VOS_EQUALIZE
* VOS_STARTING_UP
* VOS_AUTO_EQUALIZE
* VOS_EXTERNAL_CONTROL

#### float getBatVoltage()
Die Methode `getBatVoltage` liefert die aktuelle Batteriespannung in Volt [V] zurück.

#### float getYieldToday()
Die Methode `getYieldToday` liefert die bisherige PV-Tagesleistung in Wattstunden [Wh] zurück.

#### float getYieldYesterday()
Die Methode `getYieldYesterday` liefert die PV-Vortagesleistung in Wattstunden [Wh] zurück.


### Die *Shelly*-Funktionen
Die Abfrage der *Shelly*-Komponenten erfolgt zustandslos, so dass die Ansteuerung über Funktionen implementiert
sind, die in der Datei `Shelly.h` definiert sind.

#### bool getShellyStatus((String ip, ShellyStatus *status)
Über die Funktion `bool getShellyStatus(String ip, ShellyStatus *status)` werden per HTTP-Aufruf die Shelly-Komponenten abgefragt.

Die benötigten Parameter werden in dem Objekt `ShellyStatus` zurückgegeben und enthalten folgende Attribute:
```
struct ShellySetup {
  float power;        // in watt
  float total_power;  // in Wh
  bool  relayOn;      // true if on
};
```

### Die *Trucki-Stick*-Funktionen
Die Abfrage des *Trucki-Stick* erfolgt zustandslos, so dass die Ansteuerung über Funktionen implementiert
sind, die in der Datei `Trucki.h` definiert sind.

#### int getTruckiMaxPower(String ip)
Die Funktion `getTruckiMaxPower(String ip)` liefert den auf dem *Trucki-Stick* als MAXPOWER eingestellten Wert für die maximale Einspeiseleistung des Wechselrichters in Watt [W] zurück.
Ohne den **PVMonitor** liegt dieser Wert fest bei 800W. Der **PVMonitor** setzt diesen Wert allerdings aktiv herab und steuert so dynamisch die Ladekurve des Akkuspeichers.

#### bool setTruckiMaxPower(String ip, uint16_t maxPower)
Die Funktion `setTruckiMaxPower(String ip, uint16_t maxPower)` setzt die maximale Einspeiseleistung des Wechselrichters auf den angegebenen Wert *maxPower* der in Watt [W] übergeben wird.
Die Funktion selbst stellt sicher, dass dieser nicht <50W und >800W sein kann. Ein ungültiger Wert wird entsprechend korrigiert.


### Die Klasse *WCS1800*
Die Klasse *WCS1800* verwaltet die Zugriffe auf den Hall-Effekt Stromsensor *WCS1800* und ist in der Datei `WCS1800.h` definiert.

Hierzu wird ein Objekt angelegt und mit dem analogen Ausgangspin `AOUT` konfiguriert:
```
#include "WCS1800.h"
WCS1800 wcs(CURRENT_SENSOR_PIN);	// AOUT of current monitor
```

In der Funktion `setup()` wird das Objekte dann wie folgt initialisiert:
```
// init mean current value
for (int i=0; i<10; i++) {
   wcs.readCurrent();
}
```

In der Funktion `loop()` wird der Batteriestrom kontinuierlich durch fortlaufende Aufrufe der Methode `readMeanCurrent()` aktualisiert.

#### float readMeanCurrent()
Die Methode `readMeanCurrent()` liefert den gleitenden Mittelwert des Batteriestroms in Ampere [A] über die letzten 10 Messungen.
Hierbei werden Ungenauigkeiten bei der Analog/Digital-Wandlungen des *ESP* über die Zeit ausgeglichen.


### Die Klasse *Display*
Die Klasse *Display* verwaltet die Darstellung auf dem kleinen 0,96"-OLED und ist in der Datei `Display.h` definiert.
Sie dient zur Kapselung des konkret verwendeten Displays vom Rest des Codes. Da die Klasse *Display* von der Klasse *Print* abgeleitet wurde,
ist die Verwendung selbsterklärend.


### Die Klasse *PersistentData*
Die Klasse *PersistentData* verwaltet die historisch gesammelten Daten und ist in der Datei `PersistentData.h` definiert.
Auch wird diese Klasse verwendet, um die im Laufe des Tages kumulierten Werte zu retten, wenn aufgrund von Störungen im WiFi, die aktiven
Komponenten nicht erreicht werden können und der **PVMonitor** sicherheitshalber einen Neustart durchführt.


### Die *Web-Server* Funktionen 
Die *Callback-Funktionen* des Web-Servers sind in der Datei `WebServer.h` definiert.

#### void handleRoot()
Die Funktion `handleRoot()` liefert eine HTML-Seite mit internen Zustandswerten des **PVMonitor** zurück, wenn dieser über sein Web-Interface unter der Adresse `http:://<ip-adresse>/` aufgerufen wird.

#### void handleStatus()
Die Funktion `handleStatus()` liefert einen JSON-String mit den Statuswerten des **PVMonitor** zurück, wenn dieser über sein Web-Interface unter der Adresse `http://<ip-adresse>/status` aufgerufen wird.

### void handleHistory()
Die Funktion `handleHistory()` liefert eine HTML-Seite mit den letzten 30 Tageswerten der PV-Produktion des *Balkonkraftwerks* zurück, wenn der **PVMonitor** über sein Web-Interface unter der Adresse `http:://<ip-adresse>/history` aufgerufen wird.


## Konfiguration

### Kalibrierung des WCS1800
Damit der *WCS1800* Hall-Effekt Sensor die richtigen Daten über den Ladezustand des Akkuspeichers ermittelt, müssen die Parameter zunächst kalibriert werden.

In der Datei `WCS1800-Calibration/WCS1800-Calibration.ino` ist hierzu ein kleiner *Sketch* der zunächst auf den *ESP* hochgeladen werden sollte. Zur Kalibrierung
wird der Akkuspeicher durch Ausschalten des Leitungsschutzschalters vom *Balkonkraftwerk* getrennt. Die folgenden Parameter sollten so lange angepasst werden,
bis diese konstant Stromwerte von 0A ausgeben:

![WCS1800-Calibration](/docs/WCS1800-Calibration.png)

Auch lohnt es sich die tatsächliche Umgebungstemperatur zu messen und den Parameter `WCS_SENS_RATIO` gemäß den Werten aus dem 
[Datenblatt](https://www.winson.com.tw/uploads/images/WCS1800.pdf) des *WCS1800* anzupassen.

Die so ermittelten Werte müssen in die Datei `WCS1800.cpp` übernommen werden. Dies erfolgt durch Kopieren in die gleichlautenden Zeilen:

![WCS1800-Parameter](/docs/WCS1800-Parameter.png)

### Configuration.h
Damit der **PVMonitor** in der eingesetzten Umgebung korrekt funktioniert, müssen nur wenige Konfigurationsparameter in der Datei `Configuration.h` gesetzt werden.

Zunächst werden die jeweiligen IP-Adressen der *Shelly-Komponenten* und des *Trucki-Sticks* eingetragen:

![Config-IPAddresses](/docs/Config-IPAddresses.png)

Als nächstes muss die Gesamtkapazität des Akkuspeichers und die gewünschte Minimalreserve für den Nachtbetrieb angegeben werden:

![Config-BatteryCapacity](/docs/Config-BatteryCapacity.png)

Zuletzt werden noch die Zugangsdaten für das heimische WLAN konfiguriert:

![Config-WiFi](/docs/Config-WiFi.png)

Nun kann der gesamte Code übersetzt werden und auf den *ESP* hochgeladen werden.

## Copyright
**PVMonitor** is written by Andreas Trappmann.
It is published under the MIT license, check LICENSE for more information.
All text above must be included in any redistribution.

## Release Notes

Version 1.0 - 30.05.2025

  * Initial publication.
