// Buzzer Tunes
/*
   tones are separated by a 100ms break
   0 represents a 100ms space
   1 represents a 100ms tone
   2 represents a 200ms tome

   Tunes are
*/
#define BUZOK 1   // OK
#define BUZNOBT 2 // no BT connection
#define BUZBT 3   // BT connected
#define BUZLP 4   // going to low power
#define BUZSLP 5  // going to sleep
#define BUZBMP 6  // pressure sensor not working
#define BUXLB  7  // low battery warning


int tunes[][3] = {{0, 0, 0}, {1, 1, 1}, {1, 2, 1}, {1, 2, 2}, {2, 1, 2}, {2, 2, 2}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}};
int playingTune = 0;
long buzzerTimer = 0;
int buzzerNote = 0;

void buzzerOff(void) {
  //    Serial.print("Buzzer off ");Serial.println(millis());
  digitalWrite(BUZZER, LOW);
}
void buzzerOn(void) {
  // Serial.print("Buzzer on ");Serial.println(millis());
  digitalWrite(BUZZER, HIGH);
}

void setBuzzer(int tune) {

  if (tune < 9 && playingTune == 0) {
    playingTune = tune;
    //    Serial.print("Tune set to ");Serial.println(playingTune);
  }

}
void playBuzzer() {
  //return;
  if (playingTune == 0) return;

  if (buzzerTimer > millis()) return; // no change in tune yet
  // Serial.print("Tune Change  Tune ="); Serial.print(playingTune); Serial.print(" Note="); Serial.println(buzzerNote);
  switch (buzzerNote) {
    case 0:
      if (tunes[playingTune][0] > 0) buzzerOn();
      buzzerTimer = millis() + 100;
      if (tunes[playingTune][0] > 1) buzzerTimer = buzzerTimer + 100;
      buzzerNote = 1;
      break;
    case 1:
      buzzerOff();
      buzzerTimer = millis() + 100;
      buzzerNote = 2;
      break;
    case 2:
      if (tunes[playingTune][1] > 0) buzzerOn();
      buzzerTimer = millis() + 100;
      if (tunes[playingTune][1] > 1) buzzerTimer = buzzerTimer + 100;
      buzzerNote = 3;
      break;
    case 3:
      buzzerOff();
      buzzerTimer = millis() + 100;
      buzzerNote = 4;
      break;
    case 4:
      if (tunes[playingTune][2] > 0) buzzerOn();
      buzzerTimer = millis() + 100;
      if (tunes[playingTune][2] > 1) buzzerTimer = buzzerTimer + 100;
      buzzerNote = 5;
      break;
    case 5:
      buzzerOff();
      buzzerTimer = 0;
      buzzerNote = 0;
      playingTune = 0;
      break;
      //  default:
  }
  //  buzzerNote++;
  //  if (buzzerNote > 5) {
  //    buzzerNote=0;
  //    playingTune = 0;
  //    }
}
void playBmpError(void) {
  setBuzzer(BUZBMP);  // play sleep tune
  while (playingTune == BUZBMP) {   // loop until tune ended
    playBuzzer();
  }
}
