#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <string>


//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

/* 1. Define the WiFi credentials */
#define WIFI_SSID "NAME"
#define WIFI_PASSWORD "PASSWORD"

/* 2. Define the API Key */
#define API_KEY "API_KEY"

/* 3. Define the RTDB URL */
#define DATABASE_URL "DB_URL" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "@"
#define USER_PASSWORD "PASSWD"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

//variables for current time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;   // GMT offset (seconds)
const int   daylightOffset_sec = 0;  //daylight offset (seconds)

//variables for valves status
int k1 = 0;
int k2 = 2;
int k3 = 4;
int t1 = 5;
int t2 = 15;
int t3 = 16;
int t4 = 17;
int t5 = 18;


//time init
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);




//reads int from DB
int readIntfromDb(String pwd){

int i = 0;
do{
  if (Firebase.getInt(fbdo, pwd)) {

  if (fbdo.dataType() == "int") {
    return fbdo.intData();
  }

  }else {
  Serial.println(fbdo.errorReason());
  i++;
  }

} while (i < 10);
// after 10 fails to read int returns 0 to keep valves off
return 0;
}

//reads string from db (Start time)
String readStringfromDb(String pwd){
int i = 0;
do{
  if(Firebase.getString(fbdo, pwd)){
    if(fbdo.dataType() == "string"){
      return fbdo.stringData();
    }
  }else {Serial.println(fbdo.errorReason()); i++;}

} while(i<10);
// after 10 fails to read int returns invalid time to keep valves off
return "24:00:00";
}

//gets local time from server as "HH:MM:SS"
String getLocalTime(){
  timeClient.update();

  return timeClient.getFormattedTime();

}

//converts time in string "HH:MM:SS" to int HHMM
int timeToInt(String stime){
  String formatedString = "HHMM";
  formatedString[0]=stime[0];
  formatedString[1]=stime[1];
  formatedString[2]=stime[3];
  formatedString[3]=stime[4];
  // formatedString[4]=stime[6];
  // formatedString[5]=stime[7];
  //Serial.println(formatedString);
  int out = formatedString.toInt();
  return out;
}

//subtract minutes from time
int timeToHH(String stime){
  String formatedString = "HH";
  formatedString[0]=stime[0];
  formatedString[1]=stime[1];
  //Serial.println(formatedString);
  int out = formatedString.toInt();
  return out;
}

//subtract hours from time
int timeToMM(String stime){
  String formatedString = "MM";
  formatedString[0]=stime[3];
  formatedString[1]=stime[4];
  //Serial.println(formatedString);
  int out = formatedString.toInt();
  return out;
}

//calculates delay whith added to start time returns end point of time interval
int endTime(int tab[], int n){
  int delay = 0;
  for (int i = 0; i <= n; i++)
  {
    delay += tab[i];
  }
  return delay;
}
//calculates delay whith added to start time returns start point of time interval
int startTime(int tab[], int n){
  int delay = 0;
  for (int i = 0; i < n; i++)
  {
    delay += tab[i];
  }
  return delay+1;
}

//sums start time "HH:MM" with delay "MM"
int sumTime(int h, int m, int addM){
  int H = h;
  m += addM;
  h = m/60;
  m = m % 60;
  H += h;

  String s;
  s.concat(H);

  if (m<10)
  {
    s.concat(0); 
  }
  
  s.concat(m);
  return s.toInt();

}

//reads int from DB for valves 
int valveState(int h){
  int state = 0;
  switch (h)
  {
  case 0:
   state = readIntfromDb("/valves/K1");
    break;

  case 1:
   state = readIntfromDb("/valves/K2");
    break;

  case 2:
   state = readIntfromDb("/valves/K3");
    break;

  case 3:
   state = readIntfromDb("/valves/T1");
    break;

  case 4:
   state = readIntfromDb("/valves/T2");
    break;

  case 5:
   state = readIntfromDb("/valves/T3");
    break;

  case 6:
   state = readIntfromDb("/valves/T4");
    break;

  case 7:
   state = readIntfromDb("/valves/T5");
    break;
  
  default:
    break;
  }

  return state;

}

//writes state to pins
void setValveState(int no, bool state) {
  switch (no)
  {
  case 0:
    digitalWrite(k1, state);
    break;

  case 1:
    digitalWrite(k2, state);
    break;

  case 2:
    digitalWrite(k3, state);
    break;

  case 3:
    digitalWrite(t1, state);
    break;

  case 4:
    digitalWrite(t2, state);
    break;

  case 5:
    digitalWrite(t3, state);
    break;

  case 6:
    digitalWrite(t4, state);
    break;

  case 7:
    digitalWrite(t5, state);
    break;
  
  default:
    break;
  }

}

//debug print
void debugLog(int i,int tab[]){
Serial.print("local time");
Serial.println(timeToInt(getLocalTime()));
Serial.print("valve n ");
Serial.println(i);
Serial.print("start time of valve");
Serial.println(sumTime(timeToHH(readStringfromDb("/time/startTime")),timeToMM(readStringfromDb("/time/startTime")),startTime(tab,i)));
Serial.print("end time of valve");
Serial.println(sumTime(timeToHH(readStringfromDb("/time/startTime")),timeToMM(readStringfromDb("/time/startTime")),endTime(tab,i)));
Serial.println("********************************");
}


void setup()
{

  Serial.begin(11520);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);

  pinMode(k1, OUTPUT);
  pinMode(k2, OUTPUT);
  pinMode(k3, OUTPUT);
  pinMode(t1, OUTPUT);
  pinMode(t2, OUTPUT);
  pinMode(t3, OUTPUT);
  pinMode(t4, OUTPUT);
  pinMode(t5, OUTPUT);

//time init
timeClient.begin();

}


void loop()
{
  //when DB is connected and time interval is reached
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    // reads valves states from DB to array
    int tab[]={readIntfromDb("/time/K1"),readIntfromDb("/time/K2"),readIntfromDb("/time/K3"),readIntfromDb("/time/T1"),readIntfromDb("/time/T2"),readIntfromDb("/time/T3"),readIntfromDb("/time/T4"),readIntfromDb("/time/T5")};
    // reads start time from DB to String
    String s = readStringfromDb("/time/startTime");
    //gets local time from server
    int localTime = timeToInt(getLocalTime());
    // extract hour from time
    int timeHH = timeToHH(s);
    // extract minutes from time
    int timeMM= timeToMM(s);

  // iterates for 8 valves
  for (int i = 0; i <= 7; i++)
  {
    // checks if time is between time stamps OR if state in DB is 1
    if ((localTime >= sumTime(timeHH,timeMM,startTime(tab,i)) && localTime <= sumTime(timeHH,timeMM,endTime(tab,i))) || valveState(i))
    {setValveState(i,1);}
    else{setValveState(i,0);}

  //debugLog(i,tab);
    
  }
  }
}

