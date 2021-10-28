/*  Im Rahmen des P-Seminars 2P_Physik bei Herrn Sander 2020-22
 *  von Gregor Sellschopp
 *  läuft auf dem Windradmodell
 *  Die beiden grundsätzlichen Modi: Nachführen oder Messen (der Umdrehung und Leistung)
 *  Als Eingabemöglichkeiten gibt es den Drehschalter mit Knopffunktion und einen Kippschalter, um zwischen den Hauptmodi zu wechseln
 *  Finale Version vom 18.10.2021
 */

#include <Wire.h> // Wire Bibliothek einbinden
#include <LiquidCrystal_I2C.h> // LiquidCrystal_I2C Bibliothek einbinden
LiquidCrystal_I2C lcd(0x27, 16, 2); //Hier wird festgelegt um was für einen Display es sich handelt.
//In diesem Fall eines mit 16 Zeichen in 2 Zeilen und der HEX-Adresse 0x27.
//Für ein vierzeiliges I2C-LCD verwendet man den Code "LiquidCrystal_I2C lcd(0x27, 20, 4)"
// SDA -> A4
// SCL -> A5

int Index;
const float resolution = 1.8; // Schrittweite des Schrittmotors in Grad. Ein 'step' entspricht also 1.8 Grad.
int z = 0;
int winkel = 0;
int laenge = 1;
double stepperPos = 0.000; // aktuelle Position des Schrittmotors
// Anschlüsse zur Ansteuerung des Zweipunktreglers und Schrittmotors
const int enable = 4;
const int step = 9;
const int direction = 10;
const int kontakt_rechts = 3;
const int kontakt_links = 2;

// Anschlüsse und Variablen des Dreh-encoders
int Pin_clk_Letzter;
int Pin_clk_Aktuell;
const int pin_clk = 4;
const int pin_dt = 5;
const int button_pin = 6;
int counter = 0;

// Anschlüsse und Variablen des Ultraschallsensors
const int trigger = 7;
const int echo = 12;
long dauer = 0;
long entfernung = 0;
int umdrehungen;
int grenzwert = 15;

// Anschlüssen und Variablen zur elektronischen Messung
float widerstand = 1.70; // Wert des Lastwiderstandes in Ohm
float leistung; // Leistung, die bei diesem Widerstand abfällt
float stromstaerke; // theoretische, berechnete Stromstärke
const int sp_pin_pos = A0; // Pin, der die über dem Widerstand abgefallene Spannung ausliest. Beim Anschließen auf Polarität achten!

int messdauer = 5; // Messdauer in Sekunden

int positionen[] = {0, 5, 10, 15, 20, 25, 35, 45}; // Positionen, die im Mess-Modus angefahren werden
int anzahl; // anzahl der Positionen im Array positionen
boolean geeicht = false;

int ANTI_OSZ_DEL = 0; // kurzer delay um Oszillieren vorzubeugen; Zeitdauer in Millisekunden; Genaue Auswahl erfolgt im Menü

// Pins des Schiebeschalters für die Auswahl eines Modus
int modus = 11;

void setup()
{
  Serial.begin(115200); //Baudrate beim Anschließen an einen Computer; diese muss in der Arduino IDE richtig eingestellt werden, um im seriellen Monitor vernünftige Ausgaben zu erhalten.
  // Kontakte am Windrad für den Zweipunktregler. Beide werden als digitale Eingänge gesetzt.
  pinMode(kontakt_rechts, INPUT);
  pinMode(kontakt_links, INPUT);

  // pins zur Steuerung des A4988 (Schrittmotortreibers)
  pinMode(enable, OUTPUT); //Enable; nicht in verwendung; kann den Treiber insgesamt ausschalten
  pinMode(step, OUTPUT); //Step
  pinMode(direction, OUTPUT); //RIchtung

  // pins zur Steuerung des Ultraschallsensors
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);

  // Eingangs-Pins des Drehschalters
  pinMode (pin_clk, INPUT);
  pinMode (pin_dt, INPUT);
  pinMode (button_pin, INPUT);
  pinMode(modus, INPUT);

  // ...und deren Pull-Up Widerstände aktiviert
  digitalWrite(pin_clk, true);
  digitalWrite(pin_dt, true);
  digitalWrite(button_pin, true);
  digitalWrite(modus, true);
  digitalWrite(enable, LOW);

  lcd.init(); //Im Setup wird der LCD gestartet
  lcd.backlight(); //Hintergrundbeleuchtung einschalten
  lcd.clear(); // Jegliche Inhalte auf dem Bildschirm löschen

  Pin_clk_Letzter = digitalRead(pin_clk); // letzter Status des Drehschalters wird auch gespeichert, um ihn später mit dem aktuellen zu vergleichen und so Änderungen festzustellen.

  for (byte i = 0; i < (sizeof(positionen) / sizeof(positionen[0])); i++) {
    anzahl++;
  }

  eichen();
  antiOsz(); // Hier kann eine mögliche Verlangsamung der Schrittmotorbewegungen festgelegt werden, um Oszillation des Zweipunktreglers zu vermeiden
}

void loop() // Diese Schleife wird dauerhaft wiederholt
{
  if (digitalRead(modus)) nachfuehren(); // Hier wird der Kippschalter digital eingelesen, der über den Programmmodus entscheidet
  else messen();
}

void nachfuehren() ////////////////////////////////////////////////////////////////////////////////////// Diese Methode wird im Nachfuehren-Modus aufgerufen
{
  // Hier werden die Messwerte auf dem LCD-Bildschirm ausgegeben
  lcd.setCursor(0, 0);
  lcd.print("Modus:");
  lcd.setCursor(0, 1);
  lcd.print("Nachfuehren");

  while (digitalRead(kontakt_rechts) == HIGH) // Wenn die Fahne die rechte Grenze erreicht
  {
    delay(ANTI_OSZ_DEL); // Soll Oszillation verhindern (kann in der 'antiOsz' Methode festgelegt werden)
    digitalWrite(direction, LOW);
    Serial.println("RECHTS_HIGH");

    digitalWrite(step, HIGH);
    delayMicroseconds(500);
    digitalWrite(step, LOW);
    delayMicroseconds(500);
    stepperPos = stepperPos + 0.1125;
  }
  while (digitalRead(kontakt_links) == HIGH) // Wenn die Fahne die linke Grenze erreicht
  {
    delay(ANTI_OSZ_DEL);  // Soll Oszillation verhindern (kann in der 'antiOsz' Methode festgelegt werden)
    digitalWrite(direction, HIGH);
    Serial.println("LINKS_HIGH");

    digitalWrite(step, HIGH);
    delayMicroseconds(500);
    digitalWrite(step, LOW);
    delayMicroseconds(500);
    stepperPos = stepperPos - 0.1125;
  }
}

void messen() ///////////////////////////////////////////////////////////////////////////////////////////////// Diese Methode wird im Mess-Modus aufgerufen
{
  unsigned long t; // speichert eine Zeitreferenz
  // Ausgaben über die serielle Schnittstelle des Arduinos
  Serial.print("------------------messen---stepperPos: ");
  Serial.print(stepperPos);
  Serial.print(" anzahl: ");
  Serial.println(anzahl);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Messzyklus Start");

  t = millis();
  while (millis() < t + 2000) {} // Zeit für den Stepper um aus einer möglichen starken Auslenkung in die Ausgangsposition zu kommen.

  for (int j = 0; j < anzahl; j++) // geht die Positonen in 5 Grad Schritten durch -> siehe Array 'positionen'
  {
    int pos = positionen[j];
    // Hier wird vorangekündigt, welcher Winkel als nächstes gemessen wird. Der Messintervall beträgt 5 Sekunden.
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Jetzt kommen");
    lcd.setCursor(0, 1);
    lcd.print(pos);
    lcd.setCursor(4, 1);
    lcd.print("Grad");

    if (stepperPos < pos)// fährt in die richtige Position
    {
      while (stepperPos < pos)
      {
        delay(ANTI_OSZ_DEL); // Soll Oszillation verhindern (kann in der 'antiOsz' Methode festgelegt werden)
        digitalWrite(direction, HIGH);
        digitalWrite(step, HIGH);
        delayMicroseconds(500);
        digitalWrite(step, LOW);
        delayMicroseconds(500);
        stepperPos = stepperPos + 0.1125;
      }
    }
    else if (stepperPos > pos)
    {
      while (stepperPos > pos)
      {
        delay(ANTI_OSZ_DEL); // Soll Oszillation verhindern (kann in der 'antiOsz' Methode festgelegt werden)
        digitalWrite(direction, LOW);
        digitalWrite(step, HIGH);
        delayMicroseconds(500);
        digitalWrite(step, LOW);
        delayMicroseconds(500);
        stepperPos = stepperPos - 0.1125;
      }
    }
    t = millis();
    while (millis() < t + 2000) {} // Zeit für den Stepper um aus einer möglichen starken Auslenkung in die Ausgangsposition zu kommen.
    // Ausgaben über die serielle Schnittstelle des Arduinos
    Serial.print("  stepperPos: ");
    Serial.print(stepperPos); // ist jetzt in der richtigen Position
    Serial.print("  pos: " );
    Serial.print(pos);

    umdrehungen = 0; // Anzahl der erkannten Flügelblätter
    float spannung = 0; // über dem Lastwiderstand abfallende Spannung
    unsigned long k = millis();

    int z = 0;
    boolean vorherBlockiert = false; // Status der vorherigen Messung wird gespeichert, damit man nicht denselben Flügel zwei Mal detektiert und als Zwei erkennt.
    while ( k + messdauer * 1000 > millis() ) // In dieser Schleife wird auf zwei Arten die Leistung des Windrads gemessen:
    {
      z++;
      // einmal über Ultraschall
      for (int m = 0; m < 2; m++)
      {
        digitalWrite(trigger, LOW);
        t = millis();
        while (millis() < t + 5) {}
        digitalWrite(trigger, HIGH);

        t = millis();
        while (millis() < t + 10) {}
        digitalWrite(trigger, LOW);

        dauer = pulseIn(echo, HIGH, 3000);
        entfernung = entfernung + (dauer / 2) * 0.03432;
      }
      entfernung = entfernung / 2;
      Serial.println(entfernung);

      if (entfernung < grenzwert && entfernung > 0 && !vorherBlockiert) // Bewirkt, dass ein Flügel nur beim ersten Durchgang erkannt wird. Es muss wieder frei werden, damit ein neuer Fügel erkannt wird.
      {
        umdrehungen++;
        vorherBlockiert = true;
      }
      if (entfernung > grenzwert || entfernung == 0) vorherBlockiert = false;

      // und über die elektrische Leistung
      spannung = spannung + analogRead(sp_pin_pos) * 5.0 / 1023; // analoges Einlesen der Spannung, die über dem Lastwiderstand abfällt
    }
    // Umrechnungen, um die Stromstärke zu erschließen
    spannung = spannung / z;
    stromstaerke = spannung / widerstand;
    leistung = stromstaerke * spannung;
    // Hier werden die Messwerte einer Position auf dem LCD-Bildschirm ausgegeben
    lcd.clear();
    lcd.setCursor(0, 0);// In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("Flueg./5s: ");
    lcd.setCursor(11, 0);
    lcd.print(umdrehungen);
    lcd.setCursor(0, 1);
    lcd.print("P in mW: ");
    lcd.setCursor(10, 1);
    lcd.print(leistung * 1000);

    t = millis();
    while (millis() < t + 5000) {}
    // Ausgaben über die serielle Schnittstelle des Arduinos
    Serial.print("  Umdrehungen in 5 sek.: ");
    Serial.print(umdrehungen);
    Serial.print("    |    Leistung: ");
    Serial.print(leistung);
    Serial.print(",  Spannung: ");
    Serial.print(spannung);
    Serial.print(",  theoretische Stromstärke: ");
    Serial.println(stromstaerke);

    Serial.println();
  }
  lcd.clear();
}

void eichen() ///////////////////////////////////////////////////////////////////////////////////////////////////// Diese Methode legt die Ausgangspositionen für die weiteren Prozesse fest.
{
  lcd.setCursor(0, 0);
  lcd.print("nullen...");
  while (!geeicht)
  {
    winkel = counter / 2;
    Serial.print("eichen    ");
    Serial.print(winkel);
    Serial.print("    ");
    Serial.println(stepperPos);
    Pin_clk_Aktuell = digitalRead(pin_clk);

    // Überprüfung des Dreh-encoders auf Änderung
    if (Pin_clk_Aktuell != Pin_clk_Letzter)
    {
      if (digitalRead(pin_dt) != Pin_clk_Aktuell)
      {
        // Pin_CLK hat sich zuerst verändert      => die eine Drehrichtung
        counter ++;
        digitalWrite(direction, LOW);
        while (counter / 2 > stepperPos + 0.1125 / 2)
        {
          digitalWrite(step, HIGH);
          delayMicroseconds(500);
          digitalWrite(step, LOW);
          delayMicroseconds(500);
          stepperPos = stepperPos + 0.1125;
        }
      }
      else
      { // Andernfalls hat sich Pin_DT zuerst verändert       => die andere Drehrichtung
        counter--;
        digitalWrite(10, HIGH);
        while (counter / 2 < stepperPos - 0.1125 / 2)
        {
          digitalWrite(step, HIGH);
          delayMicroseconds(500);
          digitalWrite(step, LOW);
          delayMicroseconds(500);
          stepperPos = stepperPos - 0.1125;
        }
      }
    }
    if (!digitalRead(button_pin))// && winkel != 0) // Durch Drücken des encoders wird die Position des Steppers genullt.
    {
      winkel = 0;
      stepperPos = 0;
      geeicht = true;
      Serial.println("erfolgreich genullt");
      delay(500);
    }
    Pin_clk_Letzter = Pin_clk_Aktuell;
  }
  lcd.clear();
}
int antiOsz() { /////////////////////////////////////////////////////////////////////////////////////////////////////////////// Hier kann eine mögliche Verlangsamung festgelegt werden, um Oszillation des Zweipunktreglers zu vermeiden
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("delay gegen");
  lcd.setCursor(0, 1);
  lcd.print("Oszillation?");
  int t = millis();
  boolean antiOszDel = false;
  while (t + 5000 > millis()) {     // falls man eine Verlangsamung einstellen möchte, muss hier der Encoder gedrückt werden. Achtung: das System reagiert nicht sofort, sondern wartet noch den Rest der insgesamt 5 Sekunden ab. Mehrmaliges Drücken zur Sicherheit schadet nicht.
    if (!digitalRead(button_pin)) {
      antiOszDel = true;
    }
  }
  if (antiOszDel) {
    lcd.clear();
    String ANTI_OSZ_DEL_STR;
    lcd.setCursor(0, 0);
    lcd.print("delay: ");
    
    while (digitalRead(button_pin)) { // Solange man den Encoder nicht noch ein Mal drückt, kann der Wert durch Drehen verändert werden.
      if (ANTI_OSZ_DEL < 100)  ANTI_OSZ_DEL_STR = "0" + String(ANTI_OSZ_DEL);   //padding => resultierende Zahl hat immer 3 Stellen => kein clear() nötig => bessere Qualität der Ausgabe
      else  if (ANTI_OSZ_DEL < 10)  ANTI_OSZ_DEL_STR = "00" + ANTI_OSZ_DEL;
      lcd.setCursor(6, 0);
      lcd.print(ANTI_OSZ_DEL_STR);
      Pin_clk_Aktuell = digitalRead(pin_clk);

      // Überprüfung des Dreh-encoders auf Änderung
      if (Pin_clk_Aktuell != Pin_clk_Letzter)
      {
        if (digitalRead(pin_dt) != Pin_clk_Aktuell) ANTI_OSZ_DEL ++; // Wert wird entsprechend des Drehschalters verändert.
        else ANTI_OSZ_DEL --;
        if ( ANTI_OSZ_DEL < 0) ANTI_OSZ_DEL = 0;
      }
      Pin_clk_Letzter = Pin_clk_Aktuell;
      Serial.print("anti-oszillations-delay: ");
      Serial.println(ANTI_OSZ_DEL); // Finale Ausgabe des bestätigten Wertes.
    }
  }
  lcd.clear(); // Alles auf dem Bildschirm wird gelöscht
}
