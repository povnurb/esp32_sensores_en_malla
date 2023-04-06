/**https://github.com/gmag11/painlessMesh
 * 1 .- optimizar el programa de tal manera que si se envia por el protocolo esp-now ya no se envie por la malla
 * este sketch es complemento de protocolo_esp_Now que hara de servidor
 * 2.- cambiar el puerto del dht22
 * 3.- cambiar la potencia de float a int
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
#include "temperatura.hpp"
#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_I2CDevice.h> //se pondran al final para ver si no causan conflicto
#include <Adafruit_SSD1306.h>
#include "globales.hpp"
#include <esp_now.h>
#include <esp_wifi.h>
// MESH Details

#define   MESH_PORT       5555 //default port

// Digital pin connected to the DHT sensor

//****************************************************************************************
//MAC Address of the receiver osea de el dispositivo que recibe 
uint8_t broadcastAddress[] = {0x70,0xB8,0xF6,0x5A,0xF1,0x1C};
// Insert your SSID  //0x70,0xB8,0xF6,0x5A,0xF1,0x1C  //40:22:D8:04:36:4C
constexpr char WIFI_SSID[] = "CTRLHGO"; // "INFINITUM37032"//INFINITUM59W1_2.4//INFINITUMD378
//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    String modoSend;
    int id;
    String nameNodo;
    float temp;
    float hum;
    int readingId;
    float min;
    float max;
} struct_message;

//Create a struct_message called myData
struct_message myData; //envio por esp-NOW
struct_message myData2; //envio por la malla
unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 5000;        // Interval at which to publish sensor readings

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}
float getWiFiRsi(const char *ssid2) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid2, WiFi.SSID(i).c_str())) {
            Serial.println(WiFi.RSSI(i));
            return WiFi.RSSI(i);
          }
      }
  }
  return 0;
}
// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
//*************************************************************************
String modoSend;
String name;
int readingId;
//Number for this node
int nId; //numero de identificador de los demas nodos
int id = 1; //numero de identificador de este nodo--------sustituye a board
String nameNodo = "CTIHGO";  // ***************************************modificar cada vez que se programe
bool espnow = true; //envio por espnow
/** 
 *  1 es CTIHGO -ocupado
 *  2 es PMCT -ocupado
 *  3 es PTTI2 -ocupado
 *  4 es PTTI 
*/

//String to send to other nodes with sensor readings
String readings; //son las lecturas que se enviaran a las otras tarjetas

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings
StaticJsonDocument<500> jsonReadings; //lecturas de este mismo nodo
StaticJsonDocument<500> myObject;
StaticJsonDocument<500> jsonSendMesh; //envio por mesh
//funciones para saber la temperatura min y maximas
//Create tasks: to send messages and get readings;
/**Crea una Task taskSendMessage es una funcion responsable de llamar a
 * sendMessage() cada cinco segundos mientras el programa se está ejecutando.*/
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);
// obtiene lecturas de temperatura, humedad etc y devuelve un String con los datos obtenidos de este mismo nodo
String getReadings () {
  String readings;
  jsonReadings["modoSend"] = "MESH";
  jsonReadings["nId"] = id;
  jsonReadings["name"] = nameNodo;
  jsonReadings["temp"] = dht.readTemperature();
  jsonReadings["readingId"] = myData.readingId; 
  jsonReadings["hum"] = dht.readHumidity();
  jsonReadings["tmin"] = tempMin();
  jsonReadings["tmax"] = tempMax();
  //La variable se convierten luego en String y se guardan es una variable llamada readings
  serializeJson(jsonReadings,readings);
  Serial.println("Informacion a enviar por la Malla");
  Serial.println(readings); //si lo quito falla el string
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
    modoSend = myObject["modoSend"].as<String>();
    nId = myObject["nId"];
    name = myObject["name"].as<String>();
    temp = myObject["temp"];
    readingId = myObject["readingId"];
    hum = myObject["hum"];
    tmin = myObject["tmin"];
    tmax = myObject["tmax"];
  } 
    Serial.println("Informacion recivida por la malla:");
    Serial.print("modo: ");
    Serial.println(modoSend);
    Serial.print("nId: ");
    Serial.println(nId);
    Serial.print("nombre del nodo: ");
    Serial.println(name);
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" C");
    Serial.print("potencia: ");
    Serial.println(readingId);
    Serial.print("Temperature Min: ");
    Serial.print(tmin);
    Serial.println(" C");
    Serial.print("Temperature Max: ");
    Serial.print(tmax);
    Serial.println(" C");
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.println(" %");
  //hay que serealizar el objeto para enviar a la malla
    myData2.modoSend = "MESH";
    myData2.id = nId;//BOARD_ID;
    myData2.nameNodo = name;
    myData2.temp = temp;
    myData2.hum = hum;
    myData2.readingId = readingId; //readingId++; //-------------------------------------------------------------
    myData2.min = tmin;
    myData2.max = tmax;

  //Send message via ESP-NOW al esclavo
    esp_err_t result2 = esp_now_send(broadcastAddress, (uint8_t *) &myData2, sizeof(myData2));
    if (result2 == ESP_OK) {
      Serial.println("Sent with success por espnow al servidor");
      Serial.println(myData2.id);
      Serial.println(myData2.nameNodo);
      digitalWrite(SENDOK, HIGH);
    }
    else {
      Serial.println("Error sending the data por espnow al servidor");
      Serial.println(myData2.id);
      Serial.println(myData2.nameNodo);
      digitalWrite(SENDOK, LOW);
    }
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
  WiFi.mode(WIFI_AP_STA);//ver si jala sin esto

  int32_t channel = getWiFiChannel(WIFI_SSID); //obtenemos el canal

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after

  //Init ESP-NOW
  if (!esp_now_init()) {
    //si el resultado es un 0 la conexion ESP-NOW esta ok
    Serial.println("[INFO: espnow.hpp ] Conexion ESP-NOW inicializada.");
  }else{
    Serial.println("[INFO: espnow.hpp ] Conexion ESP-NOW no inicializada.");
    ESP.restart();
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo)); //si no quitar y encontrar la version 1.0.6
  memcpy(peerInfo.peer_addr, broadcastAddress, sizeof(peerInfo.peer_addr));
  peerInfo.encrypt = false;
  
  //Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    //return;   //se quito
  }


  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  /*Elija los tipos de mensajes de depuración deseados:*/
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  //Inicialice la malla con los detalles definidos anteriormente.
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler ,MESH_PORT);

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
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    if(espnow){
      // Save the last time a new reading was published
      previousMillis = currentMillis;
      //Set values to send
      myData.modoSend = "ESPNOW";
      myData.id = id;//BOARD_ID;
      myData.nameNodo = nameNodo;
      myData.temp = dht.readTemperature();
      myData.hum = dht.readHumidity();
      myData.readingId = getWiFiRsi(WIFI_SSID); 
      myData.min = tempMin();
      myData.max = tempMax();
      //Send message via ESP-NOW

      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
      if (result == ESP_OK) {
        Serial.println("Sent with success ESPNOW");
        digitalWrite(SENDOK, HIGH);
        
      }
      else {
        Serial.println("Error sending the data ESPNOW");
        digitalWrite(SENDOK, LOW);
        
      }
    }
  }
  mesh.update();
}