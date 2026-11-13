# ESP32-S3 RFID alarmni sistem

Projekat predstavlja sistem za kontrolu pristupa i detekciju pokreta, razvijen za **ESP32-S3** korišćenjem **ESP-IDF** okruženja i programskog jezika **C**.

Sistem koristi RFID kartice za autorizaciju, PIR senzor za detekciju pokreta i LCD ekran, RGB LED diodu i buzzer za obaveštavanje korisnika. Registrovane kartice čuvaju se u internoj **NVS memoriji**, pa ostaju sačuvane i nakon ponovnog pokretanja uređaja.

## Funkcionalnosti

- Očitavanje RFID kartica pomoću RC522 čitača (podržani su UID-ovi dužine 4 bajta).
- Registracija, brisanje i pregled autorizovanih kartica preko serijskog terminala.
- Čuvanje do **20 kartica** u NVS memoriji.
- Privremeno deaktiviranje alarma nakon očitavanja autorizovane kartice.
- Detekcija pokreta PIR senzorom kada je sistem aktivan.
- Zvučna i svetlosna signalizacija alarma.
- Prikaz stanja i poruka na LCD 16×2 ekranu preko I²C interfejsa.

## Korišćeni hardver

- ESP32-S3 razvojna ploča
- RFID RC522 čitač i RFID kartice/tagovi
- PIR senzor pokreta
- LCD 16×2 ekran sa I²C adapterom (adresa `0x27`)
- RGB LED dioda
- Buzzer
- Breadboard, spojne žice i odgovarajući otpornici

## Povezivanje komponenti

| Komponenta | Signal | ESP32-S3 pin |
|---|---|---|
| RC522 | SCK | GPIO 12 |
| RC522 | MISO | GPIO 13 |
| RC522 | MOSI | GPIO 11 |
| RC522 | SS / SDA (SPI) | GPIO 10 |
| RC522 | RST | GPIO 4 |
| PIR senzor | OUT | GPIO 5 |
| Buzzer | Signal | GPIO 16 |
| RGB LED | Red | GPIO 17 |
| RGB LED | Green | GPIO 18 |
| RGB LED | Blue | GPIO 21 |
| LCD I²C | SDA | GPIO 8 |
| LCD I²C | SCL | GPIO 9 |

Pinovi su definisani direktno u izvornom kodu projekta.

**Napomena:** RC522 treba napajati prema specifikaciji modula, najčešće sa 3,3 V. Kod LCD I²C adaptera koji radi na 5 V potrebno je proveriti logičke nivoe SDA i SCL linija i po potrebi koristiti konvertor logičkih nivoa.

Sve komponente treba da imaju zajedničku masu (GND).

## Princip rada sistema

Sistem koristi tri osnovna stanja:

### 1. LOCKED – Sistem aktivan

- RGB LED svetli plavo.
- Na LCD ekranu prikazuje se poruka `SISTEM AKTIVAN`.
- PIR senzor prati prisustvo pokreta.
- Sistem čeka autorizovanu RFID karticu.

### 2. UNLOCKED – Pristup odobren

- Aktivira se nakon očitavanja registrovane RFID kartice.
- RGB LED svetli zeleno.
- Na LCD ekranu prikazuje se `PRISTUP ODOBREN`.
- Alarm se privremeno deaktivira.
- Nakon **15 sekundi** sistem se automatski vraća u stanje `LOCKED`.

### 3. ALARM – Detektovan pokret

- Aktivira se kada PIR senzor detektuje novi pokret dok je sistem zaključan.
- RGB LED svetli crveno.
- Na LCD ekranu prikazuje se `ALARM!`.
- Aktivira se buzzer.
- Alarm se može deaktivirati autorizovanom RFID karticom.

Ako se očita nepoznata kartica, pristup se odbija. Dok je sistem zaključan, na LCD ekranu prikazuje se odgovarajuća poruka.

## Upravljanje preko serijskog terminala

Sistem omogućava upravljanje registrovanim karticama pomoću komandi koje se unose preko UART terminala.

| Komanda | Funkcija |
|---|---|
| `register` | Registracija nove RFID kartice |
| `delete` | Brisanje registrovane kartice |
| `list` | Prikaz svih registrovanih kartica |
| `start` | Aktiviranje sistema |
| `help` | Prikaz dostupnih komandi |

### Registracija kartice

U terminal se unosi:

```bash
register
```

Nakon unosa komande potrebno je prisloniti RFID karticu uz RC522 čitač.

Sistem omogućava očitavanje kartice u narednih **20 sekundi**.

Nakon uspešne registracije kartica se čuva u NVS memoriji.

### Brisanje kartice

U terminal se unosi:

```bash
delete
```

Zatim je potrebno prisloniti prethodno registrovanu karticu uz čitač.

Kartica se uklanja iz memorije sistema.

### Pregled registrovanih kartica

```bash
list
```

U serijskom terminalu prikazuju se sačuvane RFID kartice.

### Aktiviranje sistema

```bash
start
```

Sistem prelazi u stanje `LOCKED` i nastavlja praćenje PIR senzora.

## Struktura projekta

```text
proba1/
│
├── components/
│   │
│   ├── buzzer/
│   │   ├── include/
│   │   │   └── buzzer.h
│   │   ├── buzzer.c
│   │   └── CMakeLists.txt
│   │
│   ├── card_storage/
│   │   ├── include/
│   │   │   └── card_storage.h
│   │   ├── card_storage.c
│   │   └── CMakeLists.txt
│   │
│   ├── lcd/
│   │   ├── include/
│   │   │   └── lcd.h
│   │   ├── lcd.c
│   │   └── CMakeLists.txt
│   │
│   ├── pir/
│   │   ├── include/
│   │   │   └── pir.h
│   │   ├── pir.c
│   │   └── CMakeLists.txt
│   │
│   ├── rfid/
│   │   ├── include/
│   │   │   └── rfid.h
│   │   ├── rfid.c
│   │   └── CMakeLists.txt
│   │
│   └── rgb/
│       ├── include/
│       │   └── rgb.h
│       ├── rgb.c
│       └── CMakeLists.txt
│
├── main/
│   ├── main.c
│   ├── CMakeLists.txt
│   └── idf_component.yml
│
├── CMakeLists.txt
├── dependencies.lock
└── README.md
```

## Opis softverskih komponenti

### RFID

Komponenta `rfid` upravlja RC522 RFID čitačem.

Zadužena je za:

- Inicijalizaciju SPI komunikacije.
- Detekciju RFID kartica.
- Očitavanje UID identifikatora.
- Prosleđivanje očitanih podataka glavnom programu.

Za komunikaciju sa čitačem koristi se biblioteka `abobija/rc522`.

### Card Storage

Komponenta `card_storage` omogućava upravljanje autorizovanim RFID karticama.

Funkcionalnosti:

- Dodavanje novih kartica.
- Brisanje postojećih kartica.
- Proveravanje autorizacije.
- Prikaz registrovanih kartica.
- Čuvanje podataka u NVS memoriji.

Maksimalan broj registrovanih kartica je **20**.

### PIR

Komponenta `pir` omogućava detekciju pokreta.

PIR senzor povezan je na GPIO 5 i koristi se za aktiviranje alarma kada je sistem zaključan.

### LCD

Komponenta `lcd` upravlja LCD ekranom dimenzija 16×2.

Za komunikaciju se koristi I²C interfejs.

LCD prikazuje:

- Trenutno stanje sistema.
- Poruke o autorizaciji.
- Informacije o registraciji kartica.
- Upozorenja o detektovanom pokretu.

### RGB LED

Komponenta `rgb` omogućava vizuelni prikaz stanja sistema.

Boje imaju sledeće značenje:

- **Plava** – sistem aktivan.
- **Zelena** – pristup odobren.
- **Crvena** – alarm aktiviran.

### Buzzer

Komponenta `buzzer` koristi se za zvučnu signalizaciju.

Buzzer se aktivira kada sistem detektuje pokret u zaključanom stanju.

## Korišćene tehnologije

| Tehnologija | Namena |
|---|---|
| C | Programiranje mikrokontrolera |
| ESP-IDF | Razvojno okruženje za ESP32 |
| FreeRTOS | Upravljanje izvršavanjem i razmenom podataka |
| SPI | Komunikacija sa RFID čitačem |
| I²C | Komunikacija sa LCD ekranom |
| NVS | Trajno čuvanje RFID kartica |
| UART | Komunikacija sa serijskim terminalom |
| CMake | Organizacija i kompajliranje projekta |
| Git / GitHub | Kontrola verzija i timski razvoj |

## Instalacija i pokretanje

### 1. Preduslovi

Za pokretanje projekta potrebno je:

- ESP32-S3 razvojna ploča.
- Instaliran ESP-IDF.
- Visual Studio Code.
- Espressif IDF ekstenzija.
- Povezane hardverske komponente.
- USB kabl za povezivanje računara i ESP32-S3.

### 2. Preuzimanje projekta

Klonirati GitHub repozitorijum:

```bash
git clone URL_REPOZITORIJUMA
```

Otvoriti preuzeti folder u Visual Studio Code-u.

### 3. Podešavanje ESP32-S3

U ESP-IDF terminalu pokrenuti:

```bash
idf.py set-target esp32s3
```

### 4. Kompajliranje projekta

```bash
idf.py build
```

### 5. Prebacivanje programa na ESP32

```bash
idf.py -p COM5 flash
```

Port `COM5` zameniti odgovarajućim portom na računaru.

### 6. Pokretanje serijskog monitora

```bash
idf.py -p COM5 monitor
```

Nakon pokretanja sistema moguće je pratiti poruke i unositi komande za upravljanje RFID karticama.

## Dodatne napomene

- Projekat je razvijen za ESP32-S3 mikrokontroler.
- RFID čitač koristi SPI komunikaciju.
- LCD ekran koristi I²C komunikaciju.
- Podaci o autorizovanim karticama čuvaju se u NVS memoriji.
- Sistem podržava RFID kartice sa UID identifikatorom dužine 4 bajta.
- Nakon autorizacije sistem ostaje deaktiviran 15 sekundi.
- Registracija i brisanje kartica imaju vremensko ograničenje od 20 sekundi.
- Sistem predstavlja obrazovni prototip. Autorizacija samo na osnovu UID-a nije dovoljno bezbedna za profesionalne sisteme kontrole pristupa.
- Projekat trenutno ne upravlja fizičkom elektronskom bravom.

## Autori

Projekat je realizovan kao timski studentski rad.

**Članovi tima:**

- Luka Vojvodić 
- Andrija Mitrović
- Jovan Sandić
