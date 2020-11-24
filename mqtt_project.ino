#include <ESP8266WiFi.h>
#include "PubSubClient.h"
#include <DHT11.h>

char ssid[] = "AI-BIGDATA"; // "SO070VOIP4E71"; (집) "AI-BIGDATA";(학교)
char password[] = "aibd2019"; // "8D392A4E70"; (집) "aibd2019"; (학교)
byte server1[] = {192, 168, 0, 132};  //MQTT Server IP {192, 168, 200, 111}; 집 {192, 168, 0, 132}; (학교)

#define electricity A0 // 전압 핀 입력
const float VCC   = 5.0;// supply voltage is from 4.5 to 5.5V. Normally 5V.
const int model = 2;   // enter the model number (see below)

float cutOffLimit = 1.01;// set the current which below that value, doesn't matter. Or set 0.5

float sensitivity[] ={
          0.185,// for ACS712ELCTR-05B-T
          0.100,// for ACS712ELCTR-20A-T
          0.066// for ACS712ELCTR-30A-T
     
         };
const float QOV =   0.5 * VCC;// set quiescent Output voltage of 0.5V
float voltage;// internal variable for voltage

int port = 1883;

DHT11 dht11(4); // 4번핀 할당


void msgReceived(char *topic, byte *payload, unsigned int uLen){ //콜백함수 (topic -> 구독자, payload -> 넘어온 데이터 길이
  char pBuffer[uLen+1];//데이터가 문자열로 날라오는데 데이터를 담을 수 있는 배열선언
                       //c언어 : "1"+\0(문자열 끝) 문자열은 배열에 저장되어야 한다
  int i;
  for(i=0; i<uLen; i++){
    pBuffer[i] = payload[i];
  }
  pBuffer[i] = '\0'; // 끝을 의미

  // LED 관련정보
  Serial.println(pBuffer);
  if(pBuffer[0] == '1'){
    digitalWrite(14,HIGH);
  }
  else if(pBuffer[0] == '2')
  {
    digitalWrite(14, LOW);
  }
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg +=(char)payload[i];
  }
  Serial.print(msg);
  Serial.println();
}



WiFiClient client; // 와이파이 온습도 접속정보
WiFiClient client2;

//MQTTClient 만들기
PubSubClient mqttClient(server1, port, msgReceived, client);
PubSubClient mqttClient2(server1, port, callback, client2);

void setup() {
  pinMode(14,OUTPUT);
  Serial.begin(115200); //통신 속도 설정
  delay(10);
  Serial.println(); // 줄띄기
  Serial.println();
  Serial.println("Connecting to~");
  Serial.println(ssid); // WiFi 이름 출력 
  WiFi.begin(ssid,password); // SSID명, 비밀번호
  while(WiFi.status()!= WL_CONNECTED) { //WiFi에 접속이 안되어있다면
    delay(500); // 0.5초
    Serial.print(",");
   }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP()); //We-Mos에서 할당받은 WIFI IP 출력 (192.168.200.198)(집)

  // MQTT Server 접속
  if(mqttClient.connect("Arduino")){ // MQTT브로커에 접속을 시도하는 것 (Ardunino라는 이름으로 mqtt서버에 접속시도
      Serial.println("MQTT Broker Connected!");
      mqttClient.subscribe("led"); // 'led'라는 이름의 구독자 등록 (데이터를 읽어갈 구독자 등록) 
       
      
  }
  
}

void loop() {
  mqttClient.loop();
  float tmp, hum;
  int err = dht11.read(hum,tmp);
  if(err==0){
    char message[64] = "", pTmpBuf[50], pHumBuf[50];
    dtostrf(tmp,5,2,pTmpBuf);
    dtostrf(hum,5,2,pHumBuf);
    sprintf(message,"{\"tmp\":%s,\"hum\":%s}",pTmpBuf,pHumBuf);
    mqttClient.publish("dht11",message);
  }

  mqttClient2.loop();
  float voltage_raw =   (5.0 / 1023.0)* analogRead(electricity);// Read the voltage from sensor
  voltage =  voltage_raw - QOV + 0.012 ;// 0.000 is a value to make voltage zero when there is no current
  
  float current = voltage / sensitivity[model];

  //float err2 = electricity.read(current,voltage);
  if(abs(current) > cutOffLimit ){
    char message[64] = "", pCurBuf[50], pVolBuf[50];
    dtostrf(current,5,2,pCurBuf);
    dtostrf(voltage,5,2,pVolBuf);
    Serial.print("V: ");
    Serial.print(voltage,3);// print voltage with 3 decimal places
    Serial.print("V, I: ");
    Serial.print(current,2); // print the current with 2 decimal places
    Serial.println("A");

    sprintf(message,"{\"cur\":%s,\"vol\":%s}",pCurBuf,pVolBuf);
    mqttClient.publish("electricity",message);
  }else{
    Serial.println("No Current");
  } 
  delay(3000);
  
}
