  //udp
  #include <WiFi.h>
  #include <WiFiMulti.h>
  #include <WiFiUdp.h>

  const char* udpAddress = "192.168.1.255";
  const int udpPort = 9999;

  int trans = 1;  //0 = serial, 1 = udp, 2 = bluetooth  Choose wich data format you want use.

  WiFiUDP udp;
  WiFiMulti wifiMulti;
  // fin udp

  #include <Wire.h>
  #include "BNO08x_AOG.h"
  #define CMPS14_ADDRESS 0x60    // Address of CMPS14 shifted right one bit for arduino wire library

  // BNO08x definitions
  #define REPORT_INTERVAL 95 //Report interval in ms (same as the delay at the bottom)
  
  #define CONST_180_DIVIDED_BY_PI 57.2957795130823

  //CMPS PGN - 211
  byte data[] = {0x80,0x81,0x7D,0xD3,8, 0, 0, 0, 0, 0, 0, 0, 0, 15};
  byte dataSize = sizeof(data);

  // booleans to see if we are using CMPS or BNO08x
  bool useCMPS = false;
  bool useBNO08x = false;

  // BNO08x address variables to check where it is
  const uint8_t bno08xAddresses[] = {0x4A,0x4B};
  const int16_t nrBNO08xAdresses = sizeof(bno08xAddresses)/sizeof(bno08xAddresses[0]);
  uint8_t bno08xAddress;
  BNO080 bno08x;

  float bno08xHeading = 0;
  double bno08xRoll = 0;
  double bno08xPitch = 0;

  int16_t bno08xHeading10x = 0;
  int16_t bno08xRoll10x = 0;
    
  void setup()
  { 
    //BTmod setup
    Serial.begin(115200);
    wifiMulti.addAP("ssid1", "password1");
    wifiMulti.addAP("ssid2", "password2");
    wifiMulti.addAP("ssid3", "password3");
    wifiMulti.addAP("ssid4", "password4");

  Serial.println("Connection au Wifi...");

  if (wifiMulti.run() == WL_CONNECTED)
    {
      Serial.println("");
      Serial.println("WiFi connecté");
      Serial.println("addresse IP : ");
      Serial.println(WiFi.localIP());
    }
    
  Wire.begin();
delay(5000);
  //test if CMPS working
  uint8_t error;
  Serial.println("Checking for CMPS14");
  Wire.beginTransmission(CMPS14_ADDRESS);
  error = Wire.endTransmission();

  if (error == 0)
    {
      Serial.println("Error = 0");
      Serial.print("CMPS14 ADDRESs: 0x");
      Serial.println(CMPS14_ADDRESS, HEX);
      Serial.println("CMPS14 Ok.");
      useCMPS = true;
    }
  else 
    {
      Serial.println("Error = 4");
      Serial.println("CMPS not Connected or Found");
    }

  if(!useCMPS)
    {
      for(int16_t i = 0; i < nrBNO08xAdresses; i++)
        {
          bno08xAddress = bno08xAddresses[i];
          
          Serial.print("\r\nChecking for BNO08X on ");
          Serial.println(bno08xAddress, HEX);
          Wire.beginTransmission(bno08xAddress);
          error = Wire.endTransmission();
      
          if (error == 0)
          {
            Serial.println("Error = 0");
            Serial.print("BNO08X ADDRESs: 0x");
            Serial.println(bno08xAddress, HEX);
            Serial.println("BNO08X Ok.");
            
            // Initialize BNO080 lib        
            if (bno08x.begin(bno08xAddress))
            {
              Wire.setClock(400000); //Increase I2C data rate to 400kHz
    
              // Use gameRotationVector
              bno08x.enableGameRotationVector(REPORT_INTERVAL); //Send data update every REPORT_INTERVAL in ms for BNO085
    
              // Retrieve the getFeatureResponse report to check if Rotation vector report is corectly enable
              if (bno08x.getFeatureResponseAvailable() == true)
              {
                if (bno08x.checkReportEnable(SENSOR_REPORTID_GAME_ROTATION_VECTOR, REPORT_INTERVAL) == false) bno08x.printGetFeatureResponse();
  
                // Break out of loop
                useBNO08x = true;
                break;
              }
              else 
              {
                Serial.println("BNO08x init fails!!");
              }
            }
            else
            {
              Serial.println("BNO080 not detected at given I2C address.");
            }
          }
          else 
          {
            Serial.println("Error = 4");
            Serial.println("BNO08X not Connected or Found");
          }
        }
    }
  }
  
  void loop()
  { 
     
    if(useCMPS)
      {
          Wire.beginTransmission(CMPS14_ADDRESS);  
          Wire.write(0x02);                     
          Wire.endTransmission();
      
          Wire.requestFrom(CMPS14_ADDRESS, 2); 
          while(Wire.available() < 2);       
    
          //the heading x10
          data[6] = Wire.read();
          data[5] = Wire.read();
     
          Wire.beginTransmission(CMPS14_ADDRESS);  
          Wire.write(0x1C);                    
          Wire.endTransmission();
     
          Wire.requestFrom(CMPS14_ADDRESS, 2);  
          while(Wire.available() < 2);        
    
          //the roll x10
          data[8] = Wire.read();
          data[7] = Wire.read();
      } 
    else if(useBNO08x)
      {
        if (bno08x.dataAvailable() == true)
          {
            bno08xHeading = (bno08x.getYaw()) * CONST_180_DIVIDED_BY_PI; // Convert yaw / heading to degrees
            bno08xHeading = -bno08xHeading; //BNO085 counter clockwise data to clockwise data
            
            if (bno08xHeading < 0 && bno08xHeading >= -180) //Scale BNO085 yaw from [-180°;180°] to [0;360°]
            {
              bno08xHeading = bno08xHeading + 360;
            }
                
            bno08xRoll = (bno08x.getRoll()) * CONST_180_DIVIDED_BY_PI; //Convert roll to degrees
            //bno08xPitch = (bno08x.getPitch())* CONST_180_DIVIDED_BY_PI; // Convert pitch to degrees
    
            bno08xHeading10x = (int16_t)(bno08xHeading * 10);
            bno08xRoll10x = (int16_t)(bno08xRoll * 10);
            
            //the heading x10
            data[5] = (uint8_t)bno08xHeading10x;
            data[6] = bno08xHeading10x >> 8;
    
            //the roll x10
            data[7] = (uint8_t)bno08xRoll10x;
            data[8] = bno08xRoll10x >> 8;
          }
     }
   else
     {
          //heading         
          data[5] = (uint8_t)9999;
          data[6] = 9999 >> 8;
  
          //roll
          data[7] = (uint8_t)8888;
          data[8] = 8888 >> 8;
      }

    int CK_A = 0;
    
    for (byte i = 2; i < sizeof(data) - 1; i++)
    {
        CK_A = (CK_A + data[i]);
    }
    
    data[sizeof(data) - 1] = CK_A;
    
    switch (trans) {
      case 0:  //serial out
        Serial.write(data, dataSize);
        Serial.flush(); //wait that data will be fully sent on serial
        break;
      case 1:  //udp out
        udp.beginPacket(udpAddress, udpPort);
        udp.write(data, dataSize);
        udp.endPacket();
        udp.flush();
        break;
//      case 2:  //bluetooth out
//        SerialBT.write(data, dataSize);
//        SerialBT.flush();//wait that data will be fully sent on BT serial
//        break;
      default:  //mauvaise config
        Serial.println("mauvais choix ou oubli de configuration");
        break;
    }

    //10 hz
    delay(REPORT_INTERVAL); 
  }
