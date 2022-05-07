/*
* Code based on DRAM Tester: https://github.com/abzman/DRAM-tester
* 
* This code is for the DRAM Liberator by Stephen Arsenault
* https://github.com/Stephen-Arsenault/DRAM-Liberator
* 
* License: CC BY-SA 4.0
*/

/* ================================================================== */
#include <SoftwareSerial.h>

#define DI          15
#define DO           8
#define CAS          9
#define RAS         17
#define WE          16

#define XA0         18
#define XA1          2
#define XA2         19
#define XA3          6
#define XA4          5
#define XA5          4
#define XA6          7
#define XA7          3
#define XA8         14

#define M_TYPE      13
#define R_LED       12
#define G_LED       11

#define RXD          0
#define TXD          1

#define BUS_SIZE     9

/* ================================================================== */
volatile int bus_size;

int writeAverage = 0;
int readAverage = 0;

//SoftwareSerial USB(RXD, TXD);

const unsigned int a_bus[BUS_SIZE] = {
  XA0, XA1, XA2, XA3, XA4, XA5, XA6, XA7, XA8
};

void setBus(unsigned int a) {
  int i;
  for (i = 0; i < BUS_SIZE; i++) {
    digitalWrite(a_bus[i], a & 1);
    a /= 2;
  }
}

void writeAddress(unsigned int r, unsigned int c, int v) {
  unsigned long startWrite = micros();
  /* row */
  setBus(r);
  digitalWrite(RAS, LOW);

  /* rw */
  digitalWrite(WE, LOW);

  /* val */
  digitalWrite(DI, (v & 1)? HIGH : LOW);

  /* col */
  setBus(c);
  digitalWrite(CAS, LOW);

  unsigned int long currentTime = micros() - startWrite;

  digitalWrite(WE, HIGH);
  digitalWrite(CAS, HIGH);
  digitalWrite(RAS, HIGH);
  
  if (currentTime > 10000) {
    currentTime = writeAverage;
  }
  writeAverage = writeAverage == 0 ? currentTime : ((writeAverage + currentTime) / 2);
}

int readAddress(unsigned int r, unsigned int c) {
  unsigned long startRead = micros();
  int ret = 0;

  /* row */
  setBus(r);
  digitalWrite(RAS, LOW);

  /* col */
  setBus(c);
  digitalWrite(CAS, LOW);

  /* get current value */
  ret = digitalRead(DO);

  unsigned int long currentTime = micros() - startRead;

  digitalWrite(CAS, HIGH);
  digitalWrite(RAS, HIGH);
  
  if (currentTime > 10000) {
    currentTime = readAverage;
  }
  readAverage = readAverage == 0 ? currentTime : ((readAverage + currentTime) / 2);

  return ret;
}

void error(int r, int c)
{
  unsigned long a = ((unsigned long)c << bus_size) + r;
  digitalWrite(R_LED, LOW);
  digitalWrite(G_LED, HIGH);
  interrupts();
  Serial.print(" FAILED $");
  Serial.println(a, HEX);
  printStats();
  Serial.flush();
  while (1)
    ;
}

void ok(void)
{
  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, LOW);
  interrupts();
  Serial.println(" DRAM Memory Passed ");
  printStats();
  Serial.flush();
  while (1)
    ;
}

void green(int v) {
  digitalWrite(G_LED, v);
}

void fill(int v) {
  int r, c, g = 0;
  v &= 1;
  for (c = 0; c < (1<<bus_size); c++) {
    green(g? HIGH : LOW);
    for (r = 0; r < (1<<bus_size); r++) {
      writeAddress(r, c, v);
      if (v != readAddress(r, c))
        error(r, c);
    }
    g ^= 1;
  }
}

void fillx(int v) {
  int r, c, g = 0;
  v &= 1;
  for (c = 0; c < (1<<bus_size); c++) {
    green(g? HIGH : LOW);
    for (r = 0; r < (1<<bus_size); r++) {
      writeAddress(r, c, v);
      if (v != readAddress(r, c))
        error(r, c);
      v ^= 1;
    }
    g ^= 1;
  }
}

void setup() {
  int i;

  delay(500);
  Serial.begin(9600);
  while (!Serial)
    ; /* wait */

  Serial.println();
  Serial.print("DRAM TESTER Rev1: ");

  for (i = 0; i < BUS_SIZE; i++)
    pinMode(a_bus[i], OUTPUT);

  pinMode(CAS, OUTPUT);
  pinMode(RAS, OUTPUT);
  pinMode(WE, OUTPUT);
  pinMode(DI, OUTPUT);

  pinMode(R_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);

  pinMode(M_TYPE, INPUT);
  pinMode(DO, INPUT);

  digitalWrite(WE, HIGH);
  digitalWrite(RAS, HIGH);
  digitalWrite(CAS, HIGH);

  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, HIGH);

  if (digitalRead(M_TYPE)) {
    /* jumper not set - 41256 */
    bus_size = BUS_SIZE;
    Serial.println("256Kx1 ");
  } else {
    /* jumper set - 4164 */
    bus_size = BUS_SIZE - 1;
    Serial.println("64Kx1 ");
  }
  Serial.flush();

  digitalWrite(R_LED, LOW);
  digitalWrite(G_LED, LOW);

  noInterrupts();
  for (i = 0; i < (1 << BUS_SIZE); i++) {
    digitalWrite(RAS, LOW);
    digitalWrite(RAS, HIGH);
  }
  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, HIGH);
}

void printStats() {
  Serial.println((String)"Columns — Read AVG: " + readAverage + " micros / Write AVG: "+writeAverage+" micros");
}

void loop() {
  unsigned int long dramTimerStart = micros();
  interrupts(); Serial.println("Test: Write \"0\" & Verify Read"); Serial.flush(); noInterrupts(); fillx(0);
  interrupts(); Serial.println("Test: Write \"1\" & Verify Read"); Serial.flush(); noInterrupts(); fillx(1);
  interrupts(); Serial.println("Test: Write \"0\" then \"1\" and Verify Read"); Serial.flush(); noInterrupts(); fill(0);
  interrupts(); Serial.println("Test: Write \"1\" then \"0\" and Verify Read"); Serial.flush(); noInterrupts(); fill(1);
  unsigned int long dramTimerStop = micros();
  unsigned int long dramTimerDuration = dramTimerStop - dramTimerStart;
  interrupts(); Serial.println((String)"Total Test Duration: " + (dramTimerDuration / 1000) + " ms"); Serial.flush(); noInterrupts();
  ok();
}
