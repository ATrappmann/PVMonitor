# PV-Monitor

**PV-Monitor** ist ein *ESP-Projekt* zur Optimierung der Null-Einspeisung eines Balkonkraftwerks mit Wechselrichter und Akkuspeicher.

*Null-Einspeisung* bedeutet, dass die gesamte PV-Leistung selbst verbraucht wird und nicht für kleines Geld an den 
Energieversorger abgegeben wird. Dazu steuert der **PV-Monitor** die Einspeiseleistung so, dass der Wechselrichter 
rund um die Uhr läuft und mit Hilfe des Akkuspeichers auch nachts mindestens die Grundversorgung sicherstellen kann.
Die dynamische Steuerung der Einspeiseleistung sorgt dafür, dass tagsüber der Akkuspeicher soweit geladen wird, dass
die Grundversorgung in der Nacht sichergestellt werden kann.

## Einführung

Seit 2024 sind in Deutschland bei einem Balkonkraftwerk eine maximale PV-Leistung von 2000Wp und 
eine maximale Einspeisung von 800W zugelassen. Der PV-Überschuss zur Einspeiseleistung ist so groß, 
dass man die PV-Module in eine Ost/West-Ausrichtung aufteilen kann. So hat man 
im Durchschnitt 1000Wp zur Verfügung und verlängert gleichzeitig die Nutzungsdauer gegenüber einer
reinen Süd-Ausrichtung mit 2000Wp. Ab dem Frühjahr startet die PV-Produktion mit dem Sonnenaufgang im Osten und produziert 
nach ca. 1 Stunde bereits akzeptable Werte. Mittags haben die nach Ost/West ausgerichteten Module durch den schrägen
Sonneneinfall einen schlechten Wirkungsgrad, dafür addieren sich die Erträge auf beiden Seiten. Die West-Seite produziert
dann bis in den späten Abend hinein noch Strom. Da nur eine maximale Einspeisung von 800W erlaubt ist, werden alle
Überschüsse zur Ladung des Akkus verwendet. Der Akku dient dann dazu, die ganze Nacht über den Ruhestrom abzudecken.

## Verwendete Komponenten

Für die Einspeisung werden zwei MPPT-Laderegler von [Victron](https://www.victronenergy.de/solar-charge-controllers/) 
und ein [800W Lumentree](https://www.lumentree-portal.de/produkt/lumentree-sun-800/) Wechselrichter 
mit [Trucki-Stick](https://www.lumentree-shop.de/produkt/trucki-t2sg-stick-fuer-lumentree-sun/) verwendet. 
Zur Energiemessung des Hausverbrauchs wird ein [Shelly 3EM](https://www.shelly.com/de/products/shelly-3em) verwendet. 
Die Einspeiseleistung wird mit einem [Shelly Plug S](https://www.shelly.com/de/products/shelly-plug-s-gen3) gemessen. 
Der Ladezustand des Akkus wird mit dem Hall-Effekt Stromsensor [WCS1800](https://www.amazon.de/dp/B09CPXBG6L) 
oder [WCS1800](https://de.aliexpress.com/w/wholesale-WCS1800.html) überwacht.

Durch die aktive Steuerung des **PV-Monitor** werden all diese Komponenten miteinander verbunden und dafür
gesorgt, dass der Akku nicht leerläuft. Tagsüber wird dynamisch solange Strom von den PV-Modulen zur Ladung des Akkus
abgezweigt, bis der Ladezustand ausreichend ist, um nachts mindestens die Ruheleistung der Verbraucher abzudecken.
Dies geschieht durch aktive Steuerung des Trucki-Sticks, der ansonsten dafür verantwortlich ist, immer nur genau den Anteil über den 
Inverter einzuspeisen, der im Haus gerade verbaucht wird.

Für jede Ausrichtung, bekommen die PV-Module einen 48V MPPT-Laderegler, der an den Akku angeschlossen wird. 
Ich verwende zwei [50Ah 51.2V LiFePO4-Akkus](https://glceenergy.com/de/products/48v-50ah-lifepo4-battery-mini) parallel, 
so dass sich eine Gesamtkapzität von 5,12 KWh ergibt. Seit der Inbetriebnahme habe ich es so geschafft, keine einzige KWh mehr einzuspeisen!

## Funktion der aktiven Komponenten

### MPPT-Laderegler
Die beiden MPPT-Laderegler von Victron werden vom **PV-Monitor** über die seriellen *VE.direct* Schnittstellen ausgelesen. 
Dabei wird der *Tracker Mode* (OFF, MPPT) zur Tag/Nacht-Erkennung verwendet. 
Der *Operation State* (Bulk, Absorption, Float) wird zur Kontrolle des Ladezustands der Batterie verwendet.
Die Summe *Panel Power* ergibt die maximale Einspeiseleistung, die aktuell zur Verfügung steht.

### Shelly 3EM
Über den 3-Phasen Energiemesser *Shelly 3EM* wird der aktuelle Hausverbrauch ermittelt. Dieser wird vom **PV-Monitor** durch einen
HTTP-Aufruf an die URL *http://<IP-Adresse>/status* im Sekundentakt abgefragt. Aus der Antwort im JSON-Format wird der Parameter *total_power* 
extrahiert, welcher den aktuellen Gesamtverbrauch des Hauses widerspiegelt. 

### Shelly Plug S
Der Lumentree Wechselrichter ist über einen *Shelly Plug S* ans Stromnetz angeschlossen.  Dieser wird vom **PV-Monitor** durch einen 
HTTP-Aufruf an die URL *http://<IP-Adresse>/status* im Sekundentakt abgefragt. Aus der Antwort im JSON-Format wird der Parameter *power* 
extrahiert, welcher der aktuellen Einspeiseleistung des Inverters entspricht.
Weiterhin wird der Parameter *ison* extrahiert, um zu prüfen, ob das Relay im Shelly den Inverter vom Stromnetz getrennt hat.

### WCS1800
Über den Hall-Effekt Stromsensor *WCS1800* wird permanent der Strom gemessen, mit der der Akku geladen bzw. entladen wird.
Durch Multiplikation der Batteriespannung mit dem Strom und der vergangenen Zeit zwischen zwei Messungen wird daraus der Ladezustand
des Akkus ermittelt.

### Trucki-Stick
Aus dem *Trucki-Stick* wird über einen HTTP-Aufruf an die URL *http://<IP-Adresse>/json* der Parameter *MAXPOWER* ausgelesen. Dieser 
Wert gibt die maximale Einspeiseleistung des Wechselrichters an.

Mit einem HTTP-Aufruf an die URL *http://<IP-Adresse>/?maxPower=<wert>* kann beim *Trucki-Stick* die maximal vom Wechselrichter erzeugte
Einspeiseleistung vorgegeben werden. Dieser Aufruf wird vom **PV-Monitor** verwendet, um über den Tag hinweg dafür zu sorgen, dass von 
der *Panel Power* immer etwas abgezweigt wird, bis eine Ladekapazität des Akkus erreicht wurde, der die Grundversorgung der Verbraucher 
über die Nacht hinweg abdeckt.


## Copyright
**PV-Monitor** is written by Andreas Trappmann.
It is published under the MIT license, check LICENSE for more information.
All text above must be included in any redistribution.

## Release Notes

Version 1.0 - XX.XX.2025

  * Initial publication.
