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
#define XA8         A6

#define M_TYPE      13
#define R_LED       12
#define G_LED       11

#define RXD          0
#define TXD          1

#define BUS_SIZE     9

/* ================================================================== */
volatile int bus_size;

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

  digitalWrite(WE, HIGH);
  digitalWrite(CAS, HIGH);
  digitalWrite(RAS, HIGH);
}

int readAddress(unsigned int r, unsigned int c) {
  int ret = 0;

  /* row */
  setBus(r);
  digitalWrite(RAS, LOW);

  /* col */
  setBus(c);
  digitalWrite(CAS, LOW);

  /* get current value */
  ret = digitalRead(DO);

  digitalWrite(CAS, HIGH);
  digitalWrite(RAS, HIGH);

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
  Serial.flush();
  while (1)
    ;
}

void ok(void)
{
  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, LOW);
  interrupts();
  Serial.println(" OK!");
  Serial.flush();
  while (1)
    ;
}

void blink(void)
{
  digitalWrite(G_LED, LOW);
  digitalWrite(R_LED, LOW);
  delay(1000);
  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, HIGH);
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
  blink();
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
  blink();
}

void setup() {
  int i;

  Serial.begin(9600);
  while (!Serial)
    ; /* wait */

  Serial.println();
  Serial.print("DRAM TESTER ");

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
    Serial.print("256Kx1 ");
  } else {
    /* jumper set - 4164 */
    bus_size = BUS_SIZE - 1;
    Serial.print("64Kx1 ");
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

void loop() {
  interrupts(); Serial.print("."); Serial.flush(); noInterrupts(); fillx(0);
  interrupts(); Serial.print("."); Serial.flush(); noInterrupts(); fillx(1);
  interrupts(); Serial.print("."); Serial.flush(); noInterrupts(); fill(0);
  interrupts(); Serial.print("."); Serial.flush(); noInterrupts(); fill(1);
  ok();
}