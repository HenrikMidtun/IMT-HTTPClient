# IMT-HTTPClient

## Teknologi og forskningslære

Til dere som skal bruke dette i ToF.
Lykke til med prosjektet deres!
Det er mange kule ting som kan gjøres med en Arduino og å lage en værbøye er bare en av dem.

Arduino er basert på C/C++, men kommer med mange gode biblioteker. Hvis dere lurer på hvordan dere skal utrykke dere i henhold til *syntax* så bruk søkeord som "arduino" eller "cpp".
Biblioteker, eller *libraries* som det heter, er gode hjelpemidler laget av andre som dere kan benytte dere av. Et *library* er helt enkelt kode som noen andre har skrevet tidligere som vil være til hjelp når dere ønsker å bruke nye sensorer. Noen ganger kan de inneholde kode for avansert bruk, slik som er inkludert gjennom våre ntnu.* filer. Spesielt for Arduino er det mange gode biblioteker og guides som ligger ute. Benytt dere av dette, så er det lettvint å bygge videre på dette fundamentet.

## Guide

### Nedlastinger
- Last ned Arduino Desktop IDE [1](https://www.arduino.cc/en/software#download). I dette programmet så skal dere kode deres eget program og laste det opp til en Arduino. På Windows så anbefales det å styre unna Windows Store versjonen ettersom at den har kjente mangler.

- Last ned koden fra Institutt for Marin Teknologi IMT [2], dette repositoriet. Dette programmet tar hånd av oppkobling til internett, GPS og sending av deres data. Samt så følger det med biblioteker som kreves for å bruke programmet.

### Oppsett
- Følg guide for å sette opp deres Arduino [3](https://www.arduino.cc/en/Guide/MKRNB1500). Denne vil føre dere gjennom hvordan man laster ned nødvendige støttefiler for modellen MKR NB 1500 og hvordan Arduinoen kommuniserer med en PC. I tillegg så er det inkludert en guide til hvordan man kan laste ned eksempelkode og hvordan man overfører programmet til en Arduino.
- Flytte kode og biblioteker fra IMT til deres Arduino mappe.*
- Flytt hele mappen “IMT-HTTPClient” til “C:\Users\OlaNordmann\Documents\Arduino”. I Windows så er det her programmer som man ønsker å laste opp til en Arduino vil ligge.
- Kopier eller klipp ut innholdet av mappen “IMT-HTTPClient/libraries” og overfør dette til mappen “C:\..\Arduino\libraries\”. Det er her Arduino automatisk leter etter biblioteker.

*Hvis dere sitter fast så kan dere referere til [4](https://www.arduino.cc/en/Guide/Libraries) under “Manual installation” som viser hvor man skal overføre biblioteker.

### Oppkobling
- I første omgang så er det greit å gjøre de fleste koblingene sine på et breadboard. Etterhvert som at man ønsker å beskytte seg mot dårlig kontakt så anbefaler vi å lodde egne kretser f. eks. på stripboards.
- Sett inn SIM kort og koble til antenne.
  - Sett et SIM kort med gyldig dataplan inn i Arduinoen. Deretter koble til antennen i porten som er på andre siden av SIM-kortet.
- Koble til GPS.
  - Den røde kabelen skal kobles til en 3.3V kilde og den svarte skal til jord (GND). 
  - Den hvite kabelen er for signaler og skal kobles til RX-porten til en Arduino. For dere så vil dette være PIN 13 med Arduino MKR NB 1500.
- Koble til luftsensoren, BME280. Noen av disse blir levert uten pins. Et eksempel på hvordan man enkelt kan lodde pins på slike kort er gitt i [5](https://learn.sparkfun.com/tutorials/sparkfun-bme280-breakout-hookup-guide).
  - Den trenger jord og 3.3V input.
  - Sensoren kommuniserer over I2C og dermed skal SDA på sensoren til SDA porten på Arduino. I samme stil så skal SCL til SCL.
- Koble til vanntett temperatursensor, DS18B20.
  - Følg guiden i [6](https://randomnerdtutorials.com/guide-for-ds18b20-temperature-sensor-with-arduino/) for å koble denne til.*
  - Koble signalkabelen til port [~]5. Hvis ikke så må dere manuelt gjøre endre på pin-nummer i koden.
  
*Vi har brukt det som guiden kaller for “normal” oppkobling.

### Annet
Det anbefales å teste ut sensorene hver for seg når man kobler opp for første gang. Den kjappeste metoden for å få lage et program for en sensor er å laste ned eller se på eksempelkode. Dette pleier å følge med i biblioteker.
Hvis man ønsker å legge til flere sensorer så kan det være lurt å bruke god tid på å finne riktig bibliotek for deres bruk. I Arduino IDEen er det mulig å søke opp modellnavnet til den ønskede sensoren. Da vil man finne mange som ligger ute til bruk. Trykk på “More info” og les gjennom hva forfatteren sier om biblioteket.
En kjent “feil” med Arduino IDE er at den ikke finner brettet slik at man ikke kan laste opp programmet sitt. Da er det lurt å trykke på Reset knappen som ligger på brettet i det meldingen “Compiling sketch…” vises. Dette løser som oftest problemet.

Lykke til!

### Lenker
[1] https://www.arduino.cc/en/software#download

[2] https://github.com/HenrikMidtun/IMT-HTTPClient

[3] https://www.arduino.cc/en/Guide/MKRNB1500 

[4] https://www.arduino.cc/en/Guide/Libraries

[5] https://learn.sparkfun.com/tutorials/sparkfun-bme280-breakout-hookup-guide

[6] https://randomnerdtutorials.com/guide-for-ds18b20-temperature-sensor-with-arduino/


    Henrik S. Midtun
    Christoffer R. Helgesen
