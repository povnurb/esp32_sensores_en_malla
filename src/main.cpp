/**https://github.com/gmag11/painlessMesh
 * 1 .- agregar el protocolo ESP_NOW a esta malla (protocolo MESH)
*/

/*
Introducción a PainlessMesh
painlessMesh es una biblioteca que se ocupa de los detalles de la creación de una red de malla simple utilizando hardware esp8266 y esp32. 
El objetivo es permitir que el programador trabaje con una red de malla sin tener que preocuparse por cómo se estructura o administra la red.

Verdaderas redes ad-hoc
painlessMesh es una verdadera red ad-hoc, lo que significa que no se requiere planificación, 
controlador central o enrutador. Cualquier sistema de 1 o más nodos se autoorganizará en una malla 
completamente funcional. El tamaño máximo de la malla está limitado (creemos) por la cantidad de memoria 
en el montón que se puede asignar al búfer de subconexiones y, por lo tanto, debería ser bastante alto.

Wi-Fi y redes
painlessMesh está diseñado para usarse con Arduino, pero no usa las bibliotecas WiFi de Arduino, 
ya que teníamos problemas de rendimiento (principalmente latencia) con ellas. 
Más bien, la red se realiza utilizando las bibliotecas SDK nativas esp32 y esp8266, 
que están disponibles a través del IDE de Arduino. Sin embargo, con suerte, 
las bibliotecas de red que se utilizan no importarán mucho a la mayoría de los usuarios, 
ya que solo puede incluir painlessMesh.h, ejecutar init() y luego trabajar con la biblioteca a través de la API.

painlessMesh no es una red IP
painlessMesh no crea una red TCP/IP de nodos. Más bien, cada uno de los nodos se identifica de forma única por 
su chipId de 32 bits que se recupera del esp8266/esp32 mediante la system_get_chip_id()llamada en el SDK. Cada 
nodo tendrá un número único. Los mensajes pueden transmitirse a todos los nodos de la malla o enviarse específicamente 
a un nodo individual identificado por su `nodeId.
*/
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_I2CDevice.h> //se pondran al final para ver si no causan conflicto
#include <Adafruit_SSD1306.h>
#include "globales.hpp"

// MESH Details

#define   MESH_PORT       5555 //default port

// Digital pin connected to the DHT sensor

DHT dht(DHTPIN, DHTTYPE);
float min2 = 100;  //temperatura min si no se igual a 100 se va a 0
float max2;  //temperatura maxima

String name;
float temp, hum , tmin, tmax;
//Number for this node
int nId; //numero de identificador de los demas nodos
int id = 2; //numero de identificador de este nodo
String nameNodo = "HGO CTI 4° PISO";  // ***************************************modificar cada vez que se programe

/** 1 es PMCT
 *  2 es CTI -ocupado
 *  3 es COM -ocupado
 *  4 es PTTI2 -ocupado
 *  5 es PTTI
*/

//String to send to other nodes with sensor readings
String readings; //son las lecturas que se enviaran a las otras tarjetas

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings
StaticJsonDocument<220> jsonReadings; //lecturas de este mismo nodo
StaticJsonDocument<249> myObject;
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
//Create tasks: to send messages and get readings;
/**Crea una Task taskSendMessage es una funcion responsable de llamar a
 * sendMessage() cada cinco segundos mientras el programa se está ejecutando.*/
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);
// obtiene lecturas de temperatura, humedad etc y devuelve un String con los datos obtenidos de este mismo nodo
String getReadings () {
  String readings;
  jsonReadings["nId"] = id;
  jsonReadings["name"] = nameNodo;
  jsonReadings["temp"] = dht.readTemperature();
  jsonReadings["hum"] = dht.readHumidity();
  jsonReadings["tmin"] = tempMin();
  jsonReadings["tmax"] = tempMax();
  //La variable se convierten luego en String y se guardan es una variable llamada readings
  serializeJson(jsonReadings,readings);
  return readings;
}
// funcion envia el mensaje por mesh osea a todos los nodos de la red (difusión).
void sendMessage () {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}


// Needed for painless library 
//muestra el contenido que es mandado por cada nodo
//La función imprime el remitente del mensaje (from) y el contenido del mensaje (msg.c_str()).
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  //deserializeJson() funcion que deserealiza en este caso msg y lo vuelve un objeto llamado myObject y arroja un error
  DeserializationError error = deserializeJson(myObject,msg);
  Serial.print("[INFO main.cpp] Deserealizacion -> ");
  Serial.println(error.c_str());
  if(error == DeserializationError::Ok){
    nId = myObject["nId"];
    name = myObject["name"].as<String>();
    temp = myObject["temp"];
    hum = myObject["hum"];
    tmin = myObject["tmin"];
    tmax = myObject["tmax"];
  } 
    Serial.print("nId: ");
    Serial.println(nId);
    Serial.print("nombre del nodo: ");
    Serial.println(name);
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" C");
    Serial.print("Temperature Min: ");
    Serial.print(tmin);
    Serial.println(" C");
    Serial.print("Temperature Max: ");
    Serial.print(tmax);
    Serial.println(" C");
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.println(" %");
  
}
/*La función se ejecuta cada vez que un nuevo nodo se une a la red. Esta función simplemente imprime la identificación 
  del chip del nuevo nodo. Puede modificar la función para realizar cualquier otra tarea.*/
void newConnectionCallback(uint32_t nodeId) { //parece ser un control interno del protocolo mesh
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
  //podriamos agregar un pitido doble
}
/*La función se ejecuta cada vez que cambia una conexión en la red (cuando un nodo se une o sale de la red)*/
void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  //podriamos agregar un pitido simple o modicar o adquirir informacion adicional para ampliar
}

//La función se ejecuta cuando la red ajusta la hora, de modo que todos los nodos estén sincronizados. Imprime el desplazamiento.
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);
  
  dht.begin();


  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  /*Elija los tipos de mensajes de depuración deseados:*/
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  //Inicialice la malla con los detalles definidos anteriormente.
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler );

  //Asigne todas las funciones de devolución de llamada a sus eventos correspondientes.
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  mesh.initStation();
  Serial.println(mesh.getAPIP());
  Serial.println(mesh.getStationIP());
  // userScheduler es responsable de manejar y ejecutar las tareas en el momento adecuado.
  userScheduler.addTask(taskSendMessage);

  //Finalmente, habilite taskSendMessage, para que el programa comience a enviar los mensajes a la malla.
  taskSendMessage.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}