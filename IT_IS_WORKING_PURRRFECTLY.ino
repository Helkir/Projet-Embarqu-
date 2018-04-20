
/*   Init Son*/
#include <SD.h>                      // need to include the SD library
//#define SD_ChipSelectPin 53  //example uses hardware SS pin 53 on Mega2560
#define SD_ChipSelectPin 4  //using digital pin 4 on arduino nano 328, can use other pins
#include <TMRpcm.h>           //  also need to include this library...
#include <SPI.h>

TMRpcm tmrpcm;   // create an object for use in this sketch

unsigned long time = 0;
/* Fin init son */


/* init émission son-/ */
int Proxy = 0; // detection de proximité
int Proxy_t1 = 0;  // detection de proxi à t-1
int Temps = 0; // servira a compter le nombre de boucles
int Touch = 0; // detection de toucher
int Touch_t1 = 0;  // detection de toucher à t-1
/* init émission son-/ */


#include "mpr121.h"
#include <Wire.h>

#define SENSORS       13
#define TOU_THRESH0    0x2f   // sets the touch sensor threshold default value 0x1F
#define REL_THRESH0    0x1F   // sets the touch sensor threshold  default value 0x1A
#define TOU_THRESH1    0x2f   // sets the touch sensor threshold default value 0x1F
#define REL_THRESH1    0x1F   // sets the touch sensor threshold  default value 0x1A
#define TOU_THRESH2    0x2f   // sets the touch sensor threshold default value 0x1F
#define REL_THRESH2    0x1F   // sets the touch sensor threshold  default value 0x1A
#define TOU_THRESH3    0x2f   // sets the touch sensor threshold default value 0x1F
#define REL_THRESH3    0x1F   // sets the touch sensor threshold  default value 0x1A

#define TOU_THRESH     0x1f   // sets the touch sensor threshold default value 0x1F
#define REL_THRESH     0x0F   // sets the touch sensor threshold  default value 0x1A


#define PROX_THRESH   0x12  // sets the proximity sensor threshold  0x3f
#define PREL_THRESH   0x0C   // sets the proximity sensor release    0x3c

// variables: capacitive sensing
bool touchStates[SENSORS];    // holds the current touch/prox state of all sensors
bool activeSensors[SENSORS] = {1,1,1,1,1,1,1,1,1,1,1,1,1}; // holds which sensors are active (0=inactive, 1=active)
bool newData = false;         // flag that is set to true when new data is available from capacitive sensor
int irqpin = 2;               // pin that connects to notifies when data is available from capacitive sensor


void setup(){
/* Setup Son*/
 tmrpcm.speakerPin = 9; //5,6,11 or 46 on Mega, 9 on Uno, Nano, etc
  //Complimentary Output or Dual Speakers:
  //pinMode(10,OUTPUT); Pin pairs: 9,10 Mega: 5-2,6-7,11-12,46-45 

   pinMode(13,OUTPUT); //LED Connected to analog pin 0
  if (!SD.begin(SD_ChipSelectPin)) {  // see if the card is present and can be initialized:
    Serial.println("SD fail");  
    return;   // don't do anything more if not

  }
  else{   
    Serial.println("SD ok");   
  }
//  tmrpcm.play("music.wav"); //the sound file "music" will play each time the arduino powers up, or is reset
/* End setup son*/

/* Setup MPR121*/
  // attach interrupt to pin - interrupt 1 is on pin 2 of the arduino (confusing I know)
  pinMode(irqpin,INPUT_PULLUP);
  attachInterrupt(0, dataAvailable, FALLING);

  // set-up the Serial and I2C/Wire connections
  Serial.begin(9600);
  Wire.begin();

  // set the registers on the capacitive sensing IC
  setupCapacitiveRegisters();
/* End Setup MPR121*/

}

void loop()
{
 
  readCapacitiveSensor();
  Wire.requestFrom(0x5A,0x2B); 
  byte Reg_Stat[0x2A];
   for ( int i =0; i <= 0x2A ; i++)
   {
    Reg_Stat[i] = Wire.read();
   }

 
  Serial.print(" ELE0 Base/Filtered: ");
  Serial.print( Reg_Stat[0x1E]);
  Serial.print("/");
  Serial.print( Reg_Stat[0x04]);
  Serial.print(" ELE1 Base/Filtered: ");
  Serial.print( Reg_Stat[0x1F]);
  Serial.print("/");
  Serial.println( Reg_Stat[0x06]);
  Serial.print(" Touched LSB : ");
  Serial.println( Reg_Stat[0x00]);
  Serial.print(" Touched MSB : ");
  Serial.println( Reg_Stat[0x01]);
  Serial.println ( Temps );
  delay(200);

/*   Lancement musique */

Proxy_t1 = Proxy;       // sauvegarde état précedent > permet de ne pas boucler inutilement apres avoir jouer une musique
Proxy =  Reg_Stat[0x01];
Proxy = Proxy & 0x10;    // Proxy vaut 0x10 quand il y a une détection
Touch_t1 = Touch;        // Sauvegarde état précedent
Touch = Reg_Stat[0x00];  // Sauvegarde état précedent




  if ( Proxy != 0)     /* si proxy differe de 0 alors on detecte une proximité */
            {
  Temps++;         /* incrémente temps pour pouvoir savoir si l'approche est rapide ou lente */
  if (Touch != Touch_t1 && Touch != 0)     /* si Touch differe de 0 alors on detecte un toucher */
   {
    if (Temps > 5 ) 
          { 
          tmrpcm.play("music.wav");                   /* joue ronron car approche lente */
          }
    else if (Temps < 5 )
        {
          tmrpcm.play("music2.wav");     /* joue ouille car approche rapide */
        } 

          Temps = 0;
    }
           
  }
  if (Proxy != Proxy_t1)
    Temps = 0;
} // fin loop

/**
 * dataAvailable Callback method that runs whenever new data becomes available on from the capacitive sensor. 
 *   This method was attached to the interrupt on pin 2, and is called whenever that pins goes low.
 */
void dataAvailable() {
  newData = true;
 
}

/**
 * readCapacitiveSensor Reads the capacitive sensor values from the MP121 IC. It makes a request to
 *   the sensor chip via the I2C/Wire connection, and then parses the sensor values which are stored on
 *   the first 13 bits of the 16-bit response msg.
 */
void readCapacitiveSensor(){

  if(newData || !digitalRead(irqpin)){    
    //read the touch state from the MPR121
    Wire.requestFrom(0x5A,2); 
    byte tLSB = Wire.read();
    byte tMSB = Wire.read();
    uint16_t touched = ((tMSB << 8) | tLSB); //16bits that make up the touch states
    
    for (int i = 0; i < SENSORS; i++){  // Check what electrodes were pressed
      if (activeSensors[i] == 0) continue;
      char sensor_id [] = {'\0','\0','\0'};
      switch (i) {
        case 12:
          sensor_id[0] = 'P';
          break;
        default:
          if (i < 10) {
            sensor_id[0] = char( i+48 );
          } 
          else if (i < 12) {
          sensor_id[0] = char('1');
          sensor_id[1] = char( ( i % 10 ) + 48 );
          }
      }
      if (sensor_id != '\0') {
        // read the humidity level

        // if current sensor was touched (check appropriate bit on touched var)
        if(touched & (1<<i)){      
          // if current pin was not previously touched send a serial message
        
          if(touchStates[i] == 0){          
            Serial.print("Sensor number ");
            Serial.print(sensor_id);        
            Serial.print(":");
            Serial.println(" Proximity detected");
            Serial.println(" Play Music");
          } 
          touchStates[i] = 1;      
        } else {
          // if current pin was just touched send serial message
          if(touchStates[i] == 1){
            Serial.print("Sensor number ");
            Serial.print(sensor_id);
            Serial.print(":");
            Serial.println(" No proximity detected");
          
          }
          touchStates[i] = 0;
        }        
      }
    }
    newData = false;
  }
}

/**
 * setupCapacitiveRegisters Updates all of configurations on the MP121 capacitive sensing IC. This includes
 *   setting levels for all filters, touch and proximity sensing activation and release thresholds, debounce,
 *   and auto-configurations options. At the end it activates all of the electrodes.
 */
void setupCapacitiveRegisters(){

  set_register(0x5A, ELE_CFG, 0x00); 
  
  // Section A - filtering when data is > baseline.
    // touch sensing
    set_register(0x5A, MHD_R, 0x01);
    set_register(0x5A, NHD_R, 0x01);
    set_register(0x5A, NCL_R, 0x00);
    set_register(0x5A, FDL_R, 0x00);

    // prox sensing 
    set_register(0x5A, PROX_MHDR, 0xFF);
    set_register(0x5A, PROX_NHDAR, 0xFF);
    set_register(0x5A, PROX_NCLR, 0x00);
    set_register(0x5A, PROX_FDLR, 0x00);

  // Section B - filtering when data is < baseline.
    // touch sensing
    set_register(0x5A, MHD_F, 0x01);
    set_register(0x5A, NHD_F, 0x01);
    set_register(0x5A, NCL_F, 0xFF);
    set_register(0x5A, FDL_F, 0x02);
  
    // prox sensing
    set_register(0x5A, PROX_MHDF, 0x01);
    set_register(0x5A, PROX_NHDAF, 0x01);
    set_register(0x5A, PROX_NCLF, 0xFF);
    set_register(0x5A, PROX_NDLF, 0xFF);

  // Section C - Sets touch and release thresholds for each electrode
    set_register(0x5A, ELE0_T, TOU_THRESH0);
    set_register(0x5A, ELE0_R, REL_THRESH0);
   
    set_register(0x5A, ELE1_T, TOU_THRESH1);
    set_register(0x5A, ELE1_R, REL_THRESH1);
    
    set_register(0x5A, ELE2_T, TOU_THRESH2);
    set_register(0x5A, ELE2_R, REL_THRESH2);
    
    set_register(0x5A, ELE3_T, TOU_THRESH3);
    set_register(0x5A, ELE3_R, REL_THRESH3);
    
    set_register(0x5A, ELE4_T, TOU_THRESH);
    set_register(0x5A, ELE4_R, REL_THRESH);
    
    set_register(0x5A, ELE5_T, TOU_THRESH);
    set_register(0x5A, ELE5_R, REL_THRESH);
    
    set_register(0x5A, ELE6_T, TOU_THRESH);
    set_register(0x5A, ELE6_R, REL_THRESH);
    
    set_register(0x5A, ELE7_T, TOU_THRESH);
    set_register(0x5A, ELE7_R, REL_THRESH);
    
    set_register(0x5A, ELE8_T, TOU_THRESH);
    set_register(0x5A, ELE8_R, REL_THRESH);
    
    set_register(0x5A, ELE9_T, TOU_THRESH);
    set_register(0x5A, ELE9_R, REL_THRESH);
    
    set_register(0x5A, ELE10_T, TOU_THRESH);
    set_register(0x5A, ELE10_R, REL_THRESH);
    
    set_register(0x5A, ELE11_T, TOU_THRESH);
    set_register(0x5A, ELE11_R, REL_THRESH);

  // Section D - Set the touch filter Configuration
    set_register(0x5A, FIL_CFG, 0x04);  

  // Section E - Set proximity sensing threshold and release
    set_register(0x5A, PRO_T, PROX_THRESH);   // sets the proximity sensor threshold
    set_register(0x5A, PRO_R, PREL_THRESH);   // sets the proximity sensor release

  // Section F - Set proximity sensor debounce
    set_register(0x59, PROX_DEB, 0x50);  // PROX debounce

  // Section G - Set Auto Config and Auto Reconfig for prox sensing
    set_register(0x5A, ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   
    set_register(0x5A, ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
    set_register(0x5A, ATO_CFGT, 0xB5);  // Target = 0.9*USL = 0xB5 @3.3V
    set_register(0x5A, ATO_CFG0, 0x0B);

  // Section H - Start listening to all electrodes and the proximity sensor for ELE0-1
    set_register(0x5A, ELE_CFG, 0x2C);  // default value 0x3C Start listening to all electrodes and the proximity sensor
}

/**
 * set_register Sets a register on a device connected via I2C. It accepts the device's address, 
 *   register location, and the register value.
 * @param address The address of the I2C device
 * @param r       The register's address on the I2C device
 * @param v       The new value for the register
 */
void set_register(int address, unsigned char r, unsigned char v){
  Wire.beginTransmission(address);
  Wire.write(r);
  Wire.write(v);
  Wire.endTransmission();
}
