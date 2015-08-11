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
String upload_led;

LGPRSClient c2;
HttpClient http(c2);

void setup() {
  while(!LGPRS.attachGPRS()) {
    delay(500);
  }
  Serial.begin(115200);
  while(!Serial) delay(1000);
  
  Serial.println("calling connection");

  while (!c2.connect(SITE_URL, 80)) {
    Serial.println("Re-Connecting to WebSite");
    delay(1000);
  }
  delay(100);

  getconnectInfo();
  connectTCP();
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
  while (!c2.available()) {
    Serial.println("waiting HTTP response: ");
    Serial.println(errorcount);
    errorcount += 1;
    if (errorcount > 10) {
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
  while (c2) {
    int v = c2.read();
    if (v != -1) {
      c = v;
      Serial.print(c);
      connection_info[ipcount]=c;
      if(c==',')
      separater=ipcount;
      ipcount++;    
    }
    else {
      Serial.println("no more content, disconnect");
      c2.stop();
    }
  }
  Serial.print("The connection info: ");
  Serial.println(connection_info);
  int i;
  for(i=0;i<separater;i++)
  {  ip[i]=connection_info[i];
  }
  int j=0;
  separater++;
  for(i=separater;i<21 && j<5;i++)
  {  port[j]=connection_info[i];
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

  while (!c2.connect(SITE_URL, 80)) {
    Serial.println("Re-Connecting to WebSite");
    delay(1000);
  }
  delay(100);
  upload_led = "orderNum,,";
  int thislength = upload_led.length()+1;
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
  c2.print(upload_led);
  c2.println(random(1,9));
  
  delay(500);

  int errorcount = 0;
  while (!c2.available()) {
    Serial.print("waiting HTTP response: ");
    Serial.println(errorcount);
    errorcount += 1;
    if (errorcount > 10) {
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
  while (c2) {
    int v = c2.read();
    if (v != -1)
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

void heartBeat() {
  Serial.println("send TCP heartBeat");
  c.println(tcpdata);
  c.println();
}

void loop() {
  LDateTime.getRtc(&rtc);
  if ((rtc - lrtc) >= per) {
    heartBeat();
    lrtc = rtc;
  }
  //Check for report datapoint status interval
  LDateTime.getRtc(&rtc1);
  if ((rtc1 - lrtc1) >= per1) {
    uploadstatus();
    lrtc1 = rtc1;
  }
}
