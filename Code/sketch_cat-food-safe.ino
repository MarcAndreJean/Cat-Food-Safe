/*  
 *  Frequency : RFID 125 kHz Module
 *  Card Format : EM 4001 or compatible (4100)
 *  Encoding Manchester 64-bit, modulus 64
 *  Input Data Structure : ASCII - 9600 Baud, No Parity, 1 stop bit.
 *  
 *  Based on ID-2LA, ID-12LA, ID-20LA.
 *  Remark: disconnect the rx serial wire to the ID-12 when uploading the sketch.
 *  
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  
 *  Version:           1.0
 *  Marc-André Jean
 */

 
// Constants, define and globals.
// ------------------------------------------------------------------------------------------
#define SLEEP_TIME 1000 // ms.
#define AWAKE_IT_TIME 10 // Number of read() we done before we re-enter sleep mode.
#define WHITELIST_LEN 2
const byte whitelist[WHITELIST_LEN][5] = {{0x00, 0x00, 0x00, 0x00, 0x01},
                                          {0x00, 0x00, 0x00, 0x00, 0x02}};
// Pin 1 and 2 / RX, TX for RFID reader (serial).
#define powerRFIDpin 3 // Relay or transistor which control current to RFID reader.
#define powerTrayPin 4 // Relay or transistor which control current to tray motor.
#define bridgeTrayPin 5 // Bridge which control current sign to tray motor.
#define stoppperOpenPin 6 // Switch which is active when tray is completely open.
#define stoppperClosePin 7 // Switch which is active when tray is completely close.
byte key[5];
bool auth;
bool trayState;


// Definitions.
// ------------------------------------------------------------------------------------------
bool read_RFID(byte*);
bool whitelisting(byte*);
void checkstate(bool);
void openTray();
void closeTray();


// setup(). Arduino call it once at startup.
// ------------------------------------------------------------------------------------------
void setup() {
  // Setup for the RFID Reader.
  pinMode(powerRFIDpin, OUTPUT);
  pinMode(powerTrayPin, OUTPUT);
  pinMode(bridgeTrayPin, OUTPUT);
  pinMode(stoppperOpenPin, INPUT);
  pinMode(stoppperClosePin, INPUT);
  // Set initial states.
  digitalWrite(powerRFIDpin, HIGH);
  digitalWrite(powerTrayPin, LOW);
  digitalWrite(bridgeTrayPin, LOW);
  openTray();
}


// loop(). Arduino run it repeatedly.
// ------------------------------------------------------------------------------------------
void loop() {
  // Open the RFID Reader.
  digitalWrite(powerRFIDpin, HIGH);
  Serial.begin(9600);

  for (int i = 0; i < AWAKE_IT_TIME; ++i) {
    // Read key
    if (read_RFID(key) == false) {
      continue;
    }
    // Check and apply whitelisting.
    auth = whitelisting(key);
    checkstate(auth);
  }
  
  // Enter sleep mode.
  Serial.end();
  digitalWrite(powerRFIDpin, LOW);
  // Sleep.
  delay(SLEEP_TIME);
}


// Functions for loop()
// ------------------------------------------------------------------------------------------
// --read_RFID()
bool read_RFID(byte key[]) {
  // Based on code by BARRAGAN <http://people.interaction-ivrea.it/h.barragan> 
  // and code from HC Gilje - http://hcgilje.wordpress.com/resources/rfid_id12_tagreader/
  // and http://playground.arduino.cc/Code/ID12
  int i;
  byte val = 0;
  byte code[6];
  byte checksum = 0;
  byte bytesread = 0;
  byte tempbyte = 0;

  if(Serial.available() > 0) {
    // Check for header. 
    if((val = Serial.read()) == 2) {
      bytesread = 0; 
      // Read 10 digit code + 2 digit checksum.
      while (bytesread < 12) {
        if( Serial.available() > 0) { 
          val = Serial.read();
          // If header or stop bytes before the 10 digit reading.
          if( val == 0x0D || val == 0x0A || val == 0x03 || val == 0x02 ) { 
            break; // Stop reading.
          }

          // Do Ascii/Hex conversion:
          if (val >= '0' && val <= '9') {
            val = val - '0';
          } else if (val >= 'A' && val <= 'F') {
            val = 10 + val - 'A';
          }

          // Every two hex-digits, add byte to code:
          if (bytesread & 1 == 1) {
            // Make some space for this hex-digit by
            // shifting the previous hex-digit with 4 bits to the left:
            code[bytesread >> 1] = (val | (tempbyte << 4));

            // If we're at the checksum byte, Calculate the checksum... (XOR) :
            if (bytesread >> 1 != 5) {
              checksum ^= code[bytesread >> 1];
            }
          } else {
            // Store the first hex digit first...
            tempbyte = val;
          }

          // Ready to read next digit.
          bytesread++;
        } 
      } 

      // Output to Serial:
      // If 12 digit read is complete.
      if (bytesread == 12 && code[5] == checksum ) {
        for (i = 0; i < 5; ++i) {
          key[i] = code[i];
        }
        return true;
      }
      bytesread = 0;
    }
  }
  // Else, return false.
  return false;
}


// --whitelisting()
bool whitelisting(byte key[]) {
  // For each whitelist_key in whitelist[][] :
  for (int i = 0, j; i < WHITELIST_LEN; ++i) {
    // Compare each bytes :
    for (j = 0; j < 5; ++j) {
      // If bytes equal, compare next byte :
      if (key[j] == whitelist[i][j]) {
        // If each bytes are equal, return True. Key is in whitelist.
        if (j == 4) {
          return true;
        }
        continue;
      } else { // Else, fetch next whitelist_key.
        break;
      }
    }
  }
  // End of for-loop, no matching key in whitelist. Return false.
  return false;
}


// --checkstate()
void checkstate(bool auth) {
  if (trayState == auth) {
    return;
  }
  trayState = auth;
  if (trayState == true) {
    openTray();
  } else {
    closeTray();
  }
}


// --openTray()
void openTray() {
  // Active motor of tray; set bridge to open(LOW).
  digitalWrite(bridgeTrayPin, LOW);
  digitalWrite(powerTrayPin, HIGH);
  
  // Wait until tray is opened.
  for(;;) {
    if (digitalRead(stoppperOpenPin) == HIGH) {
      break;
    }
    delay(250);
  }
  
  // Turn off motor.
  digitalWrite(powerTrayPin, LOW);
}


// --closeTray()
void closeTray() {
  // Active motor of tray; set bridge to close(HIGH).
  digitalWrite(bridgeTrayPin, HIGH);
  digitalWrite(powerTrayPin, HIGH);
  
  // Wait until tray is closed.
  for(;;) {
    if (digitalRead(stoppperClosePin) == HIGH) {
      break;
    }
    delay(250);
  }
  
  // Turn off motor.
  digitalWrite(powerTrayPin, LOW);
}



