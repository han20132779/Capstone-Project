// Arducam mini controlled by ESP8266.  Arduino IDE 1.6.5. Photo upload to a php script on a webserver using html POST.
// Libraries from Arducam github placed under arduino/libraries/arduCAM. Power consumption ca 200 mA from 3.3V.
// PIR module controls FET that switches on 3.3 power Lipo+buck regulator used.  ESP connects to WiFi and starts taking and uploading photos.
// http://www.arducam.com/tag/arducam-esp8266/
// ESP12-E module: CS=GPIO16   MOSI=GPIO13   MISO=GPIO12   SCK=GPIO14  GND&GPIO15  VCC&EN=3.3V SDA=GPIO4  SCL=GPIO5  GPIO0=button  GPIO2=BlueLED

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <Servo.h>
#include "memorysaver.h"

const int gasPin = A0;
static const size_t bufferSize = 1024;
static uint8_t buffer[1025] = {0xFF};
uint8_t temp = 0, temp_last = 0;
int i = 0;
bool is_header = false;
Servo servo; 

WiFiClient client;
// Enabe debug tracing to Serial port.
 // Here we define a maximum framelength to 64 bytes. Default is 256.
  // Define how many callback functions you have. Default is 1.
const int CS = 16; // chip select of camera

ArduCAM myCAM(OV5642, CS);


void Camera(ArduCAM myCAM){      //reads out pixels from the Arducam mini module

   myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  Serial.print("Picture captured. ");

  
  uint32_t len = myCAM.read_fifo_length();
  if (len >= 393216){ Serial.println("Over size."); return;}
  else if (len == 0){ Serial.println("Size is 0."); return;}
  Serial.print("Length in bytes: "); Serial.println(len); Serial.println();
  myCAM.CS_LOW(); myCAM.set_fifo_burst(); 
  
  Serial.println("Connection trying....---------------- ");
  if (client.connect("teamssdweb.kr", 80)==1) { 
    Serial.println("Connection Complete! ");
  while(client.available()) {String line = client.readStringUntil('\r');   
  }  // Empty wifi receive bufffer

  String start_len ;
  String start_request = ""; String end_request = ""; String start_request2 = ""; String start_request3 = "";
  start_request = start_request + "--AaB03x";
  start_request2 = start_request2 + "Content-Disposition: form-data; name=\"userfile\"; filename=\"CAM.JPEG\"";
  start_request3 = start_request3 + "Content-Type: image/jpeg";
  start_len =  "\n" + start_request +"\n" + start_request2+ "\n" + start_request3 + "\n\n";
  end_request = end_request + "--AaB03x--";  // in file upload POST method need to specify arbitrary boundary code

  uint16_t full_length;
  full_length = start_len.length() + len + end_request.length() + 4;

    Serial.println("POST /mobilewebcam.php HTTP/1.1");
    Serial.println("Host: teamssdweb.kr");
    Serial.println("Content-Type: multipart/form-data; boundary=AaB03x");
    Serial.print("Content-Length: "); Serial.println(full_length);
    Serial.print(start_request);  Serial.println(end_request); 
    Serial.println("----------------------------Start Post Request----------------------------- ");
   
    
    client.println("POST /mobilewebcam.php HTTP/1.1");
    client.println("Host: teamssdweb.kr");    
    client.println("Content-Type: multipart/form-data; boundary=AaB03x");
    client.print("Content-Length: "); client.println(full_length);
    client.println();
    client.println(start_request);
    client.println(start_request2);
    client.println(start_request3);
    client.println();

    
  // Read image data from Arducam mini and send away to internet
  static const size_t bufferSize = 1024; // original value 4096 caused split pictures
  static uint8_t buffer[bufferSize] = {0xFF};
  
  while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      if (!client.connected()) break;
      client.write(&buffer[0], will_copy);
      len -= will_copy;
      delay(500);
      }
   client.println();
     client.println(end_request);
     client.println();
 
     myCAM.CS_HIGH(); //digitalWrite(led, HIGH);
  }
  
  // Read  the reply from server
  delay(3000); 
  while(client.available()){ String line = client.readStringUntil('\r'); Serial.print(line);} 
  client.stop();  
  while(client.available()){ String line = client.readStringUntil('\r'); Serial.print(line);}
  delay(3000);
}

void setup() {
  uint8_t vid, pid, temp;
  Wire.begin();
  Serial.begin(115200); Serial.println("ArduCAM Mini ESP8266 uploading photo to server");

  pinMode(CS, OUTPUT); // set the CS as an output:
  SPI.begin(); // initialize SPI
  SPI.setFrequency(4000000); //4MHz
  
  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55){Serial.println("SPI1 interface Error!");while(1);}

  
#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
    Serial.println(F("Can't find OV2640 module!"));
  else
    Serial.println(F("OV2640 detected."));
#elif defined (OV5640_MINI_5MP_PLUS) || defined (OV5640_CAM)
  //Check if the camera module type is OV5640
  myCAM.wrSensorReg16_8(0xff, 0x01);
  myCAM.rdSensorReg16_8(OV5640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5640_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x40))
    Serial.println(F("Can't find OV5640 module!"));
  else
    Serial.println(F("OV5640 detected."));
#elif defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
  //Check if the camera module type is OV5642
  myCAM.wrSensorReg16_8(0xff, 0x01);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x42)) {
    Serial.println(F("Can't find OV5642 module!"));
  }
  else
    Serial.println(F("OV5642 detected."));
#endif

   
   myCAM.set_format(JPEG);
  myCAM.InitCAM();
#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
#elif defined (OV5640_MINI_5MP_PLUS) || defined (OV5640_CAM)
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
  myCAM.OV5640_set_JPEG_size(OV5640_320x240);
#elif defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
  myCAM.OV5640_set_JPEG_size(OV5642_1280x960);
#endif

  // Connect to a router
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to AP specified during programming");
  
  WiFi.begin("han", "123456789");  // 와이파이 아이디 및 비밀 번호 입력
  
  while (WiFi.status() != WL_CONNECTED) {delay(500); Serial.print("."); }
  Serial.print("\r\nWiFi connected IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  }
  
void loop() {
  
  for(int i = 0 ; i < 12; i++) {
    servo.attach(2);
    servo.writeMicroseconds(95);
    delay(20);
    servo.detach();
    delay(1000);
    Camera(myCAM);
    Serial.println("captured");  
  }
  
  servo.attach(2);
  servo.write(50);
  delay(580);
  servo.detach();
  delay(3000);
  

 /* if(analogRead(gasPin)>150){
      Serial.println("----------------------Smoking Detected-----------------------------");
      Camera(myCAM);  
  }
  
  Serial.println("Sleeping ...");
  delay(5000);*/

}
