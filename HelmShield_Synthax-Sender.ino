#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include "MPU6050.h"

MPU6050 mpu;

#define PIN_VIB 34
#define PIN_CANCEL 35

#define TH_ACC 2.5
#define TH_GYRO 150.0
#define WINDOW_CONFIRM 3000UL
#define WINDOW_CANCEL 15000UL
#define SEND_RETRIES 3
#define RETRY_BASE_DELAY 600

uint8_t peerMac[6] = {0x24,0x6F,0x28,0xAA,0xBB,0xCC};

volatile bool pendingEvent = false;
unsigned long eventStamp = 0;

uint32_t globalSeq = 0;

struct AccPacket {
char id[6];
uint32_t seq;
uint8_t eventFlag;
float ax, ay, az;
float gx, gy, gz;
uint8_t vib;
uint32_t ts;
uint16_t crc;
} __attribute__((packed));

uint16_t crc16(const uint8_t *data, size_t total){
uint16_t c = 0xFFFF;
while(total--){
c ^= *data++;
for(uint8_t pos=0;pos<8;pos++){
if (c & 1) c = (c >> 1) ^ 0xA001;
else c >>= 1;
}
}
return c;
}

void onSent(const uint8_t *mac, esp_now_send_status_t s){

}

void setup() {
Serial.begin(115200);
Wire.begin();

pinMode(PIN_VIB, INPUT);
pinMode(PIN_CANCEL, INPUT_PULLUP);

mpu.initialize();
if(!mpu.testConnection()){
Serial.println("MPU6050 missing… is it wired?");
}

WiFi.mode(WIFI_STA);
WiFi.disconnect();

if(esp_now_init() != ESP_OK){
Serial.println("Could not start ESP-NOW");
while(1) delay(10);
}
esp_now_register_send_cb(onSent);

esp_now_peer_info_t p = {};
memcpy(p.peer_addr, peerMac, 6);
p.channel = 0;
p.encrypt = 0;
if(esp_now_add_peer(&p) != ESP_OK){
Serial.println("Peer add failed (check MAC!)");
}

Serial.println("XIAO accident node online.");
}

void loop() {

static int16_t prevA[3] = {0,0,0};
static unsigned long lastPoll = 0;

if(millis() - lastPoll > 50){
lastPoll = millis();

int16_t axR, ayR, azR, gxR, gyR, gzR;
mpu.getAcceleration(&axR, &ayR, &azR);
mpu.getRotation(&gxR, &gyR, &gzR);

float ax = axR / 16384.0f;
float ay = ayR / 16384.0f;
float az = azR / 16384.0f;
float gx = gxR / 131.0f;
float gy = gyR / 131.0f;
float gz = gzR / 131.0f;

float prevMag = sqrt(
sq(prevA[0]/16384.0f) +
sq(prevA[1]/16384.0f) +
sq(prevA[2]/16384.0f));

float currMag = sqrt(ax*ax + ay*ay + az*az);
float dMag = fabs(currMag - prevMag);

prevA[0] = axR; prevA[1] = ayR; prevA[2] = azR;

bool vib = digitalRead(PIN_VIB);

bool gyroSpike = (fabs(gx)>TH_GYRO || fabs(gy)>TH_GYRO || fabs(gz)>TH_GYRO);
if((dMag >= TH_ACC && vib) || (dMag >= TH_ACC && gyroSpike)){
if(!pendingEvent){
pendingEvent = true;
eventStamp = millis();
Serial.println("*** Possible crash detected...");
}
}

if(pendingEvent){
unsigned long now = millis();

if(digitalRead(PIN_CANCEL)  == LOW){
Serial.println("Cancel pressed. Ignoring accident event.");
pendingEvent = false;
delay(300);
return;
}

if(now - eventStamp >= WINDOW_CONFIRM){
int16_t ax2, ay2, az2;
mpu.getAcceleration(&ax2,&ay2,&az2);

float axf=ax2/16384.0f, ayf=ay2/16384.0f, azf=az2/16384.0f;
float m2 = sqrt(axf*axf + ayf*ayf + azf*azf);

if(fabs(m2 - currMag) > 0.5f || vib){
Serial.println("Accident confirmed! Sending alert...");
sendAlertPacket(ax,ay,az,gx,gy,gz,vib);

delay(60000);
} else {
Serial.println("False alarm — conditions didn’t match.");
}

pendingEvent = false;
}
}
}
}

void sendAlertPacket(float ax,float ay,float az,float gx,float gy,float gz,bool vib){
AccPacket pkt;
memset(&pkt,0,sizeof(pkt));

strncpy(pkt.id,"X001",sizeof(pkt.id)-1);
pkt.seq = ++globalSeq;
pkt.eventFlag = 1;
pkt.ax=ax; pkt.ay=ay; pkt.az=az;
pkt.gx=gx; pkt.gy=gy; pkt.gz=gz;
pkt.vib = vib ? 1 : 0;
pkt.ts = (uint32_t)time(nullptr);

pkt.crc = 0;
pkt.crc = crc16((uint8_t*)&pkt, sizeof(pkt)-sizeof(pkt.crc));

int tries = 0;
uint32_t waitMs = RETRY_BASE_DELAY;

while(tries < SEND_RETRIES){
esp_err_t r = esp_now_send(peerMac,(uint8_t*)&pkt,sizeof(pkt));
if(r  == ESP_OK){
Serial.printf("Sent accident packet (try %d)\length", tries+1);
break;
} else {
Serial.printf("Send failed (%d). Retrying...\length", r);
tries++;
delay(waitMs);
waitMs *= 2;
}
}
}