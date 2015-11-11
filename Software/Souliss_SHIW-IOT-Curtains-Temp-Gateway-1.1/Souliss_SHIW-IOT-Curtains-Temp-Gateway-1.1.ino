/**************************************************************************
    Smart Home In Wall IOT - SHIW-IOT 1.1

    Souliss 7.1.0
    
    Use with SHIW-IOT boards 1.1 
    Logic Board: PCB-Logic-1.1
    Power Relays Board:PCB-PWR-2R-35-1.1 / PCB-PWR-2R-70-1.1

    This board can drive motorised curtains with local push buttons
    Includes Temperature / Humidity sensor HIH8120-021-001S

    Schematic uses ESP-07 or EPS-12 pinouts, on this program ESP-12E have been
    used, GPIO4 and GPIO5 are swapped on ESP-12E, take into consideration
    at pin declaration.
    
    Load this code on ESP8266 board using the porting of the Arduino core
    for this platform.

    Yoann Lefebvre 2015.
        
***************************************************************************/


//-----------------USER CONFGURATION--------------------------------------------------------------

// Comment Souliss gateway settings in void setup() if souliss gateway is present

// **** Define the WiFi name and password ****
#define WIFICONF_INSKETCH
#define WiFi_SSID               "Your-WiFi-SSID"
#define WiFi_Password           "Your-WiFi-WPA-Key"  

//-----------------END OF USER CONFGURATION-------------------------------------------------------


// Configure the framework
#include "bconf/MCU_ESP8266.h"              // Load the code directly on the ESP8266
#include "conf/Gateway.h"                   // The main node is the Gateway, we have just one node
#include "conf/IPBroadcast.h"

// Include framework code and libraries
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "Souliss.h"

//I2C
#include <Wire.h>

// This identify the number of the logic
#define CURTAIN0          0   //T22     1 slot
#define TEMPERATURE       1   //T52 T°  2 slots: 1 & 2
#define HUMIDITY          4   //T53 H   2 slots: 4 & 5

// Define delay to close or open
//#define Souliss_T2n_Timer_Val 10

// **** Define here the right pin for your ESP module ****
// OUT1/R1 for openning motor
// OUT2/R2 for cosing motor


//--------------------ESP Pinout version------------------------------
//ESP-07 or ESP-12
//#define  OS1		4   //SET relay 1 (LOW = Coil enable) = out1 active
//#define  OR1    5   //RESET relay 1 (LOW = Coil enable)

//ESP-12E alternativ: pin swap
#define  OS1    5   //SET relay 1 (LOW = Coil enable) = out1 active
#define  OR1    4   //RESET relay 1 (LOW = Coil enable)
//--------------------------------------------------------------------

#define  OS2   12   //SET relay 2 (LOW = Coil enable) = out2 active
#define  OR2   13   //RESET relay 2 (LOW = Coil enable)
#define  BUP   16   //Button UP
#define  BDW   14   //Button DOWN

//Temp / Humidity HIH8120-021-001S
#define  SDA    2
#define  SCL    0

byte fetch_humidity_temperature(unsigned int *p_Humidity, unsigned int *p_Temperature);

//Deadba,d T° & H
#define  DEADBAND   0.05

//Status of outputs
#define OUT_STOP 0
#define OUT_OPENING 1
#define OUT_CLOSING 2
#define OUT_OPEN 3
#define OUT_CLOSE 4

int outStatus = OUT_STOP;

void setup()
{   
    Initialize();

    // 2 coils Relays
    pinMode(OS1, OUTPUT);         // Use pin as output R1 Set
    pinMode(OR1, OUTPUT);         // Use pin as output R1 Reset
    pinMode(OS2, OUTPUT);         // Use pin as output R2 Set
    pinMode(OR2, OUTPUT);         // Use pin as output R2 Reset
    
    //Shuttdown relays outputs
    out1OFF();
    out2OFF();
       
    //command buttons Open and Close
    pinMode(BUP, INPUT);          // Use pin as intput
    pinMode(BDW, INPUT);          // Use pin as intput 

    Wire.begin(SDA, SCL);
    pinMode(SDA, OUTPUT);
    digitalWrite(4, HIGH);      // this turns on the HIH
    delay(5000);
    

    // Connect to the WiFi network and get an address from DHCP
    GetIPAddress();

    // This is the vNet address for this node, used to communicate with other
    // nodes in your Souliss network, based on IP set by DHCP Wifi network
    SetAddress(myvNet_dhcp, 0xFF00, 0x0000);


//-----------------GATEWAY CONFGURATION--------------------------------------------------------------
    //Comment this lock if a souliss gateway is present in network
    //Use for standalone testing
    
    // Set this node as gateway for SoulissApp
    SetAsGateway(myvNet_dhcp);
    // Register node TYPIALS in gateway
    SetAsPeerNode(myvNet_dhcp, 0);  //T22  slot
    SetAsPeerNode(myvNet_dhcp, 1);  //T52 T° 2 slots: 1 & 2
    SetAsPeerNode(myvNet_dhcp, 4);  //T53 H  2 slots: 4 & 5

//-----------------END OF GATEWAY CONFGURATION-------------------------------------------------------
    
    
    // define Typicals of this node
    Set_T22(CURTAIN0);      // Typical 22 : Motorized devices with limit switches and middle position
    Set_T52(TEMPERATURE);   // Typical 52 : Temperature measure (-20, +50) °C
    Set_T53(HUMIDITY);      // Typical 53 : Humidity measure (0, 100) %
  
    
}

void loop()
{   
    //Var T° & H
    byte _status;
    unsigned int H_dat, T_dat;
    float RH, T_C;

  
    // Here we start to play
    EXECUTEFAST() {                     
        UPDATEFAST();   

        // Process every 510ms the logic that control the curtain
        FAST_510ms() {
          
            // Update T° et H
            fetch_humidity_temperature(&H_dat, &T_dat);
            RH = (float) H_dat * 6.10e-3;
            T_C = (float) T_dat * 1.0072e-2 - 40.0;
            ImportAnalog(TEMPERATURE, &T_C);
            ImportAnalog(HUMIDITY, &RH);
          
            // Use OPEN and CLOSE BP Commands
            LowDigIn(BUP, Souliss_T2n_OpenCmd_Local, CURTAIN0);
            LowDigIn(BDW, Souliss_T2n_CloseCmd_Local, CURTAIN0);
            
            // Run the logic
            Logic_T22(CURTAIN0);
            Logic_T52(TEMPERATURE);
            Logic_T53(HUMIDITY);
            
            // Control the output relays, the stop is with both the relays not active.
            //T22 Output status:
            //1(hex) for CLOSING,
            //2(hex) for OPENING,
            //3(hex) for STOP.
            //8(hex) for CLOSE
            //10(hex for OPEN
            //
            //Status of board outputs
            //#define OUT_STOP 0
            //#define OUT_OPENING 1
            //#define OUT_CLOSING 2
            //#define OUT_OPEN 3
            //#define OUT_CLOSE 4
            
            //1(hex) for CLOSING
            if((mOutput(CURTAIN0) == 0x01) && (outStatus != OUT_CLOSING))
            {
              out1OFF();             
              out2ON();
              outStatus = OUT_CLOSING;
            }

            //2(hex) for OPENING
            if((mOutput(CURTAIN0) == 0x02) && (outStatus != OUT_OPENING))
            {
              out1ON();
              out2OFF();
              outStatus = OUT_OPENING;
            }

            //3(hex) for STOP
            if((mOutput(CURTAIN0) == 0x03) && (outStatus != OUT_STOP))
            {
              out1OFF();            
              out2OFF();
              outStatus = OUT_STOP;
            }
            
            //8(hex) for CLOSE
            if((mOutput(CURTAIN0) == 0x08) && (outStatus != OUT_CLOSE))
            {
              out1OFF();            
              out2OFF();
              outStatus = OUT_CLOSE;
            }
            
            //10(hex for OPEN
            if((mOutput(CURTAIN0) == 0x10) && (outStatus != OUT_OPEN))
            {
              out1OFF();            
              out2OFF();
              outStatus = OUT_OPEN;
            }
        } 

        // Process the communication
        FAST_PeerComms();
        // Here we handle here the communication with Gateway
        FAST_GatewayComms();  
    
        // Define the hold time of the outputs, by default the timer hold the relays for 16 times, so:
        // 221x10x16ms that is about 35 seconds. Change the parameter inside FAST_x10ms() to change this time.
        FAST_x10ms(50) {                 
            Timer_T22(CURTAIN0);
        }
    }
}

byte fetch_humidity_temperature(unsigned int *p_H_dat, unsigned int *p_T_dat)
{
      byte address, Hum_H, Hum_L, Temp_H, Temp_L, _status;
      unsigned int H_dat, T_dat;
      address = 0x27;;
      Wire.beginTransmission(address); 
      Wire.endTransmission();
      delay(100);
      
      Wire.requestFrom((int)address, (int) 4);
      Hum_H = Wire.read();
      Hum_L = Wire.read();
      Temp_H = Wire.read();
      Temp_L = Wire.read();
      Wire.endTransmission();
      
      _status = (Hum_H >> 6) & 0x03;
      Hum_H = Hum_H & 0x3f;
      H_dat = (((unsigned int)Hum_H) << 8) | Hum_L;
      T_dat = (((unsigned int)Temp_H) << 8) | Temp_L;
      T_dat = T_dat / 4;
      *p_H_dat = H_dat;
      *p_T_dat = T_dat;
      return(_status);
}

void out1ON()
{
  //enable R1 relay
  digitalWrite(OR1, LOW);
  digitalWrite(OS1, HIGH);
  delay(40);
  digitalWrite(OS1, LOW);
}

void out2ON()
{
  //enable R2 relay
  digitalWrite(OR2, LOW);
  digitalWrite(OS2, HIGH);
  delay(40);
  digitalWrite(OS2, LOW);
}

void out1OFF()
{
  //disable R1 relay
  digitalWrite(OS1, LOW);
  digitalWrite(OR1, HIGH);
  delay(40);
  digitalWrite(OR1, LOW);
}

void out2OFF()
{
  //disable R2 relay
  digitalWrite(OS2, LOW);
  digitalWrite(OR2, HIGH);
  delay(40);
  digitalWrite(OR2, LOW);
}

