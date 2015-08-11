#include<LAudio.h>
AudioStatus status;

#include <HttpClient.h>
#include <LGPRS.h>
#include <LGPRSServer.h>
#include <LGPRSClient.h>
#include <LDateTime.h>
#define per 50
#define per1 10
#define DEVICEID "DL3c6ORp" // Input your deviceId
#define DEVICEKEY "tFi1j0K6Me4WJnpS" // Input your deviceKey
#define SITE_URL "api.mediatek.com"
LGPRSClient c;
unsigned int rtc;
unsigned int lrtc;
unsigned int rtc1;
unsigned int lrtc1;
char port[4]={0};
char connection_info[21]={0};
char ip[21]={0};             
int portnum;
int val = 0;
String tcpdata = String(DEVICEID) + "," + String(DEVICEKEY) + ",0";
String upload_order;
LGPRSClient c2;
HttpClient http(c2);

#include <arduino.h>
#include <LTask.h>
#include <LFlash.h>
#include <LStorage.h>
#define Drv LFlash
#define PIC_PKT_LEN    128 //128/512
#define PIC_FMT_VGA    7
#define PIC_FMT_CIF    5
#define PIC_FMT_OCIF   3
#define CAM_ADDR       0
#define CAM_SERIAL     Serial1
#define PIC_FMT        PIC_FMT_VGA
LFile myFile;
const byte cameraAddr = (CAM_ADDR << 5);  // addr
unsigned long picTotalLen = 0;            // picture length
bool start = false;
datetimeInfo t;

unsigned long lastClick;
int clickCount = 0;


void setup() {
  for(int i = 2 ; i < 6; i++){
    pinMode(i,OUTPUT);
    digitalWrite(i,LOW);
  }
  
  LAudio.begin();
  LAudio.setVolume(6);
  Serial.begin(115200);
  
  pinMode(6,INPUT);
  
  Serial1.begin(115200);
  LTask.begin();
  Drv.begin();
  Serial.println("Initialization begin...");
  initialize();
  Serial.println("Good to go!");
}

/*********************************************************************/
void clearRxBuf() {
  while(Serial1.available()) {
    Serial1.read();
  }
}
/*********************************************************************/
void sendCmd(char cmd[], int cmd_len) {
    for(char i = 0; i < cmd_len; i++) Serial1.write(cmd[i]);
}
/*********************************************************************/
void initialize() {
  char cmd[] = {0xaa,0x0d|cameraAddr,0x00,0x00,0x00,0x00} ;
  unsigned char resp[6];
  Serial1.setTimeout(500);
  while(1) {
    //clearRxBuf();
    sendCmd(cmd,6);
    if(Serial1.readBytes((char *)resp, 6) != 6) {
      continue;
    }
    if(resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x0d && resp[4] == 0 && resp[5] == 0) {
      if(Serial1.readBytes((char *)resp, 6) != 6) continue;
      if(resp[0] == 0xaa && resp[1] == (0x0d | cameraAddr) && resp[2] == 0 && resp[3] == 0 && resp[4] == 0 && resp[5] == 0) break;
    }
  }
  cmd[1] = 0x0e | cameraAddr;
  cmd[2] = 0x0d;
  sendCmd(cmd, 6);
  Serial.println("Camera initialization done.");
}
/*********************************************************************/
void preCapture() {
  char cmd[] = { 0xaa, 0x01 | cameraAddr, 0x00, 0x07, 0x00, PIC_FMT };
  unsigned char resp[6];
  Serial1.setTimeout(100);
  while(1) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if(Serial1.readBytes((char *)resp, 6) != 6) continue;
    if(resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x01 && resp[4] == 0 && resp[5] == 0) break;
  }
}
void Capture() {
  char cmd[] = { 0xaa, 0x06 | cameraAddr, 0x08, PIC_PKT_LEN & 0xff, (PIC_PKT_LEN>>8) & 0xff ,0};
  unsigned char resp[6];
  Serial1.setTimeout(100);
  while(1) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if(Serial1.readBytes((char *)resp, 6) != 6) continue;
    if(resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x06 && resp[4] == 0 && resp[5] == 0) break;
  }
  cmd[1] = 0x05 | cameraAddr;
  cmd[2] = 0;
  cmd[3] = 0;
  cmd[4] = 0;
  cmd[5] = 0;
  while(1) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if(Serial1.readBytes((char *)resp, 6) != 6) continue;
    if(resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x05 && resp[4] == 0 && resp[5] == 0) break;
  }
  Serial.println("get snapshot");
  cmd[1] = 0x04 | cameraAddr;
  cmd[2] = 0x1;
  while(1) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if(Serial1.readBytes((char *)resp, 6) != 6) continue;
    if(resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x04 && resp[4] == 0 && resp[5] == 0) {
      Serial1.setTimeout(1000);
      if(Serial1.readBytes((char *)resp, 6) != 6) {
        continue;
      }
      if(resp[0] == 0xaa && resp[1] == (0x0a | cameraAddr) && resp[2] == 0x01) {
        picTotalLen = (resp[3]) | (resp[4] << 8) | (resp[5] << 16);
        Serial.print("picTotalLen:");
        Serial.println(picTotalLen);
        break;
      }
    }
  }
}
/*********************************************************************/
void GetData() {
  unsigned int pktCnt = (picTotalLen) / (PIC_PKT_LEN - 6);
  if ((picTotalLen % (PIC_PKT_LEN-6)) != 0) pktCnt += 1;
  char cmd[] = { 0xaa, 0x0e | cameraAddr, 0x00, 0x00, 0x00, 0x00 };
  unsigned char pkt[PIC_PKT_LEN];
  Serial.println("get data begin");
  LDateTime.getTime(&t);
  Serial.println("get data middle");
  char picName[] = "pic000000.jpg";
  picName[3] = t.hour/10 + '0';
  picName[4] = t.hour%10 + '0';
  picName[5] = t.min/10 + '0';
  picName[6] = t.min%10 + '0';
  picName[7] = t.sec/10 + '0';
  picName[8] = t.sec%10 + '0';
  if(Drv.exists(picName)) {
      Drv.remove(picName);
  }
  myFile = Drv.open(picName, FILE_WRITE);
  if(!myFile) {
    Serial.println("myFile open fail...");
  }
  else {
    Serial1.setTimeout(1000);
    for(unsigned int i = 0; i < pktCnt; i++) {
      cmd[4] = i & 0xff;
      cmd[5] = (i >> 8) & 0xff;
      int retry_cnt = 0;
      retry:
      delay(10);
      clearRxBuf();
      sendCmd(cmd, 6);
      uint16_t cnt = Serial1.readBytes((char *)pkt, PIC_PKT_LEN);
      //Serial.println(cnt);
      unsigned char sum = 0;
      for(int y = 0; y < cnt - 2; y++) {
        sum += pkt[y];
      }
      if(sum != pkt[cnt-2]) {
        //if (++retry_cnt < 100){Serial.println("retry");goto retry;}
        if (++retry_cnt < 100) goto retry;
        else break;
      }
      //Serial.println((int)&pkt[4]);
      myFile.write((const uint8_t *)&pkt[4], cnt-6);
      //if (cnt != PIC_PKT_LEN) break;
    }
    cmd[4] = 0xf0;
    cmd[5] = 0xf0;
    sendCmd(cmd, 6);
  }
  myFile.close();
  Serial.println("get data end");
}

void playWords(int num) {
  switch(num){
    case 1:
      LAudio.playFile(storageFlash,(char*)"hello.wav");
      break;
    case 2:
      LAudio.playFile(storageFlash,(char*)"order.wav");
      break;
    case 3:
      LAudio.playFile(storageFlash,(char*)"done.wav");
      break;
  }
  Serial.println("song picked");
  do{
    status = LAudio.getStatus();
    Serial.println("not finished playing yet...");
  }while(status != AudioEndOfFile);
  delay(200);
}

void getconnectInfo(){
  //calling RESTful API to get TCP socket connection
  c2.print("GET /mcs/v2/devices/");
  c2.print(DEVICEID);
  c2.println("/connections.csv HTTP/1.1");
  c2.print("Host: ");
  c2.println(SITE_URL);
  c2.print("deviceKey: ");
  c2.println(DEVICEKEY);
  c2.println("Connection: close");
  c2.println();
  delay(500);
  int errorcount = 0;
  while(!c2.available()) {
    Serial.println("waiting HTTP response: ");
    Serial.println(errorcount);
    errorcount += 1;
    if(errorcount > 50) {
      c2.stop();
      return;
    }
    delay(100);
  }
  int err = http.skipResponseHeaders();
  int bodyLen = http.contentLength();
  Serial.print("Content length is: ");
  Serial.println(bodyLen);
  Serial.println();
  char c;
  int ipcount = 0;
  int count = 0;
  int separater = 0;
  while(c2) {
    int v = c2.read();
    if(v != -1) {
      c = v;
      Serial.print(c);
      connection_info[ipcount]=c;
      if(c==',')
      separater=ipcount;
      ipcount++;    
    } else {
      Serial.println("no more content, disconnect");
      c2.stop();
    }
  }
  Serial.print("The connection info: ");
  Serial.println(connection_info);
  int i;
  for(i=0;i<separater;i++) {
    ip[i]=connection_info[i];
  }
  int j=0;
  separater++;
  for(i=separater;i<21 && j<5;i++) {
    port[j]=connection_info[i];
    j++;
  }
  Serial.println("The TCP Socket connection instructions:");
  Serial.print("IP: ");
  Serial.println(ip);
  Serial.print("Port: ");
  Serial.println(port);
  portnum = atoi (port);
  Serial.println(portnum);
} //getconnectInfo

void uploadstatus(){
  //calling RESTful API to upload datapoint to MCS to report LED status
  Serial.println("calling connection");
  while(!c2.connect(SITE_URL, 80)) {
    Serial.println("Re-Connecting to WebSite");
    delay(1000);
  }
  delay(100);
  upload_order = "orderNum,,";
  int thislength = upload_order.length()+1;
  HttpClient http(c2);
  c2.print("POST /mcs/v2/devices/");
  c2.print(DEVICEID);
  c2.println("/datapoints.csv HTTP/1.1");
  c2.print("Host: ");
  c2.println(SITE_URL);
  c2.print("deviceKey: ");
  c2.println(DEVICEKEY);
  c2.print("Content-Length: ");
  c2.println(thislength);
  c2.println("Content-Type: text/csv");
  c2.println("Connection: close");
  c2.println();
  c2.print(upload_order);
  c2.println(clickCount);////////////////
  delay(500);
  int errorcount = 0;
  while(!c2.available()) {
    Serial.print("waiting HTTP response: ");
    Serial.println(errorcount);
    errorcount += 1;
    if(errorcount > 50) {
      c2.stop();
      return;
    }
    delay(100);
  }
  int err = http.skipResponseHeaders();
  int bodyLen = http.contentLength();
  Serial.print("Content length is: ");
  Serial.println(bodyLen);
  Serial.println();
  while(c2) {
    int v = c2.read();
    if(v != -1)
      Serial.print(char(v));
    else {
      Serial.println("no more content, disconnect");
      c2.stop();
    }
  }
}

void connectTCP(){
  c.stop();
  Serial.println("Connecting to TCP");
  Serial.println(ip);
  Serial.println(portnum);
  while (0 == c.connect(ip, portnum)) {
    Serial.println("Re-Connecting to TCP");    
    delay(1000);
  }  
  Serial.println("send TCP connect");
  c.println(tcpdata);
  c.println();
  Serial.println("waiting TCP response:");
} //connectTCP

void loop() {
  clickCount = 0;
  if(digitalRead(6) == 1){
    delay(50);
    while(digitalRead(6) == 1);
    Serial.println("Starting...");
    
    //move robot
    digitalWrite(2,HIGH);
    digitalWrite(4,HIGH);
    delay(1000);
    digitalWrite(2,LOW);
    digitalWrite(4,LOW);
    
    //play welcoming words
    playWords(1);
    Serial.println("Play 1 finished");
    
    //take a photo
    preCapture();
    Capture();
    GetData();
    
    //play request for order
    playWords(2);
    Serial.println("Play 2 finished");
    
    //ordering
    lastClick = millis();
    while(millis()-lastClick < 3000){
      if(digitalRead(6) == 1){
        delay(100);
        while(digitalRead(6) == 1);//until click released
        clickCount++;
        Serial.print("pressed ");
        Serial.println(clickCount);
        lastClick = millis();
      }
    }
    Serial.print("You've clicked ");
    Serial.print(clickCount);
    Serial.println(" times.");
    
    //uploading to MCS
    while(!LGPRS.attachGPRS()) {
      delay(500);
    }
    Serial.println("calling connection");
    while (!c2.connect(SITE_URL, 80)) {
      Serial.println("Re-Connecting to WebSite");
      delay(1000);
    }
    delay(100);
    getconnectInfo();
    connectTCP();
    uploadstatus();
    
    //play finish words
    playWords(3);
    Serial.println("Play 3 finished");
    
    //move robot back
    digitalWrite(3,HIGH);
    digitalWrite(5,HIGH);
    delay(1000);
    digitalWrite(3,LOW);
    digitalWrite(5,LOW);
  }
}
