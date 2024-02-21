#include <Arduino.h>
#include <SPI.h>
#define RCV_SERIAL_BUFFER_SIZE 2048
enum TaskState{WAIT_MESSAGE, SEND_MESSAGE, WAIT_DEVICE_REPLY, DEVICE_REPLY_TIMOUT, READ_DEVICE_REPLY, RETRANSMIT_DEVICE_REPLY};

byte message[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x80, 0x00, 0x00, 0x82, 0xFF, 0xFF};
unsigned int rcvModem0BufferCount = 0;
char rcvBufferModem0[RCV_SERIAL_BUFFER_SIZE] = {'\0'};
TaskState state, stateOld = WAIT_MESSAGE;
unsigned long startMillis = 0;
unsigned long start = 0;

void setup()
{
    Serial.begin(9600);
    Serial1.begin(1200);

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
        //RTS et OPTO !
        for (size_t i = 0; i < sizeof(message); i++)
        {
            Serial1.print(message[i], HEX);
        }
        Serial1.flush();
        state = WAIT_DEVICE_REPLY;
        startMillis = millis();
    }
    else if(state == WAIT_DEVICE_REPLY)
    {
         Serial.println("WAIT_DEVICE_REPLY");
        if(millis() - startMillis < 8000 && Serial1.available() > 0)
        {
            state = READ_DEVICE_REPLY;
        }
        else if(millis() - startMillis > 8000 && Serial1.available() == 0)
        {
            state = DEVICE_REPLY_TIMOUT;
        }         
    }
    else if(state == READ_DEVICE_REPLY)
    {
        //Serial.println("READ_DEVICE_REPLY");
        /*char tmp[RCV_SERIAL_BUFFER_SIZE] = {'\0'};
        rcvModem0BufferCount = Serial1.readBytes(tmp, RCV_SERIAL_BUFFER_SIZE);
        strcpy(rcvBufferModem0, tmp);
        Serial.println(rcvModem0BufferCount);
        state = RETRANSMIT_DEVICE_REPLY;*/     
        if(Serial1.available() > 0 && (millis() - start > 5))
        {            
            start = millis();
            rcvBufferModem0[rcvModem0BufferCount] = (char)Serial1.read();
            rcvModem0BufferCount++;
        }
        else if(Serial1.available() == 0 && (millis() - start > 20))
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
        Serial.flush();
        memset(rcvBufferModem0, 0x00, RCV_SERIAL_BUFFER_SIZE);
        rcvModem0BufferCount = 0;
        state = WAIT_MESSAGE;
    }
}