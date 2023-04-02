#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_I2CDevice.h> //se pondran al final para ver si no causan conflicto
#include <Adafruit_SSD1306.h>
// MESH Details
#define   MESH_PREFIX     "CTRLHGO" //nombre de la red malla
#define   MESH_PASSWORD   "Personal" //password de la red malla
#define   MESH_PORT       5555 //default port

// Digital pin connected to the DHT sensor
#define DHTPIN 2  
#define SENDOK 12                                      //GPIO12 envio ok
#define SENDFAIL 15                                     //GPIO15 fallo el envio
#define Interrupcion 35 
#define tiempoDeRebote 250      
// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);
float min2 = 100;  //temperatura min si no se igual a 100 se va a 0
float max2;  //temperatura maxima

int node;
float temp, hum , tmin, tmax;

//Number for this node
int nodeNumber = 1;
/** 1 es Com
 *  2 es CTI
 *  3 es PMCT
 *  4 es PTTI2
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
  jsonReadings["node"] = nodeNumber;
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
  Serial.print("[INFO main.cpp] ");
  Serial.println(error.c_str());
  if(error == DeserializationError::Ok){
    node = myObject["node"];
    temp = myObject["temp"];
    hum = myObject["hum"];
    tmin = myObject["tmin"];
    tmax = myObject["tmax"];
  }
    Serial.print("Node: ");
    Serial.println(node);
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
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
  //podriamos agregar un pitido doble
}
/*La función se ejecuta cada vez que cambia una conexión en la red (cuando un nodo se une o sale de la red)*/
void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  //podriamos agregar un pitido simple
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
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );

  //Asigne todas las funciones de devolución de llamada a sus eventos correspondientes.
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // userScheduler es responsable de manejar y ejecutar las tareas en el momento adecuado.
  userScheduler.addTask(taskSendMessage);

  //Finalmente, habilite taskSendMessage, para que el programa comience a enviar los mensajes a la malla.
  taskSendMessage.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}