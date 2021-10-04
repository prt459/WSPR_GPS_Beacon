//
// A simple mono-band WSPR beacon using an si5351 breakout for clock synthesis and a Ublox 6M for time sync. 
// Paul Taylor  VK3HN   
//
// For a Blog post write-up see: https://vk3hn.wordpress.com/2021/10/04/20-meters-200mw-12000-miles-wspr-magic/
//
// The WSPR code is copied from Software for Zachtek "WSPR-TX_LP1 Version 1"
//    https://github.com/HarrydeBug/1011-WSPR-TX_LP1 
//
//  4 Oct 2021  Version 1.0  Initial working version.    
//

#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <si5351.h>
#include <JTEncode.h>   //JTEncode  by NT7S https://github.com/etherkit/JTEncode

#define WSPR_FREQ_2M       14449000000ULL   //2m    144.490,000MHz 
#define WSPR_FREQ_2M_STR   "144,490,000" 
 
#define WSPR_FREQ_6M        5029450000ULL   //6m     50.294,500MHz 
#define WSPR_FREQ_6M_STR    "50,294,500" 

#define WSPR_FREQ_10M       2812610000ULL   //10m    28.126,100MHz
#define WSPR_FREQ_10M_STR   "28,126,100" 

#define WSPR_FREQ_12M       2492610000ULL   //12m    24.926,100MHz 
#define WSPR_FREQ_12M_STR   "24,926,100" 

#define WSPR_FREQ_15M       2109610000ULL   //15m    21.096.100MHz  
#define WSPR_FREQ_15M_STR   "21,096,100" 

#define WSPR_FREQ_17M       1810610000ULL   //17m    18.106,100MHz
#define WSPR_FREQ_17M_STR   "18,106,100" 

#define WSPR_FREQ_20M       1409710000ULL   //20m    14.097,100MHz 
#define WSPR_FREQ_20M_STR   "14,097,100" 

#define WSPR_FREQ_30M       1014020000ULL   //30m    10.140,200MHz  
#define WSPR_FREQ_30M_STR   "10,140,200" 

#define WSPR_FREQ_40M       704010000ULL   //40m     7.040,100MHz 
#define WSPR_FREQ_40M_STR   "7,040,100" 

#define WSPR_FREQ_80M       357010000ULL   //80m     3.570,100MHz
#define WSPR_FREQ_80M_STR   "3,570,100" 

#define WSPR_FREQ_160M      183810000ULL   //160m    1.838,100MHz
#define WSPR_FREQ_160M_STR   "1,838,100" 

#define WSPR_FREQ_630M       47570000ULL   //630m      475.700kHz
#define WSPR_FREQ_630M_STR   "475,700" 

#define WSPR_FREQ_2190M      13750000ULL   //2190m     137.500kHz
#define WSPR_FREQ_2190M_STR   "137,500" 
//---------------------------------------------------------------------
// Set the band here!  Set BOTH and make sure they are for the same band!
uint64_t freq;   // frequency variable  
#define WSPR_FREQ        WSPR_FREQ_20M      // set the band here 
#define WSPR_FREQ_STR    WSPR_FREQ_20M_STR  // set the band display string here 
//---------------------------------------------------------------------
// transmit windows that the beacon will use, as a percentage (0 == never transmit, 100 == always transmit)
#define TX_FRACTION 75   

//---------------------------------------------------------------------

#define GPS_RX_PIN 2  // receive serial data pin on the ublox GPS module
#define GPS_TX_PIN 3  // transmit serial data pin on the ublox GPS module

enum E_LocatorOption {
  Manual,
  GPS
};

struct S_WSPRData
{
  char CallSign[7];                //Radio amateur Call Sign, zero terminated string can be four to six char in length + zero termination
  E_LocatorOption LocatorOption;   //If transmitted Maidenhead locator is based of GPS location or if it is using MaidneHead4 variable.
  char MaidenHead4[5];             //Maidenhead locator, must be 4 chars and a zero termination
  uint8_t TXPowerdBm;              //Power data in dBm min=0 max=60
};

S_WSPRData WSPRData;  // data structure for the WSPRPS data block

uint8_t tx_buffer[171];

byte GPSM; //GPS Minutes
byte GPSS; //GPS Seconds

LiquidCrystal lcd(8,9,10,11,12,13);

SoftwareSerial mySerial(GPS_RX_PIN, GPS_TX_PIN);  // create a Software Serial object

#define NMEA_BUFF_LEN 16
char nmea_buff[NMEA_BUFF_LEN];
char c = '0'; 
byte i, j;
bool GPS_locked = false;
boolean TXEnabled = true;  // can use this to inhibit transmit, for testing


Si5351 si5351;             // I2C address defaults to x60 in the NT7S lib

JTEncode jtencode;         // NT7S WSPR encoder lib


//Create a random seed by doing CRC32 on 100 analog values from port A0
unsigned long RandomSeed(void) {

  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  uint8_t ByteVal;
  unsigned long crc = ~0L;

  for (int index = 0 ; index < 100 ; ++index) {
    ByteVal = analogRead(A0);
    crc = crc_table[(crc ^ ByteVal) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (ByteVal >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}


void set_tx_buffer()
{
  // Clear out the transmit buffer
  memset(tx_buffer, 0, sizeof(tx_buffer));
  jtencode.wspr_encode(WSPRData.CallSign, WSPRData.MaidenHead4, WSPRData.TXPowerdBm, tx_buffer);
}



void SendWSPRBlock()
{
  // Loop through the string, transmitting one character at a time
  uint8_t i;
  unsigned long startmillis;
  unsigned long endmillis;

//  Serial.println("TX");
  si5351.output_enable(SI5351_CLK0, 1);  // turn the VFO on

  // Send WSPR for two minutes
  startmillis = millis();
  for (i = 0; i < 162; i++)  //162 WSPR symbols to transmit
  {
    endmillis = startmillis + ((i + 1) * (unsigned long) 683) ;   // Delay value for WSPR delay is 683 milliseconds
    uint64_t tonefreq;
    tonefreq = freq + ((tx_buffer[i] * 146));  //~1.46 Hz  Tone spacing in centiHz
    if (TXEnabled) 
    {
      si5351.set_freq(tonefreq, SI5351_CLK0);
    };

    Serial.print(i);     Serial.print(" "); Serial.println((unsigned long)tonefreq);  
//    if((i>0) && (i%20 == 0)) Serial.println(); 

    // wait until tone is transmitted for the correct amount of time
    while (millis() < endmillis){
      //Do nothing, just wait
    };

    byte k=(162-i);
    lcd.setCursor(13,1);
    if(k < 100) lcd.print(" ");
    if(k < 10)  lcd.print(" ");
    lcd.print(k-1);      
    
//    if((i)%20 == 0) Serial.print("#");  // transmit progress bar
  };
  
  // Switch off Si5351a output
  si5351.output_enable(SI5351_CLK0, 0);  // turn the VFO off
  Serial.println("/TX");
};



void Send(){
  if(!GPS_locked) return;
//   Serial.println ("Wait even minute");
  if ((GPSS == 0) && ((GPSM % 2) == 0))   //If second is zero at even minute then start WSPR transmission
  {
    if(random(0, 101) >= (TX_FRACTION - 1) )
    {
      // skip this slot, wait for the next one
      Serial.println("  Skipping slot!");
      lcd.setCursor(11,0);
      lcd.print("Skip!");
    }
    else
    {
      // start a WSPR transmission
      Serial.println ("start WSPR");
      
      // hard code the band -- this label is set at the top of the script
      freq = WSPR_FREQ;
      freq = freq + (100ULL * random (-100, 100)); //modify TX frequency with a random value beween -100 and +100 Hz
      Serial.print("f="); 
      Serial.println((unsigned long)(freq/100));

      // overwrite hertz 3 digits of the transmit frequency on the LCD with actual value
      byte hz = 100 - abs((int)((WSPR_FREQ - freq)/100ULL));
//      Serial.print(" ="); Serial.println(hz);
      
      lcd.setCursor(7,1);   
      if(hz < 100) lcd.print('0');
      if(hz < 10)  lcd.print('0');
      lcd.print(hz);

      set_tx_buffer();    // Encode the message in the transmit buffer
                          // Not sure why this could not be done in setup()
  
      delay(980);  // Transmissions nominally start one second into an even UTC minute: e.g., at hh:00:01, hh:02:01, etc.

      lcd.setCursor(8,0);
      lcd.print(" WSPR-Tx");
      lcd.setCursor(8,1);

      SendWSPRBlock ();
//      if (LastFreq ())  {
//          smartdelay(GadgetData.TXPause * 1000UL);   // Pause for some time to give a duty cycle on the transmit. 2000=100%, 20000=50%, 130000=33%
//        }
//        NextFreq();

      freq = freq + (100ULL * random (-100, 100));    // modify TX frequency with a random value beween -100 and +100 Hz

//      Serial.println("delay 10s");
      lcd.setCursor(10,1);
      lcd.print("wait>");
      for (byte n=9; n>0; n--){
        lcd.setCursor(15,1);
        lcd.print(n);
         Serial.println(n);
        delay(1000);  // to get us clear of the next even minute     
      };
      
      lcd.clear();

//      Serial.println (F("Wait..."));

    }  // else start a WSPR transmission 
  } // even minute and zero second
} // Send()



void setup()
{
  Serial.begin(9600); 
  while (!Serial) ; // wait for serial port to connect, needed for native USB port only
  
  mySerial.begin(9600);

  lcd.begin(16,2);
  lcd.clear(); 
  lcd.setCursor(0,0); 
  lcd.print("GPS WSPRv1.0    ");
  delay(500);

  lcd.setCursor(0,1); 
  lcd.print("Arduino/ Ublox6M");
  delay(1000);
  lcd.clear(); 

  pinMode(GPS_RX_PIN, INPUT);
  pinMode(GPS_TX_PIN, OUTPUT);
  random(RandomSeed());

  // set up WSPR transmit data
  WSPRData.CallSign[0] = 'V';    
  WSPRData.CallSign[1] = 'K';    
  WSPRData.CallSign[2] = '3';    
  WSPRData.CallSign[3] = 'H';    
  WSPRData.CallSign[4] = 'N';    
  WSPRData.CallSign[5] = 0;    
  WSPRData.CallSign[6] = 0;    
  WSPRData.LocatorOption = GPS; 
  WSPRData.MaidenHead4[0] = 'Q';          //Maidenhead locator, must be 4 chars and a zero termination
  WSPRData.MaidenHead4[1] = 'F';             
  WSPRData.MaidenHead4[2] = '2';             
  WSPRData.MaidenHead4[3] = '2';             
  WSPRData.MaidenHead4[4] = 0;             
  WSPRData.TXPowerdBm = 23;              // 200mW   Power data in dBm min=0 max=60, set to 20dBm 100mW

  // set up the si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);        // (0 is the default xtal freq of 25Mhz)
  si5351.set_correction(19100);                      // trim the onboard oscillator
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
}



void loop()
{
  i=0; 
  j=0; 
  c = '0'; 
  for(j=0; j<NMEA_BUFF_LEN; j++) nmea_buff[j]='\n';

  while(c != '\n') {
    if (mySerial.available()) {  // check SoftwareSerial port
      c = mySerial.read();       // read the next serial char from the GPS
      if(i<(NMEA_BUFF_LEN-1)) nmea_buff[i++] = c;  // append to NMEA buffer
//    if((c != '\n')) Serial.print(c);
    }    
  }

// we have a NMEA message, see what it is...
   if(nmea_buff[3] == 'R' &&
      nmea_buff[4] == 'M' &&
      nmea_buff[5] == 'C')
     {        
     j=0;
     while(nmea_buff[j]!='\n') Serial.print(nmea_buff[j++]);
     Serial.println();
//     Serial.println("]");

     if(nmea_buff[7] != ',') 
       GPS_locked = true;   // have decoded a timestamp 
     else 
       GPS_locked = false;  // havent decoded a timestamp yet
     
     lcd.setCursor(0,0);

     if(GPS_locked){
       lcd.setCursor(0,0);
//       for(j=7; j<13; j++) lcd.print(nmea_buff[j]);
       lcd.print(nmea_buff[7]);
       lcd.print(nmea_buff[8]);
       lcd.print(':');
       lcd.print(nmea_buff[9]);
       lcd.print(nmea_buff[10]);
       lcd.print(':');
       lcd.print(nmea_buff[11]);
       lcd.print(nmea_buff[12]);
       lcd.print("Z       "); 
     
       String tokn(nmea_buff[11]);
       tokn.concat(nmea_buff[12]);
       byte sec = (byte)tokn.toInt(); 
       
       tokn="";
       tokn.concat(nmea_buff[9]); 
       tokn.concat(nmea_buff[10]); 
       byte mn = (byte)tokn.toInt(); 

       GPSS = sec;
       GPSM = mn;
     }
     else
       // No GPS timestamp decode yet...
       lcd.print("GPS scan");

     lcd.setCursor(0,1);
     lcd.print(WSPR_FREQ_STR);      

     // see if it's time to WSPR...
     Send();

  }
}
