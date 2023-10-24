//void dumpArray(void) {
//
//  // Constants for main key press history array
////#define STATUS 0   // current status of key
////#define CURRENT 1  // current key position value
////#define MARGIN 2   // key position value to indicate off
////#define AVERAGE 3  // average closed key value
////#define TRIGGER 4  // key position value to indicate on
////#define MIN 5      // minimum key voltage recorded
//
//  for (int mux = 0; mux < muxcount; mux++) {
//    for (int i = 0; i < 16; i++) {
//
//      Serial.print("mux="); Serial.print(mux); Serial.print(" \tkey="); Serial.print(i); Serial.print(" \tStatus="); Serial.print(muxhist[mux][i][STATUS]);
//      Serial.print(" \tCurrent="); Serial.print(muxhist[mux][i][CURRENT]); Serial.print(" \tMargin="); Serial.print(muxhist[mux][i][MARGIN]);
//      Serial.print(" \tAverage="); Serial.print(muxhist[mux][i][AVERAGE]); Serial.print(" \tTrigger="); Serial.println(muxhist[mux][i][TRIGGER]);
//    }
//  }
//      //  while(1);
//}
