#include <Arduino.h>
#include <DHT.h>
#define     DHTPIN 23       //pin del sensor dht22
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

float temp, hum , tmin, tmax, max2;
float min2 = 100;  //temperatura min si no se igual a 100 se va a 0

//funciones para saber la temperatura min y maximas
float readDHTTemperature2() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t2 = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(t2)) {    
    Serial.println("Failed to read from DHT sensor!");
    return 500;
  }
  else {
    Serial.println(t2);
    return t2;
  }
}
float tempMin(){
  
  float min = readDHTTemperature2();
  if(min < min2){
    min2 = min;
  }else if(min == 0){
    min2 = readDHTTemperature2();
  }
  Serial.println(min2);
  return min2;
}  
float tempMax(){
  
  float max = dht.readTemperature();
  if(max > max2){
    max2 = max;
  }
  Serial.println(max2);
  return max2;
}
