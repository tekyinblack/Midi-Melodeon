//
/***************************************************************************
  This is a library for the BMP280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BMEP280 Breakout
  ----> http://www.adafruit.com/products/2651

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
/*
   Version 3
   Rewrite to prepare for new hardware build
   Flexible number of muxes - new architecture will see one mux per reed block
   SysEx selection of GM tables for standardisation
   Prepare for bluetooth
   Add reset and special key combination code
*/
/*
   Version 4
   New hardware build
   Split between Accordion and Midi synthesizer code
       Accordion code is only a controller issuing pressure and keypress information
   Power on button support
   Narcoleptic power sleep mode
   Mux rewrite
   Pinout
   A0-2 Mux inputs
   A3 power on switch input
   A4-5 I2C to BMP280
   D0-1 Standard RX/TX
   D2-3 Midi output for testing
   D4 Power on button power supply
   D5 Buzzer activation
   D6 Bluetooth Status
   D7 Mux and BMP280 power supply
   D8 Bluetooth power supply
   D9-12 Mux selection
   D13 detector power switch
*/


#include <avr/pgmspace.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <SoftwareSerial.h>
#include <Narcoleptic.h>


float averagep;
float diffp;

Adafruit_BMP280 bmp; // I2C

SoftwareSerial xmitout(2, 3);

#define PUSH 0   // key translation for push direction
#define PULL 1   // key translation for pull direction 

// Constants for main key press history array
#define STATUS 0   // current status of key
#define CURRENT 1  // current key position value
#define MARGIN 2   // key position value to indicate off
#define AVERAGE 3  // average closed key value
#define TRIGGER 4  // key position value to indicate on
#define MIN 5      // minimum key voltage recorded



// digital pins allocation
#define POWERBUTTON 4    // power button sensor
#define BUZZER 5         // Activate Buzzer
#define BTSTATUS 6       // BT status input
#define MUXPOWER 7       // power to key muxes
#define BTPOWER 8        // power to bluetooth
#define DETECTORS 13     // power to switch sensors

// Power wait timers
#define KBDWAIT 60000         // if keyboard not touched for 1 minute then turn keyboard power off
#define PRESSUREWAIT 540000   // if no pressure change in bellows for 9 minutes then power off


float newp;              // storage for read pressure
float ndiffp;            // actual difference of pressure to average
int absdiffp;            // absolute version of avdiffp
int avdiffp;             // average version of ndiffp
int bellows = PUSH;      // Initial status is PUSH
int prev_bellows = PUSH; // keeps last bellows direction to indicate change
int expression = 0;      // default expression volume is 0, silent
int prev_expression = 0; // keeps last expression value to indicate change


// keyboard control
int muxcount = 3;                 // number of multiplexers in use, 3 x 16 buttons
int muxID[4] = {9, 10, 11, 12} ;  // digital pins used for multiplexer addresses

const int muxmap[16][4]  = {
  {0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 1, 0}, {1, 0, 1, 0}, {0, 1, 1, 0}, {1, 1, 1, 0},
  {0, 0, 0, 1}, {1, 0, 0, 1}, {0, 1, 0, 1}, {1, 1, 0, 1}, {0, 0, 1, 1}, {1, 0, 1, 1}, {0, 1, 1, 1}, {1, 1, 1, 1},
};
int muxhist[3][16][5]; // storage array for main keyboard
int powerTrigger = 0; // trigger value for power button
int powerMargin = 0; // margin value for power button
int powerAverage = 0; // average value for power button
int powerCurrent = 0; // last value for power button
int powerStatus = 0;  // last key status for power button
long powerTimer = 0;  // timer storage for power on

int powerState = 0;

int tempReading = 0;



/**********************************************************************************
   Start of setup routine
*/
void setup() {
  // Initializes serial port for debug.
  Serial.begin(115200);
//  Serial.println("Start");
//  Serial.print("Sketch:   ");   Serial.print(__FILE__);
//  Serial.print("  Uploaded: ");   Serial.print(__DATE__);  Serial.print(" "); Serial.println(__TIME__);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(5,LOW);
  pinMode(7, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  powerUp();    // turn on all power sources

    // initialise the pressure sensor and values
  delay(100);
  if (!bmp.begin()) {
//    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    playBmpError();
    while (1);
  }
//  Serial.println("bmp init ok");
  /* Default settings from datasheet. */
  //  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */  ORIGINAL SETTINGS
  //                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
  //                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
  //                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
  //                  Adafruit_BMP280::STANDBY_MS_1); /* Standby time. */

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X1,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X2,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_OFF,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_1); /* Standby time. */


  xmitBegin();   // initialise BT transmissions
 //   delay(500);
  reinit();      // run calibration of keyboard and bellows
  setBuzzer(1);
}

/* Initialisation routine reloads the average values again

*/
void reinit() {
//    Serial.println("Start reinit");
  // Load the current pressure over a period of one second to get an average
  for (int i = 0; i < 10; i++) {
    averagep += bmp.readPressure();
    delay(100);
  }
  averagep = averagep / 10;

 // Serial.print("averagep = ");
//  Serial.println(averagep);

  // Read the baseline key values and populate the key history array

  for (int k = 0; k < 5; k++) { // loop through 5 times to get an average
    powerAverage += analogRead(3); // seperate read of power key not connected to mux
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 4; j++) { // set key selection pins
        digitalWrite(muxID[j], muxmap[i][j]);
      }
      // delay(1);
      for (int mux = 0; mux < muxcount; mux++) { // read each mux value
        delayMicroseconds(50); // wait for 50 microseconds to let values settle <<<<<---delay put here to account for mux in Arduino ADC
        tempReading = analogRead(mux);  // temporary read to switch ADC mux. Actual delay is 200 microseconds due to read time of ADC
        delayMicroseconds(50); // wait for 50 microseconds to let values settle <<<<<---delay put here to account for mux in Arduino ADC
        muxhist[mux][i][AVERAGE] += analogRead(mux);
      }
    }
  }

  // Calculate initial values for average closed reading, the ON trigger value and the OFF trigger value.
  // Also populate the CURRENT and MIN value fields for use here and later when in use.

  powerAverage = powerAverage / 5;  // separate calculations just for power button not part of mux
  powerTrigger = powerAverage * 9 / 10;
  powerMargin = powerAverage * 19 / 20;

  for (int i = 0; i < 16; i++) {

    for (int mux = 0; mux < muxcount; mux++) { // loop through the array to calculate the initial key measurements
      //  Calculate average key value
      muxhist[mux][i][AVERAGE] = muxhist[mux][i][AVERAGE] / 5;
      muxhist[mux][i][CURRENT] = muxhist[mux][i][AVERAGE];

      //  Calculate key trigger value as 90% of key average value minus the key difference value

      muxhist[mux][i][TRIGGER] = muxhist[mux][i][AVERAGE] * 9 / 10;
      muxhist[mux][i][MARGIN] = muxhist[mux][i][AVERAGE] * 19 / 20;
    }
  }
 // Serial.println("Reinit complete");
 // dumpArray();
//  Serial.println("Start monitor");

}
// Soft reset of system from start, same as poweron reset. Branches to memory location zero.
void(* resetSoft)(void) = 0;

/*************************************************************************************************************
   Main loop, continuously reading the status of each key sensor
   As each opens past the TRIGGER value, it records a key ON signal
   As each closes above the MARGIN value, it records a key OFF signal
   For each cycle round the keys, a pressure measurement is made and on the value obtained
   the following is sent
      If positive pressure then push signal sent
      If negative pressure, then pull signal sent
      If pressure drops below threshold, then a stop signal sent
*/
void loop() {
  /*
    Loop

  */
  xmitCheck(0);   // check and change BT status if necessary
  playBuzzer();   //check and change tune if necessary

  
  powerControl(1);  // check if power status must change


  // Get the pressure to compute an expression value which drives the actual sound volume output
  // as it would do in a real instrument
  //
  newp = bmp.readPressure();   // get pressure, BMP280 is set to automatically read value every 8ms
  ndiffp = newp - averagep;    // calculate pressure difference
                               // and magnitude

  avdiffp = ndiffp;             //temp kludge
  absdiffp = abs(avdiffp);       


  if (absdiffp < 20) expression = 0;                   // Anything under 20 we ignore as air button or small bellows movement
  else {
    if ((absdiffp <= 30) & (expression > 0))             // If under 30 and already sounding then continue, this adds some hysterisis
      expression = constrain((absdiffp), 0, 127);
  }
  if (absdiffp > 30) expression = constrain((absdiffp), 0, 127); // definite pressure above 30, set an expression value
  if (expression > 0) {                                             // and also set a bellows direction
    if (avdiffp > 0.0) bellows = PUSH;
    else bellows = PULL;
  }

  // **************************** TEST values expression = 0 or 127

  if (expression > 0) {
    expression = 127;
    powerTimer = millis(); // if there is change in expression leave power timer running
  }
  if (expression != prev_expression) {                // avoid sending expression volume messages if no change
    // send expression value  because of change
    xmitBellows(expression,bellows);
  }

  if (expression == 0 and prev_expression != 0) {                  // if expression = 0 then turn off all notes and reset status's
    // midi turn all notes off                        the notes will be turned on again with the next key pass,
    //                                          as a real instrument behaves when the musician stops pushing or pulling
    xmitBellows(expression,0);
    // set all key statuses to off
    for (int i = 0; i < 16; i++) {
      for (int mux = 0; mux < muxcount; mux++) {
        muxhist[mux][i][STATUS] = 0;
      }
    }
  }
  prev_expression = expression;

  if (bellows != prev_bellows) {                  // if bellows direction changes then turn off all notes and reset status's
    // midi turn all notes off                        the notes will be turned on again with the next key pass in the correct direction
    xmitBellows(expression,0);
    // set all key statuses to off
    for (int i = 0; i < 16; i++) {
      for (int mux = 0; mux < muxcount; mux++) {
        muxhist[mux][i][STATUS] = 0;
      }
    }
    prev_bellows = bellows;

  //  Serial.print("Bellows= ");
  //  Serial.println(bellows);
    //   Serial.print("PrevExpression= ");
    //   Serial.println(prev_expression);
  }


  for (int i = 0; i < 16; i++) {             // cycle round the pins to select values

    for (int j = 0; j < 4; j++) {            //  cycle round the bitmap to select the next mux value
      digitalWrite(muxID[j], muxmap[i][j]);
    }

    for (int mux = 0; mux < muxcount; mux++) {
      if (muxhist[mux][i][AVERAGE] > 50) {  //Check, if this pin is unused then bypass

        delayMicroseconds(50); // wait for 50 microseconds to let values settle <<<<<---delay put here to account for mux in Arduino ADC
        tempReading = analogRead(mux);  // temporary read to switch ADC mux. Actual delay is 200 microseconds due to read time of ADC
        delayMicroseconds(50); // wait for 50 microseconds to let values settle <<<<<---delay put here to account for mux in Arduino ADC

        muxhist[mux][i][CURRENT] = analogRead(mux);


        if (muxhist[mux][i][CURRENT] < muxhist[mux][i][TRIGGER]) {  // if measured value less than trigger then process note on
          if ( !muxhist[mux][i][STATUS] ) {
            //Trigger midi ON

            xmitKeyDown(mux,i,127);                // transmit key down event, max velocity until implemented
            muxhist[mux][i][STATUS] = 1;           // trigger note ON
            powerControl(0);                       // reset power timer
            // Debug
//            Serial.print(" Mux ");
//            Serial.print(mux);
//            Serial.print(" key ");
//            Serial.print(i);
//            Serial.print(" Cur ");
//            Serial.print(muxhist[mux][i][CURRENT]);
//            Serial.print(" Trg ");
//            Serial.println(muxhist[mux][i][TRIGGER]);
          }
        }

        if (muxhist[mux][i][CURRENT] > muxhist[mux][i][MARGIN]) {  // if measured value more than margin then process note off
          if ( muxhist[mux][i][STATUS] ) {
            //Trigger midi OFF
            xmitKeyUp(mux,i,127);                // transmit key up event, max velocity until implemented
            muxhist[mux][i][STATUS] = 0;            // trigger note OFF
            powerControl(0);                     // reset power timer
//            Serial.print("OFF  ");
//            Serial.print(" Mux ");
//            Serial.print(mux);
//            Serial.print(" key ");
//            Serial.print(i);
//            Serial.print(" Cur ");
//            Serial.print(muxhist[mux][i][CURRENT]);
//            Serial.print(" Mgn ");
//            Serial.println(muxhist[mux][i][MARGIN]);
          }
        }
      }
    }
  }
  powerCurrent = 0;
  powerCurrent = analogRead(3);
 
  if (powerCurrent < powerTrigger) {  // if measured value less than trigger then process note on
    if (powerStatus == 0) {
      powerStatus = 1;
      if (powerState == 0) xmitKeyDown(4,0,127);            // transmit power key down event, mux=4, key = 0
      // Debug
//      Serial.print("ON   PowerButton");
//                  Serial.print(" Cur ");
//            Serial.print(powerCurrent);
//            Serial.print(" Trg ");
//            Serial.println(powerTrigger);
    }
  }
  if (powerCurrent > powerMargin) {  // if measured value more than margin then process note off
    if (powerStatus == 1) {
      powerStatus = 0;
      if (powerState == 0) xmitKeyUp(4,0,127);            // transmit power key up event, mux=4, key = 0
      powerControl(2);               // power button reset event, only on release
//      Serial.print("OFF  PowerButton");
//                  Serial.print(" Cur ");
//            Serial.print(powerCurrent);
//            Serial.print(" Mgn ");
//            Serial.println(powerMargin);
    }
  }

}
