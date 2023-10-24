// Power Status
/*  There are 3 power statuses:
    After reset, all power is switched on and the instrument gets ready for playing.
    The pressure is checked, the keyboard is read and initialised and bluetooth communication is established
    when powerup complete, an audible signal is given

    When the a key hasn't been pressed for a while, the keyboard is powered off
    to save battery power. An audible signal is given

    When the keyboard is powered off and power button is pressed, power is reapplied to the keyboard in the expectation that
    it is about to be played. An audible signal is given.

    When the keyboard has been powered off for some time, a full powerdown is performed which disconnects the
    bluetooth connection and puts the cpu into a deep sleep which can only be exited from via the power button routine or
    by removing and reapplying the power.

    In deep sleep, the cpu is suspended for one second before waking to switch on the power button. The power button is
    checked to see if it depressed, and if so, a full reset is performed and the instrument goes through a full power up cycle.
    If not depressed, the cpu goes back to sleep for another second. This power test cycle uses approximately 20mA for
    1 millisecond every second, or approximatley 20uA average, giving it a lifetime of several years on AA batteries.

    When fully powered up, the instrument uses around 250mA. The automatic power off is required as the instrument is unaltered
    so has no physical power buttons. Saving the batteries is important as to replace them requires the instrument to be openned
    Rechargable batteries will be a future implementation but current OTS technology does not work reliably with the instrument
    construction.

    Power off can also be achieved via a keyboard button combination
*/

#define KBD_DELAY 60000           // inactive time before keyboard is powered off, typically a minute
#define POWER_DELAY 60000         // inactive time before instrument powered off, typically 9 minutes

/*
 * powerControl is called to check that the musician is playing and if not cut power usage
 */
void powerControl(int powerCheck) {  // 0 resets timer, 1 checks activity, powers OFF keyboard if none, 2 restarts timer, powers on keyboard if off
  switch (powerCheck) {
    case 0:                   // positive activity. reset timer
      powerTimer = millis() + KBD_DELAY;
      return;
    case 1:                   // check for activity, if recently active, return
      if (powerTimer > millis()) return;
      switch (powerState) {
        case 0:           // if timeout and power state is up, power off keyboard, change state, new timer
          powerOffKbd();
          powerState = 1;
          powerTimer = millis() + POWER_DELAY;
          return;
        case 1:          // if keyboard powered off and timer expired, power off and sleep
          powerDown();
          powerSleep();
          resetSoft();
          return;
        }
      case 2:            // used on power button, as 0 if powered on, powers on keyboard if off
        powerOnKbd();
        powerState = 0;
        powerTimer = millis() + KBD_DELAY;
        return;
    }
    //

  }
/*
 *  powerSleep cycles round powering up the power button, reading the value and shutting down again if not pressed
 *  this results in the instrument sleeping for 999ms out of every second, power on time uses 20mA for 1ms
 */

  void powerSleep(void) {
  while (1) {
    // delay(999);   // replace with narcoleptic after testing
    Narcoleptic.delay(999);
    //sleep 999 ms
    powerOnPower();    // power on power button
    delay(1);
    if (analogRead(3) < powerTrigger) {     // if power button depressed, return
      while (analogRead(3) < powerMargin);  // wait until key is released
      return;
    }
    powerOffPower();    // power off power button again
  }
}
/*
 *  Power down alerts the BT receiver that power is going off , alerts the musician and then turns off the power
 */
void powerDown(void) {
  xmitPowerOff();    // signal power off to BT receiver
  powerOffKbd();  // shouldn't be necessary but just in case
  powerOffMux();  // cut power to mux and bmp280
  setBuzzer(BUZSLP);  // play sleep tune
  while (playingTune == BUZSLP) {   // loop until tune ended
    playBuzzer();
  }
   delay(1000);
  powerOffBt();   // power off bluetooth
  powerOffPower();    // power off power button, not really necessary but for completeness sake
}
/*
 *  Power up switches on the power and alerts the musician
 */
void powerUp(void) {
  powerOnPower();      // poweron power button
  powerOnMux();        // power on bmp and muxes
  powerOnBt();         // power on BT and auto connect
  powerOnKbd();        // power on the keyboard
  powerTimer = millis() + KBD_DELAY;   // initialise power control timer
  buzzerOn();          // switch buzzer on to indicate power up
  delay(100);          // wait 100ms
  buzzerOff();         // switch buzzer off
  //   Serial.println("Power on complete");
}
/*
 * Individual power off/on functions, created to separate the function from the main code
 */
void powerOffKbd(void) {
//  Serial.println("Power off keyboard");
       for (int j = 0; j < 4; j++) { // set mux selectors to low
        digitalWrite(muxID[j], LOW);
      }
  digitalWrite(DETECTORS, LOW);
}
void powerOnKbd(void) {
 // Serial.println("Power ON keyboard");
  digitalWrite(DETECTORS, HIGH);
}
void powerOffMux(void) {
//  Serial.println("Power off Mux");
  digitalWrite(MUXPOWER, LOW);
}
void powerOnMux(void) {
//  Serial.println("Power ON Mux");
  digitalWrite(MUXPOWER, HIGH);
}
void powerOffBt(void) {
//  Serial.println("Power off BT");
  digitalWrite(BTPOWER, LOW);
}
void powerOnBt(void) {
 // Serial.println("Power ON BT");
  digitalWrite(BTPOWER, HIGH);
}
void powerOffPower(void) {
//  Serial.println("Power off PwrBtn");
  digitalWrite(POWERBUTTON, LOW);
}
void powerOnPower(void) {
//  Serial.println("Power ON PwrBtn");
  digitalWrite(POWERBUTTON, HIGH);
}
