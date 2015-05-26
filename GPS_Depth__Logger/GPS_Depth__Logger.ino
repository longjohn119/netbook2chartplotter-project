/* 
  A combiner-logger for NMEA GPS and depth finder outputs. It grabs the GPS $GPGGA, $GPRMC, and $GPGSA sentences 
  from the GPS module and the $SDDPT and $SDMTW sentences from the depth finder combines them and sends them all
  out the USB port to a computer running depth mapping software and sends the $GPGGA, $GPRMC, and $SDDPT sentences
  and logs them to a SD card. Note: Currently a memory card must be inserted for the unit to work.
  
  The check_gps and check_sdd functions written in large part by SlashDevin the author of the NeoGPS library 
  ( https://github.com/SlashDevin/NeoGPS )with some minor tweaks and additions by myself. Thanx /Dev!!
  
  Details of the hardware can be found on my blog http://netbook2chartplotter.blogspot.com/
  
  */

#include <SPI.h>
#include <SD.h>

#define fixPin 2

static void check_gps();
static void check_sdd();

char c;

const int chipSelect = 10;

File sd_file;

void setup() {
  // put your setup code here, to run once:
  Serial.begin (38400); //USB serial out
  Serial1.begin (9600); //GPS (gps)
  Serial2.begin (4800); //Sounder (sdd)

  Serial.print("Initializing SD card...");
  pinMode(SS, OUTPUT);

  if (!SD.begin(10, 11, 12, 13)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1) ;
  }
  Serial.println("card initialized.");

  // Open up the file we're going to log to!

  sd_file = SD.open("NMEA.LOG", FILE_WRITE);

  if (! sd_file) {
    Serial.println("error opening NMEA.LOG");
    // Wait forever since we cant write data
    while (1) ;
  }

  pinMode(fixPin, INPUT);      // Waits for the PPS GPS Fix signal from pin 3 uBlox Module 
  unsigned long int duration;   //   before moving on to the main program loop
  pulseIn(fixPin, HIGH, 300000000000);
  
}

void loop()
{
  check_gps();
  check_sdd();

  sd_file.flush();
}

//----------------

enum NMEA_state_t { WAITING_FOR_START, RECEIVING };

static NMEA_state_t gps_state, sdd_state;

static char start_of_gps[8];  // temporary place until first 6 chars received
static char start_of_sdd[8];

static char rmc[120] = "$GPRMC";
static char gga[120] = "$GPGGA";
static char gsa[120] = "$GPGSA";
static char dpt[120] = "$SDDPT";
static char mtw[120] = "$SDMTW";

static bool got_rmc;
static bool got_gga;
static bool got_gsa;
static bool got_dpt;
static bool got_mtw;

static char *gps_line;
static uint8_t gps_count;

static char  *sdd_line;
static uint8_t sdd_count;

static uint32_t last_rx = 0UL; //  to remember when we last received a char

//............

static void check_gps()
{
  bool got_something = false;

  while (Serial1.available()) {
    c = Serial1.read();
    got_something = true;

    switch (gps_state) {
      case WAITING_FOR_START:
        if (c == '$') {
          // Start of sentence!
          gps_count = 0;
          gps_line  = &start_of_gps[0];

          *gps_line++ = c;
          gps_count++;

          gps_state = RECEIVING;
        }
        break;

      case RECEIVING:
        if (c == '\r') {
          *gps_line++ = c;
          gps_count++;

        } else if (c == '\n') {
          *gps_line++ = c;
          gps_count++;
          *gps_line++ = 0; // NUL-terminate

          gps_state = WAITING_FOR_START; // get next sentence

        } else if (gps_count == 5) {
          // check the sentence type
          *gps_line++ = c;
          gps_count++;

          if (strncmp( start_of_gps, rmc, 6 ) == 0) {
            got_rmc  = true;
            gps_line = &rmc[6];
          } else if (strncmp( start_of_gps, gga, 6 ) == 0) {
            got_gga  = true;
            gps_line = &gga[6];
          } else if (strncmp( start_of_gps, gsa, 6 ) == 0) {
            got_gsa = true;
            gps_line = &gsa[6];
          } else
            gps_state = WAITING_FOR_START; // ignore the rest

        } else if (gps_count < sizeof(rmc) - 3) {
          // save chars if there's room
          *gps_line++ = c;
          gps_count++;
        }
        break;
    }
  }

  if (got_something)
    last_rx = millis();

  else if (millis() - last_rx > 10UL) {

    if (gps_state == WAITING_FOR_START) {

      // Quiet time, write the GPS sentences...

      if (got_rmc) {
        sd_file.write( rmc );
        Serial.write( rmc );
        got_rmc = false;
      }

      if (got_gga) {
        sd_file.write( gga );
        Serial.write( gga );
        got_gga = false;
      }

      if (got_gsa)  {
        //sd_file.write( gsa );
        Serial.write( gsa );
        got_gsa = false;
      }

      // ... and the SDDPT sentence, when/if it shows up during the quiet time.

      if (sdd_state == WAITING_FOR_START) {


        if (got_dpt) {
          sd_file.write( dpt );
          Serial.write( dpt );
          got_dpt = false;
        }

        if (got_mtw) {
          //sd_file.write( mtw );
          Serial.write( mtw );
          got_mtw = false;
        }
      }
    }
  }
}

//...............

//static char   *sdd_line;
//static uint8_t sdd_count;

static void check_sdd()
{
  bool got_something = false;

  while (Serial2.available()) {
    c = Serial2.read();
    got_something = true;

    switch (sdd_state) {
      case WAITING_FOR_START:
        if (c == '$') {
          // Start of sentence!
          sdd_count = 0;
          sdd_line  = &start_of_sdd[0];

          *sdd_line++ = c;
          sdd_count++;

          sdd_state = RECEIVING;
        }
        break;

      case RECEIVING:
        if (c == '\r') {
          *sdd_line++ = c;
          sdd_count++;

        } else if (c == '\n') {
          *sdd_line++ = c;
          sdd_count++;
          *sdd_line = 0; // NUL-terminate

          sdd_state = WAITING_FOR_START;

        } else if (sdd_count == 5) {
          // check the sentence type
          *sdd_line++ = c;
          sdd_count++;

          if (strncmp( start_of_sdd, dpt, 6 ) == 0) {
            got_dpt  = true;
            sdd_line = &dpt[6];
          } else if (strncmp( start_of_sdd, mtw, 6 ) == 0) {
            got_mtw = true;
            sdd_line = &mtw[6];
          } else
            sdd_state = WAITING_FOR_START; // ignore the rest

        } else if (sdd_count < sizeof(dpt) - 3) {
          // save chars if there's room
          *sdd_line++ = c;
          sdd_count++;
        }
        break;
    }
  }
}

