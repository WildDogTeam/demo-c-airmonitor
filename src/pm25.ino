/*
  pm25 demo.

  This sketch demonstrate how to updata  yourair quality data to the wilddog cloud.

  A Wilddog account are necessary to run this demo. 
  If you don't already have one, you can register for a free Wilddog account at 
  https://www.wilddog.com/ 
  
  In order to run this sketch, you'll need to creat an application using
  the Wilddog dashboard console at https://www.wilddog.com/dashboard. 
  After creating the app, you'll get an url which type following 
  https://YourAppId.wilddogio.com/
  Your data can add and save under that url,which will be access in this sketch. 

  Note that since this sketch will access your data on Wilddog,
  your Arduino Yun need to connect to the Tnternet first.
  
  usage:
  1. connect sersor DHT11 and gp2y1010 to your arduino.
            
  2. Modify YOURURL to your application.
  3. Upload to your Arduino.
  4. log in your application your will see PM25/temperature/humidity node was build .
    
  created on 2015/12/19.
  by skyli.
  
  https://www.wilddog.com/  
  for more information.
*/
#include <Wilddog.h>
#include <dht11.h>
#include "Wilddog_utility.h"

#define YOURURL  "coap://YourAppId.wilddogio.com/"
#define KEY_PM25    "PM25"
#define KEY_temperature    "temperature"
#define KEY_humidity        "humidity"

#define JSON_DATA_LEN   (256)

int measurePin = 0; //Connect dust sensor to Arduino A0 pin
int ledPower = 2;   //Connect 3 led driver pins of dust sensor to Arduino D2

#define DHT11PIN 3  //  DHT11 Data pin connect to D3

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
int g_loop_cnt = 0;

Wilddog *p_wd = NULL;
dht11 DHT11;
char json_data[JSON_DATA_LEN];

/*
 * Function:    getTemperatureHumidity
 * Description: read sersor DHT11's value ,get current temperature and mumidity.
 * Input:       N/A
 * Output:      *temperature : current temperature ;
 *              *p_humidity :  current humidity;
 * Return:     fault return -1,success return 0.
*/
int getTemperatureHumidity(unsigned int *temperature, unsigned int *p_humidity)
{
  int res = -1, chk = 0 ;
  
  chk = DHT11.read(DHT11PIN);
  switch (chk)
  {
    case DHTLIB_OK: 
    res = 0;
    break;
    case DHTLIB_ERROR_CHECKSUM: 
    Serial.println("Checksum error"); 
    break;
    case DHTLIB_ERROR_TIMEOUT: 
    Serial.println("Time out error"); 
    break;
    default: 
    Serial.println("Unknown error"); 
    break;
  }
  *p_humidity =  DHT11.humidity;
  *temperature = DHT11.temperature;

  return res;
}
/*
 * Function:    get_dustDensity.
 * Description: read sersor GP2Y1010AU0F's value ,get current dust Density.
 * Input:       N/A.
 * Output:     current dust Density.
 * Return:     current dust Density.
*/
unsigned int get_dustDensity()
{
    float voMeasured = 0;
    float calcVoltage = 0;
    float dustDensity = 0;

  digitalWrite(ledPower,LOW); // power on the LED
  delayMicroseconds(samplingTime);

  voMeasured = analogRead(measurePin); // read the dust value
  
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH); // turn the LED off
  delayMicroseconds(sleepTime);

  // 0 - 5V mapped to 0 - 1023 integer values
  // recover voltage
  calcVoltage = voMeasured * (5.0 / 1024.0);
  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  //unit: mg/m3
  dustDensity = 0.17 * calcVoltage;

  return (100*dustDensity);
}
/*
 * Function:    setValueCallBack.
 * Description: set query respond.
 * Input:      pdata: pointer of respond data.
 *             error : respond error code.
 *             arg :  user arg
 * Output:     N/A.
 * Return:     N/A.
*/                      
void setValueCallBack(const char *pdata, int error, void* arg)
{
  Serial.print("\n get error : ");
  Serial.print(error);
  if(pdata)
  {
      Serial.print("\n get data : ");
      Serial.print(pdata);
    }
  if (arg)
     Serial.print(*(char*)arg);
}
/*
 * Function:    updataPM25ToServer.
 * Description: Updata dust density and temperature and humidity to server.
 * Input:      dustDensity : dust density's value.
 *             temp: temperature.
 *             humidity: humidity's value.
 * Output:     N/A.
 * Return:     N/A.
*/ 
int updataPM25ToServer( unsigned int  dustDensity , unsigned int temp,unsigned int humidity)
{
  int res = 0; 
  
  if(p_wd == NULL)
    return -1;
    
  memset(json_data,0,JSON_DATA_LEN);

  sprintf(json_data,"{\"%s\":\"%u\",\"%s\":\"%u\",\"%s\":\"%u\"}",KEY_PM25,dustDensity,KEY_temperature,temp,KEY_humidity,humidity);
  Serial.print(json_data);
  Serial.print("\n");
  res = p_wd->setValue(json_data,setValueCallBack,(void*)NULL);
  if( res  < 0 )
    Serial.print("set value error !! \n");

  return res;
 }

void setup(){
  Serial.begin(9600);
  pinMode(ledPower,OUTPUT);
   // Wait until a Serial Monitor is connected.
  while (!Serial);
  Serial.print("\n start... \n");
  p_wd = new  Wilddog(YOURURL); 
}

void loop(){
  
  int res = 0;
  /*per 3 second send data*/
  if(g_loop_cnt++ %3 == 0)
  {    
    unsigned int  dustDensity ,temp,humidity;
    /* integration filter should be doing there. */
    dustDensity = get_dustDensity();
    res = getTemperatureHumidity(&temp,&humidity);
    if(res >= 0 )
      updataPM25ToServer(dustDensity,temp,humidity);
  }
  if(p_wd)
    p_wd->trySync();
  delay(1000);
}

