#include <Arduino.h>
#include <TaskScheduler.h>    
#include <Ethernet.h>

//#include <Streaming.h> pour utiliser l'opérateur << au lien de client.print

#define RCV_SERIAL_BUFFER_SIZE 64
#define HART_RAW_REQUEST_LENGHT 100
#define FACTORY_DEFAULT_IP      {192,168,0,102}
#define FACTORY_DEFAULT_SUBNET  {255,255,255,0}
#define FACTORY_DEFAULT_GATEWAY {0,0,0,0}
#define IPV4_LENGTH 4

void modem0callback();
void modem1callback();
void modem2callback();
void modem3callback();
void hartServerCallback();
void serverListeningLoopCallback();

byte message[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x80, 0x00, 0x00, 0x82, 0xFF, 0xFF};
String hartRequest = String(HART_RAW_REQUEST_LENGHT);
Task hartServerTask(5, TASK_FOREVER, &hartServerCallback);
Task serverListeningLoopTask(500, TASK_FOREVER, &serverListeningLoopCallback);

byte rcvBufferModem0[RCV_SERIAL_BUFFER_SIZE] = {0};
byte rcvBufferModem1[RCV_SERIAL_BUFFER_SIZE] = {0};
byte rcvBufferModem2[RCV_SERIAL_BUFFER_SIZE] = {0};
byte rcvBufferModem3[RCV_SERIAL_BUFFER_SIZE] = {0};
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[IPV4_LENGTH] = FACTORY_DEFAULT_IP;
byte subnet[IPV4_LENGTH] = FACTORY_DEFAULT_SUBNET;
byte gateway[IPV4_LENGTH] = FACTORY_DEFAULT_GATEWAY;


int rcvModem0BufferCount = 0;
int rcvModem1BufferCount = 0;
int rcvModem2BufferCount = 0;
int rcvModem3BufferCount = 0;

bool rcvOkModem0 = false;
bool rcvOkModem1 = false;
bool rcvOkModem2 = false;
bool rcvOkModem3 = false;

bool retransmitModem0 = false;
bool retransmitModem1 = false;
bool retransmitModem2 = false;
bool retransmitModem3 = false;


Task modem0Task(2, TASK_FOREVER, &modem0callback);
Task modem1Task(2, TASK_FOREVER, &modem1callback);
Task modem2Task(2, TASK_FOREVER, &modem2callback);
Task modem3Task(2, TASK_FOREVER, &modem3callback);

Scheduler runner;
EthernetServer hartServer(102);
EthernetClient hartClient;


void SendHartLongAddressCmd(HardwareSerial * serial)
{
    for (size_t i = 0; i < sizeof(message); i++)
    {
        serial->write((byte)message[i]);
    }
    serial->flush();
}

void serverListeningLoopCallback()
{
  /*Gestion du client hart*/
  if(hartServerTask.isEnabled() == false)
  {
    hartClient = hartServer.available();
  }
  if(hartClient && hartServerTask.isEnabled() == false)
  {
    hartServerTask.enable();
    modem0Task.enableDelayed(1000);   
    modem1Task.enableDelayed(1000);   
    modem2Task.enableDelayed(1000);   
    modem3Task.enableDelayed(1000);   
  }
}

void retransmitBuffer(EthernetClient * client, char * modem, byte * buffer, int * count, bool * retransmitFlag)
{
    client->print(modem);
    for (size_t i = 0; i < *count; i++)
    {
        client->print(rcvBufferModem0[i], HEX);
    }
      
    client->println();
    client->flush();
    *retransmitFlag = true;
    *count = 0;
}

void hartServerCallback()
{
  /*GESTION SERVEUR HART*/  
  if (hartClient) 
  {
    /*Le client est connecté*/ 
    if (hartClient.connected()) 
    {           
        /*Le client envoi des données*/ 
        if (hartClient.available()) 
        {
            char clientRequest = hartClient.read();                                          
            if (hartRequest.length() < HART_RAW_REQUEST_LENGHT) 
            {   
                hartRequest.concat(clientRequest);        
            } 
            /*Découpage trame et remplissage des buffers de transmission aux modems*/       
            if(clientRequest == 'q')
            {
                hartClient.flush();
                hartClient.stop();
                hartServerTask.disable();
            }
        }
        //a,b,c,d,0,0,0
        //b,c,d,0,0,0,0
        /*Envoi des trames hart des modem au client*/
        //hartClient.print(0x00, HEX);
        if(rcvOkModem0 == true)
        {
            retransmitBuffer(&hartClient, "M0:", rcvBufferModem0, &rcvModem0BufferCount, &retransmitModem0);
        }
        if(rcvOkModem1 == true)
        {
            retransmitBuffer(&hartClient, "M1:", rcvBufferModem1, &rcvModem1BufferCount, &retransmitModem1);
        }
        if(rcvOkModem2 == true)
        {
            retransmitBuffer(&hartClient, "M2:", rcvBufferModem2, &rcvModem2BufferCount, &retransmitModem2);
        }
        if(rcvOkModem3 == true)
        {
            retransmitBuffer(&hartClient, "M3:", rcvBufferModem3, &rcvModem3BufferCount, &retransmitModem3);
        }
    }
    else
    {    
      hartClient.flush();
      hartClient.stop();
      hartServerTask.disable();
    }     
  }
  else
  {
    hartClient.flush();
    hartClient.stop();
    hartServerTask.disable();
  }
}

void modem0callback()
{
    /*Des données sont disponibles*/
    if(Serial.available()){
        rcvOkModem0 = false;
        char c = Serial.read();
        if(rcvModem0BufferCount < RCV_SERIAL_BUFFER_SIZE)
        {
            rcvBufferModem0[rcvModem0BufferCount] = c;
            rcvModem0BufferCount++;
        }
    }
    else if(retransmitModem0 == true)
    {
        retransmitModem0 = false;
        SendHartLongAddressCmd(&Serial);
    }
    else
    {
        rcvOkModem0 = true;
    }   
}

void modem1callback()
{
    /*Des données sont disponibles*/
    if(Serial1.available()){
        char c = Serial1.read();
        if(rcvModem1BufferCount < RCV_SERIAL_BUFFER_SIZE)
        {
            rcvBufferModem1[rcvModem1BufferCount] = c;
            rcvModem1BufferCount++;
        }
    }
    else
    {
        SendHartLongAddressCmd(&Serial1);
    }
}

void modem2callback()
{
    /*Des données sont disponibles*/
    if(Serial2.available()){
        char c = Serial2.read();
        if(rcvModem2BufferCount < RCV_SERIAL_BUFFER_SIZE)
        {
            rcvBufferModem2[rcvModem2BufferCount] = c;
            rcvModem2BufferCount++;
        }
    }
    else
    {
        SendHartLongAddressCmd(&Serial2);
    }
}

void modem3callback()
{
    /*Des données sont disponibles*/
    if(Serial3.available()){
        char c = Serial3.read();
        if(rcvModem3BufferCount < RCV_SERIAL_BUFFER_SIZE)
        {
            rcvBufferModem3[rcvModem3BufferCount] = c;
            rcvModem3BufferCount++;
        }
    }
    else
    {
        SendHartLongAddressCmd(&Serial3);
    }
}

void setup() 
{
    Serial.begin(1200, SERIAL_8O1);
    Serial1.begin(1200, SERIAL_8O1);
    Serial2.begin(1200, SERIAL_8O1);
    Serial3.begin(1200, SERIAL_8O1);

    runner.init();
    runner.addTask(hartServerTask);
    runner.addTask(modem0Task);
    runner.addTask(modem1Task);
    runner.addTask(modem2Task);
    runner.addTask(modem3Task);
    
    Ethernet.begin(mac, ip, gateway, subnet);    
    hartServer.begin();
    serverListeningLoopTask.enable();
}

void loop()
{
    runner.execute();    
}