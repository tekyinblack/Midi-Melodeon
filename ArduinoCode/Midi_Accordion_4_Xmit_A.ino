/*
   Routines to translate keypresses into raw transmissions or midi messages

   xmitBegin - start transmission port
   xmitInit  - initial transmission parameters
   xmitSynch - transmit 4 bytes of high values to indicate start of keyboard scan and data transmission
*/
int xmitStatus = 2;
long xmitTimer = 0;
union {
  struct {
    uint8_t synch;
    uint8_t command;
    uint8_t dummy[3]; 
  } header;
  struct {
    uint8_t dummy[2]; 
    uint8_t mux;
    uint8_t key;
    uint8_t velocity;
  } keypress;
  struct {
    uint8_t dummy[2]; 
    uint8_t expression;
    uint8_t direction;
    uint8_t dummy2;
  } bellows;
  struct {
    uint8_t dummy[2]; 
    uint8_t change;
    uint8_t parameter1;
    uint8_t parameter2;
  } state;   
  uint8_t buf[4];    
} xmit;
void xmitBegin(void) {
  xmitout.begin(115200); // Setup midi serial port
  xmit.header.synch = 0xFF;
  
}
void xmitPowerOn(void) {
  xmit.header.command = 0x08;     // state change
  xmit.state.change = 0x01;       // power change
  xmit.state.parameter1 = 0x01;   // ordinary power on
  xmitSendBuf();
}

void xmitPowerOff(void) {
  xmit.header.command = 0x08;     // state change
  xmit.state.change = 0x02;       // power change
  xmit.state.parameter1 = 0x01;   // full power off
  xmitSendBuf();
}

void xmitSendBuf() {
  if (xmitCheck(2) == 0) {
    for (int i=0; i<5; i++) {
      xmitout.write(xmit.buf[i]);
    }
  }
}
void xmitBellows(uint8_t expression, uint8_t direction) {
  xmit.header.command = 0x04;           // send bellows data
  xmit.bellows.expression = expression; // relative bellows pressure
  xmit.bellows.direction = direction;   // bellows direction 0-not moving, 1-push, 2-pull
  xmitSendBuf();
}

void xmitKeyDown(uint8_t mux, uint8_t key, uint8_t velocity) {
  xmit.header.command = 0x01;        // key pressed
  xmit.keypress.mux = mux;           // mux number
  xmit.keypress.key = key;           // key number
  xmit.keypress.velocity = velocity; // key number
  if (powerState == 0) xmitSendBuf();
  xmitCheck(1);      // user has pressed key, if BT dow, alert user
}

void xmitKeyUp(uint8_t mux, uint8_t key, uint8_t velocity) {
  xmit.header.command = 0x02;        // key released
  xmit.keypress.mux = mux;           // mux number
  xmit.keypress.key = key;           // key number
  xmit.keypress.velocity = velocity;  // key number
  if (powerState == 0) xmitSendBuf();
}
/*
 * xmitCheck returns a status for bluetooth transmission to ensure that the player is alerted
 * to the BT availability. BT is automatic and there isn't much the player can do except ensure that 
 * both end of the connection are powered on. When the connection is established, an OK signal is sent
 * The routine includes a delay to ensure that the player isn't alerted to an absence of BT when the 
 * instrument first powers on, which may be annoying. To save power, when BT is unavailable, the keyboard 
 * is powered down. This also reduces power consumption in other circuts when the BT module needs the most 
 * power to operate.
 * When the player attempts to press a key and BT down, a warning tune is played. 
 * 
 * xmitStatus = 2 if startup condition, 1 if down and 0 if ok.
 * 
 */
int xmitCheck(int test) {
    if (test==2) {                    // If test is 2, returns status
      return xmitStatus;
     }
  if (test==1) {                    // If test is 1, is a call from keypress and warns user BT is down
    if (xmitStatus != 0) {
      setBuzzer(BUZNOBT);
      return xmitStatus;
    }
  }
 
  switch (xmitStatus) {
    case 0:                     // check normal status, will return 0 if all ok or 1 if down
      if (digitalRead(6) == 0) {
        xmitStatus = 1;
        setBuzzer(BUZNOBT);
        xmitTimer = millis() + 20000;
      }
      break;
    case 1:                     // check regularly and issue sound alert if not connected every 10 seconds
      if (digitalRead(6) == 0) {
        if (xmitTimer < millis()) {
          setBuzzer(BUZNOBT);
          xmitTimer = millis() + 20000;
        }
      } 
      else {
        xmitStatus = 0;
        setBuzzer(BUZBT);
      }
      break;
    case 2:                     // startup condition, set timer if down, but set
      if (digitalRead(6) == 0) {
        xmitTimer = millis() + 20000;
        xmitStatus = 1;
      }
      else {
        xmitStatus = 0;
        setBuzzer(BUZBT);
      }
      break;
  }
  return xmitStatus;
}
