#include<CountUpDownTimer.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Battery.h>
#include "LowPower.h"
#include <JC_Button.h>          // https://github.com/JChristensen/JC_Button
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM

byte masterCard[4];   // Stores master card's ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader


#define RST_PIN         9           // reset pin of 522 connected to d9
#define SS_PIN          10          // ss pin of 522 connected to d10

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

CountUpDownTimer T(DOWN); // initialize timer in down counter mode

Battery battery(3600, 4200, A0); // defining min and max voltages of the battery and voltage monitor pin

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_64X32_1F_1_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

// initial Time display is 0:0:0
int h = 0;
int m = 0;
int s = 0;

Button myBtn(4);       // define the button on pin d4
Button hBtn(2);       // define the button on pin d4
Button sBtn(3);       // define the button on pin d4


int sl = 0; // switch lock to prevent setting timer more than once

unsigned long timeon ;    // keeping the time for auto power off
const unsigned long LONG_PRESS(1000); //duration of the long press
const unsigned long VERY_LONG_PRESS(4000); //duration of the long press

void wakeUp()
{
  // Just a handler for the pin interrupt.
    h=m=0;
    timeon= millis();
  detachInterrupt(0);

}

void setup()
{
  myBtn.begin();              // initialize the button object
  hBtn.begin();              // initialize the button object
  sBtn.begin();              // initialize the button object

  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init();
  u8g2.begin();
  
  u8x8_cad_StartTransfer(u8g2.getU8x8());// reduce brightnesss to minimum
  u8x8_cad_SendCmd( u8g2.getU8x8(), 0x81);
  u8x8_cad_SendArg( u8g2.getU8x8(), 1);  //max 157
  u8x8_cad_EndTransfer(u8g2.getU8x8());

 
  
  
  pinMode(5, OUTPUT);//motor output
  T.SetTimer(h, m, s);
  battery.begin(3300, 6.1, &sigmoidal);
  mfrc522.PCD_SoftPowerDown(); //put mfrc522 to sleep



  for ( uint8_t i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard

  }



}


void loop()
{
  myBtn.read();               // read the button
  hBtn.read();               // read the button
  sBtn.read();               // read the button




  T.Timer(); // run the timer
  if (sl == 0) {
    u8g2.setPowerSave(0);

    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_profont11_tf);
      u8g2.setCursor(8, 20);
      if (h < 10)u8g2.print("0"); // always 2 digits
      u8g2.print(h);
      u8g2.print(":");
      if (m < 10)u8g2.print("0");
      u8g2.print(m);
      u8g2.print(":");
      if (s < 10)u8g2.print("0");
      u8g2.print(s);
      u8g2.setCursor(8, 30);
      u8g2.print(battery.voltage());
    } while ( u8g2.nextPage() );

  }



  if (hBtn.wasReleased()) {
    h = h + 1;
    timeon= millis();
    }
    
  if (myBtn.wasReleased()) {
    m = m + 5;
    timeon= millis();
    }
    

  // ---- manage  minutes, hours overflow ----

  if (m == 60)
  {
    m = 0;

  }
  if (h == 13)
  {
    h = 0;

  }
  if (myBtn.pressedFor(LONG_PRESS) & (sl == 0))
  {

    u8g2.setPowerSave(1);
    mfrc522.PCD_SoftPowerDown();
      // Allow wake up pin to trigger interrupt on low.
  attachInterrupt(0, wakeUp, LOW);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

  }
if (hBtn.pressedFor(VERY_LONG_PRESS) )
  {

    u8g2.firstPage();
    do {
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.setCursor(8,20);


    u8g2.print("Scan");

    } while ( u8g2.nextPage() );

    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0

    }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( uint8_t j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
 
 
    for ( uint8_t i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
  }
   readCard[0] = 0;
      readCard[1] = 0;
      readCard[2] = 0;
      readCard[3] = 0;
  }


  while ( (sBtn.wasReleased() ) & (sl == 0) ) {
u8g2.firstPage();
    do {
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.setCursor(8,11);
    u8g2.print("Sure?");
    u8g2.setCursor(8,22);
    u8g2.print("Yes ^");
        u8g2.setCursor(8,32);
            u8g2.print("No v");



    } while ( u8g2.nextPage() );u8g2.firstPage();
     myBtn.read();               // read the button
  hBtn.read();               // read the button
   if (hBtn.wasPressed()){
     T.SetTimer(h, m, s);
    T.StartTimer();
    mfrc522.PCD_SoftPowerDown();

    //digitalWrite(RST_PIN, LOW);
    u8g2.setPowerSave(1);
    sl = 1;
    }
    if (myBtn.wasPressed()){
      break;
      }
    
   
  }

   if ( (sl == 0) & ((millis() - timeon)>60000)){
    
     u8g2.setPowerSave(1);
    mfrc522.PCD_SoftPowerDown();
    //digitalWrite(RST_PIN, LOW);
      // Allow wake up pin to trigger interrupt on low.
  attachInterrupt(0, wakeUp, LOW);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    
    }
  while ((T.TimeCheck(0, 0, 0)) & (sl == 1)) {
    u8g2.setPowerSave(0);

    mfrc522.PCD_SoftPowerUp();
    // mfrc522.PCD_Init();

    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_profont11_tf);
      u8g2.setCursor(8, 20);

      u8g2.print("wake up");
      u8g2.setCursor(8, 30);
      u8g2.print(battery.voltage());
    } while ( u8g2.nextPage() );

    digitalWrite(5, HIGH);
    delay(30);
    digitalWrite(5, LOW);
    delay(200);
    digitalWrite(5, HIGH);
    delay(60);
    digitalWrite(5, LOW);
    delay(320);
    getID();
    if ( isMaster(readCard) ) { 
      sl = 0;
      u8g2.setPowerSave(1);
      //     digitalWrite(RST_PIN, LOW);
      mfrc522.PCD_SoftPowerDown();
        // Allow wake up pin to trigger interrupt on low.
  attachInterrupt(0, wakeUp, LOW);
      readCard[0] = 0;
      readCard[1] = 0;
      readCard[2] = 0;
      readCard[3] = 0;
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);


    }
      hBtn.read();               // read the button

    if (hBtn.pressedFor(VERY_LONG_PRESS) )
  {

    u8g2.firstPage();
    do {
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.setCursor(8,20);


    u8g2.print("Scan");

    } while ( u8g2.nextPage() );

    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
digitalWrite(5, HIGH);
    delay(30);
    digitalWrite(5, LOW);
    delay(200);
    digitalWrite(5, HIGH);
    delay(60);
    digitalWrite(5, LOW);
    delay(320);
    }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( uint8_t j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
 
 
    for ( uint8_t i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
  }
   readCard[0] = 0;
      readCard[1] = 0;
      readCard[2] = 0;
      readCard[3] = 0;
  }
  }

if (battery.voltage()<3600){
   u8g2.firstPage();
    do {
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.setCursor(8,11);


    u8g2.print("Low Batt");
     u8g2.setCursor(8,22);


    u8g2.print("Pls Chrg");

    } while ( u8g2.nextPage() );
    delay(2000);
     sl = 0;
      u8g2.setPowerSave(1);
      //     digitalWrite(RST_PIN, LOW);
      mfrc522.PCD_SoftPowerDown();
        // Allow wake up pin to trigger interrupt on low.
  attachInterrupt(0, wakeUp, LOW);
      readCard[0] = 0;
      readCard[1] = 0;
      readCard[2] = 0;
      readCard[3] = 0;
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  
  }
}

uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Scanned PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];

  }

  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

bool checkTwo ( byte a[], byte b[] ) {
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] ) {     // IF a != b then false, because: one fails, all fail
      return false;
    }
  }
  return true;
}


bool isMaster( byte test[] ) {
  return checkTwo(test, masterCard);
}
