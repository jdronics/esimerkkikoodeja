//////////////////////////////////////////////////
//
// Program  : HC-SR04 Ultrasonar etäisyysmittaus
// Author   : Jani Hirvinen / JTL, jani@jdronics.fi
// Versio   : v1.0 02.04.2022
//
//////////////////////////////////////////////////
//
// Tämä ohjelma on tehty esimerkki ohejlamksi M.Laineelle
// Idea on mitata pellettitankin sisällön korkeutta. 
//
// Ohjelma ei missään nimessä ole täydellinen ja siitä puuttuu monia
// turvamekanismeja, tämä on vain esimerkki jolla pääsee mukavasti
// alkuun etäisyys mittauksissa. 
//
// Tärkeitä muuttujia:
//  EMPTY           - Arvo centtimetreinä kun menee yli niin tulee "häly" eli LED syttyy
//  NAYTE_AIKA      - Kuinka monen minuutin välein näyte otetaan
//  ECHO_PIN        - Missä Arduino pinnissä anturin echo ulostulo on
//  TRIG_PIN        - Missä Arduino pinnissä anturin trig heräte on
//


///////////////////////
// Hardkoodatut muuttujat

#define EMPTY       100    // cm arvo jolloin tankki on "tyhjä" ja tulee hälyytys
#define NAYTE_AIKA  5      // Näyteaika minuutteina (DEVON modessa sekunninkymmenyksinä)

#define ECHO_PIN    2      // Arduinon D2 pinna HR-SR04sen ECHO pinnassa
#define TRIG_PIN    3      // Arduinon D3 pinna HT-SR04sen TRIG pinnassa
#define LED_PIN     9      // Meidän "hälytys" pinna, D9

#define FULL        20     // Arvo joka tulee kun tankki on täynnä
#define NAYTTEITA   5      // Kuinka monta näytettä otetaan josta lasketaan keskiarvo etäisyys


#define LOOP50HZ    20     // 50 Hz loopin viiveaika millisekuntteina
#define LOOP01HZ    1000   // 1 Hz loopin viiveaika millisekuntteina
#define CLEAR_MS    2      // Viiveaika sensorin nollaamiseen
#define MEAS_MS     10     // Ultraääni pulssin pituus

#define DEVON              // Devaus moodi, käytetään vain kun ohjelmaa kehitetään. Kommentoi pois // kun ei tarvita

// Ja muutama yleistkäyttöinen #define
#define FALSE       0
#define TRUE        1



///////////////////////
// Globaalit muuttujat


int etaisyys     =  0;             // Mitattu etäisyys
int etaisyys_taulu[NAYTTEITA + 1]; // Pooli missä pidetään X kpl viimeisimpiä näytteitä ja poolin ensimmäinen arvo on montako näytettä on otettu
boolean is_alarm =  0;             // Onko hälytys ON vai OFF

// Ohjelman globaaleja muuttujia
unsigned long nayte_millis = 0;
unsigned long loop_50hz_millis = 0;
unsigned long loop_01hz_millis = 0;
unsigned long curMillis = 0;


/////////////////////////////
// SETUP()

void setup() {

    // Määritellään sarjaportti nopeus (115k) ja lähetetään perus headeri sarjaporttiin
    Serial.begin(115200);
    Serial.println("Pelletti mittari v1.0");
    Serial.println("by jpkh@JTL");

    // Määritellään I/O pinnat
    pinMode(ECHO_PIN, INPUT);         // Määritellään ECHO pinnan tila, SISÄÄNTULEVA / EI YLÖSVETOA
    pinMode(TRIG_PIN, OUTPUT);        // Määritellään TRIG pinnan tila, ULOSLÄHTEVÄ
    pinMode(LED_PIN, OUTPUT);         // Määritellään LED pinnan tila, ULOSLÄHTEVÄ

} // END SETUP()


//////////////////////////////
// LOOP()
void loop() {

    // Paikalliset muuttujat
    char output[128];

    // Näytteenotto ajastin, tässä myös DEVON optio ohjelmakehittäjälle
    curMillis = millis();
#ifndef DEVON
    if (curMillis - nayte_millis >= (NAYTE_AIKA * 1000) * 60) {
#else
    if (curMillis - nayte_millis >= NAYTE_AIKA * 100) {
#endif
        nayte_millis = curMillis;
        mittaus();
    }

    // Hälytyksen tarkistus ajastin, 50Hz
    curMillis = millis();
    if (curMillis - loop_50hz_millis >= LOOP50HZ) {
        loop_50hz_millis = curMillis;
        paivita_halytystila();
    }

    // Serial tulostuksen ajastin, 1Hz
    curMillis = millis();
    if (millis() - loop_01hz_millis >= LOOP01HZ) {
        loop_01hz_millis = curMillis;
        sprintf(output, "Naytteita : %4d  Etaisyys : %d cm \n", etaisyys_taulu[0], etaisyys);
        Serial.print(output);
    }
} // END LOOP()

// - END OF MAIN PROGRAM



//////////////////////////////////////////////////////////////
// Alirutiinit
//////////////////////////////////////////////////////////////


///////////////////////////////////////
// func mittaus()
//
// Mitataan tämänhetkinen etäisyys, talletetaan se taulukkoon
// ja lasketaan uusi keskiarvo
//
// Parannuksia: Tarkistetaan että uusi arvo ei ole XYZ% isompi tai pienempi 
// edellisistä mittauksista, näin voidaan sulkea virhemittaus pois. 
//
void mittaus() {

    // Alustetaan paikalliset muuttujat
    int mitattu_etaisyys = 0;
    long mitattu_kesto = 0;

    // Valmistellaan sensori mittausta varten
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(CLEAR_MS);

    // Lähetetään 10ms pituinen pulssi
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(MEAS_MS);
    digitalWrite(TRIG_PIN, LOW);

    // Luetaan paluupulssin pituus ja lasketaan etäisyys
    mitattu_kesto = pulseIn(ECHO_PIN, HIGH);
    mitattu_etaisyys = mitattu_kesto * 0.034 / 2;   // Tulos pitää jakaa kahdella että saadaan yhden suunnan etäisyys

    // Tallennetaan etäisyys ja tarkistetaan monesko luentakerta tämä oli
    // jos luentakerta oli ensimmäinen ja etäisyys on MIN/MAX sisällä, alustetaan taulukko jotta saadaan mahdollisimman
    // nopeasti ensimmäiset tulokset perille
    if (!etaisyys_taulu[0]) {
        etaisyys_taulu[0] = 1;
        for (int tmploop = 1; tmploop <= NAYTTEITA; tmploop++) {
            etaisyys_taulu[tmploop] = mitattu_etaisyys;
        }
    } else {
        etaisyys_taulu[0]++;
        // Siirretään taulussa kaikki arvot yksi "oikealle" jotta saadaan ensimmäinen paikka vapaaksi uudelle mittaustulokselle
        for (int tmploop = NAYTTEITA; tmploop >= 2; tmploop--) {
            etaisyys_taulu[tmploop] = etaisyys_taulu[tmploop - 1];
        }
        etaisyys_taulu[1] = mitattu_etaisyys;

        long keskiarvo_lasku;
        // Lasketaan uusi keskiarvo mittaustuloksille ja talletetaan se muuttujaan
        for (int tmploop = 1; tmploop <= NAYTTEITA; tmploop++) {
            keskiarvo_lasku = keskiarvo_lasku + etaisyys_taulu[tmploop];
        }
        //Serial.println(keskiarvo_lasku);
        etaisyys = keskiarvo_lasku / NAYTTEITA;
    }
}

/////////////////////////////////////////////
// Func: paivita_halytystila()
//
// Tilan tarkistus ja hälätyksen teko
//
// Tämä on vain esimerkiksi. Varmaankin tarvitsee paremman toimenpiteen kun "häly" tulee
//
void paivita_halytystila() {

    // Jos mitatty etaisyys on suurempi kuin EMPTY mitta. Laitetaan häly päälle ja LED palamaan
    if (etaisyys >= EMPTY) {
        digitalWrite(LED_PIN, HIGH);
    } else {
        digitalWrite(LED_PIN, LOW);
    }
}
