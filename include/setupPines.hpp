#include <Arduino.h>
void setupPines(){
    pinMode(2,PULLUP); //alarma1
    pinMode(4,PULLUP);
    pinMode(16,PULLUP);
    pinMode(17,PULLUP);
    pinMode(18,PULLUP);
    pinMode(19,PULLUP);
    pinMode(21,PULLUP);
    pinMode(13,PULLUP);//alarma8
    digitalWrite(2,HIGH);
    digitalWrite(4,HIGH);
    digitalWrite(16,HIGH);
    digitalWrite(17,HIGH);
    digitalWrite(18,HIGH);
    digitalWrite(19,HIGH);
    digitalWrite(21,HIGH);
    digitalWrite(13,HIGH);
}