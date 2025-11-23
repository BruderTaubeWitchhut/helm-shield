#include <WiFi.h>
#include <esp_now.h>
#include <TinyGPSPlus.h>

TinyGPSPlus gps;

#define GPS_RX 16
#define GPS_TX 17
#define SIM_RX 18
#define SIM_TX 19

#define EMERGENCY_NUM "+911234567890"

HardwareSerial SerialGPS(1);
HardwareSerial SerialSIM(2);

uint8_t selfMac[6];

struct AccPacket {
char id[6];
uint32_t seq;
uint8_t event;
float ax, ay, az;
float gx, gy, gz;
uint8_t vib;
uint32_t ts;
uint16_t crc;
} attribute((packed));

struct AckPacket {
uint32_t seq;
uint8_t status;
uint16_t crc;
} attribute((packed));

uint16_t crc16(const uint8_t *d, size_t l){
uint16_t c = 0xFFFF;
while(l--){
c ^= *d++;
for(uint8_t idx=0;idx<8;idx++){
if(c&1) c=(c>> 1)^0xA001;
else c>> =1;
}
}
return c;
}

void onDataRecv(const uint8_t *mac, const uint8_t *inData, int count) {
if(count < (int)sizeof(AccPacket)) return;

AccPacket p;
memcpy(&p, inData, sizeof(p));

uint16_t r = p.crc;
p.crc = 0;
uint16_t c = crc16((uint8_t*)&p, sizeof(p)-sizeof(p.crc));
if(r != c) return;

AckPacket ak;
ak.seq = p.seq;
ak.status = 1;
ak.crc = 0;
ak.crc = crc16((uint8_t*)&ak, sizeof(ak)-2);
esp_now_send(mac, (uint8_t*)&ak, sizeof(ak));

handleAlert(p);
}

void setup() {
Serial.begin(115200);

WiFi.mode(WIFI_STA);
WiFi.disconnect();

esp_wifi_get_mac(WIFI_IF_STA, selfMac);

if(esp_now_init() != ESP_OK){
while(1);
}

esp_now_register_recv_cb(onDataRecv);

SerialGPS.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
SerialSIM.begin(115200, SERIAL_8N1, SIM_RX, SIM_TX);

delay(1500);
initSimModule();
}

void initSimModule() {
sendSIM("AT");
sendSIM("ATE0");
sendSIM("AT+CMGF=1");
sendSIM("AT+CNMI=2,2,0,0,0");
}

String sendSIM(const char* cmd, unsigned long timeout=2000) {
while(SerialSIM.available()) SerialSIM.read();
SerialSIM.println(cmd);

unsigned long t = millis();
String r = "";
while(millis() - t < timeout){
if(SerialSIM.available()){
r += (char)SerialSIM.read();
}
if(r.indexOf("OK")!=-1 || r.indexOf("ERROR")!=-1) break;
}
return r;
}

bool waitForGPSFix(unsigned long t, double &lat, double &lon, double &hdop, unsigned long &age){
unsigned long s = millis();

while(millis() - s < t){
while(SerialGPS.available()){
gps.encode(SerialGPS.read());
}
if(gps.location.isValid() && gps.hdop.isValid() && gps.location.age() < 20000){
lat = gps.location.lat();
lon = gps.location.lng();
hdop = gps.hdop.hdop();
age = gps.location.age();
return true;
}
}

if(gps.location.isValid()){
lat = gps.location.lat();
lon = gps.location.lng();
hdop = gps.hdop.isValid() ? gps.hdop.hdop() : 99;
age = gps.location.age();
return false;
}

return false;
}

void handleAlert(const AccPacket &p){
double lat=0, lon=0, hdop=99;
unsigned long age=99999;

bool f = waitForGPSFix(15000, lat, lon, hdop, age);

char smsBuf[260];
snprintf(smsBuf, sizeof(smsBuf),
"ALERT! Helmet %s reported accident. T:%lu Lat:%.6f Lon:%.6f HDOP:%.1f Age:%lus",
p.id, (unsigned long)p.ts, lat, lon, hdop, age/1000
);

if(!isNetRegistered()) delay(3000);

sendSMS(EMERGENCY_NUM, smsBuf);
placeCall(EMERGENCY_NUM);
}

bool isNetRegistered(){
String r = sendSIM("AT+CREG?");
int idx = r.indexOf(",");
if(idx >= 0 && idx+1 < r.length()){
char s = r.charAt(idx+1);
if(s=='1' || s=='5') return true;
}
return false;
}

void sendSMS(const char* amount, const char* msg){
String c = String("AT+CMGS="") + amount + """;
SerialSIM.println(c);

delay(200);

SerialSIM.print(msg);
SerialSIM.write(0x1A);

unsigned long t = millis();
String r="";
while(millis()-t < 10000){
if(SerialSIM.available()) r += (char)SerialSIM.read();
if(r.indexOf("OK")!=-1 || r.indexOf("ERROR")!=-1) break;
}
}

void placeCall(const char* amount){
String c = String("ATD") + amount + ";";
sendSIM(c.c_str(), 10000);
delay(15000);
sendSIM("ATH");
}