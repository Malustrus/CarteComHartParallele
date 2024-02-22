#include <Arduino.h>
#include <SPI.h>
#define RCV_SERIAL_MAX_BUFFER_SIZE 2048
#define RTS_SERIAL1 10
#define OPTO_SERIAL1 11
#define DEVICE_REPLY_TIMOUT_MS 8000
enum TaskState{WAIT_MESSAGE, SEND_MESSAGE, WAIT_DEVICE_REPLY, DEVICE_REPLY_TIMOUT, READ_DEVICE_REPLY, RETRANSMIT_DEVICE_REPLY};
char stressTestSend[] = "abababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaabababababababababababababababababababababababababababababababaababa";
byte message[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x80, 0x00, 0x00, 0x82, 0xFF, 0xFF};
unsigned int rcvModem0BufferCount = 0;
unsigned int sendModem0BufferCount = 0;

char rcvBufferModem0[RCV_SERIAL_MAX_BUFFER_SIZE] = {'\0'};
TaskState state, stateOld = WAIT_MESSAGE;
unsigned long startMillis = 0;
unsigned long start = 0;


/*void toto(
    HardwareSerial * serial, 
    TaskState * state, 
    unsigned long * startMillis1, 
    unsigned long * startMillis2, 
    unsigned int * rstPin, 
    unsigned int * optoPin, 
    char * sendBuffer,
    unsigned int * sendBufferLength,
    char * recieveBuffer,
    unsigned int * recieveBufferLength);*/



void setup()
{
    Serial.begin(9600);
    Serial1.begin(1200);
    pinMode(RTS_SERIAL1, OUTPUT);
    pinMode(OPTO_SERIAL1, OUTPUT);

    //Serial1.setTimeout(1500);
}

void loop()
{
    if(state == WAIT_MESSAGE)
    {
        if(Serial.available() > 0)
        {
            while (Serial.available())
            {
                Serial.read();
            }
        state = SEND_MESSAGE;
        }
    }
    else if(state == SEND_MESSAGE)
    {
        Serial.println("SEND_MESSAGE");
        if(Serial1.availableForWrite() > 0 && sendModem0BufferCount < sizeof(message))
        {
            digitalWrite(RTS_SERIAL1, HIGH);
            digitalWrite(OPTO_SERIAL1, LOW);
            Serial1.print(message[sendModem0BufferCount++]);
        }
        else if(Serial1.availableForWrite() >= 63 && sendModem0BufferCount >= sizeof(message))
        {
            digitalWrite(RTS_SERIAL1, LOW);
            digitalWrite(OPTO_SERIAL1, HIGH);
            sendModem0BufferCount = 0;
            state = WAIT_DEVICE_REPLY;
            startMillis = millis();
        }       
    }
    else if(state == WAIT_DEVICE_REPLY)
    {
         Serial.println("WAIT_DEVICE_REPLY");
        if(millis() - startMillis < DEVICE_REPLY_TIMOUT_MS && Serial1.available() > 0)
        {
            state = READ_DEVICE_REPLY;
        }
        else if(millis() - startMillis > DEVICE_REPLY_TIMOUT_MS && Serial1.available() == 0)
        {
            state = DEVICE_REPLY_TIMOUT;
        }         
    }
    else if(state == READ_DEVICE_REPLY)
    {   
        if(Serial1.available() > 0 && (millis() - start > 5) && rcvModem0BufferCount < RCV_SERIAL_MAX_BUFFER_SIZE)
        {            
            start = millis();
            rcvBufferModem0[rcvModem0BufferCount++] = (char)Serial1.read();
        }
        else if((Serial1.available() == 0 && (millis() - start > 20)) || rcvModem0BufferCount >= RCV_SERIAL_MAX_BUFFER_SIZE)
        {
            state = RETRANSMIT_DEVICE_REPLY;
        }
    }
    else if(state == DEVICE_REPLY_TIMOUT)
    {
        Serial.println("DEVICE_REPLY_TIMOUT");
        state = WAIT_MESSAGE;
    }
    else if(state == RETRANSMIT_DEVICE_REPLY)
    {
        Serial.println("RETRANSMIT_DEVICE_REPLY");
        for (unsigned int i = 0; i < rcvModem0BufferCount; i++)
        {
            Serial.print(rcvBufferModem0[i]);
        }
        Serial.flush(); //Pour le test, attention bloquant !
        memset(rcvBufferModem0, 0x00, rcvModem0BufferCount);
        rcvModem0BufferCount = 0;
        state = WAIT_MESSAGE;
    }
}