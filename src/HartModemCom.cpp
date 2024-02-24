#include "HartModemCom.h"

/*Permet d'avoir une chaîne de caractère qui représente l'état de la com.*/
char const * HartModemCom::ComStateToString(ComState state)
{
    switch (state)
    {
        case WAIT_MESSAGE_TO_TRANSMIT:
            return "WAIT_MESSAGE_TO_TRANSMIT";
            break;
        case SEND_MESSAGE_TO_DEVICE:
            return "SEND_MESSAGE_TO_DEVICE";
            break;
        case WAIT_DEVICE_REPLY:
            return "WAIT_DEVICE_REPLY";
            break;
        case DEVICE_REPLY_TIMEOUT:
            return "DEVICE_REPLY_TIMEOUT";
            break;
        case READ_DEVICE_REPLY:
            return "READ_DEVICE_REPLY";
            break;
        case RETRANSMIT_DEVICE_REPLY:
            return "RETRANSMIT_DEVICE_REPLY";
            break;
        default:
            return "UNKNOWN STATE !!!";
            break;
    }
}

/*Constructeur.*/
HartModemCom::HartModemCom(){
    isInitialized = false;
    debugSerial = nullptr;
    modemSerial = nullptr;
    startMillisWaitDeviceReply = 0;
    startMillisReadDeviceReply = 0;
    comState = WAIT_MESSAGE_TO_TRANSMIT;
    comStateOld = WAIT_MESSAGE_TO_TRANSMIT;
    config = DEFAULT_HART_MODEM_COM_CONFIG;
}

/*Destructeur.*/
HartModemCom::~HartModemCom(){
    clearRxModemBuffer();
    clearTxModemBuffer();
}

/*Reset le buffer de réception du modem AD5700. Cela n'effecture pas de lecture.*/
void HartModemCom::clearRxModemBuffer(){
    memset(rxModemBuffer, 0, RX_MODEM_MAX_BUFFER_SIZE);
    rxModemBufferCurrentSize = 0;
    rxModemBufferCurrentIndex = 0;
}

/*Reset le buffer d'envoi du modem AD5700. Cela n'effectue pas d'écriture.*/
void HartModemCom::clearTxModemBuffer(){
    memset(txModemBuffer, 0, TX_MODEM_MAX_BUFFER_SIZE);
    txModemBufferCurrentSize = 0;
    txModemBufferCurrentIndex = 0;
}

/*Initialisation de la communication avec un envoi d'un flux de debug sur un port série spécifié.*/
void HartModemCom::init(HardwareSerial * debugSerial, HardwareSerial * modemSerial, HartModemComConfig config){
    this->debugSerial = debugSerial;
    this->debugSerial->begin(9600);
    this->debugSerial->println("HartModemCom debug is enabled !");
    init(modemSerial, config);
}

/*Initialisation de la communication*/
void HartModemCom::init(HardwareSerial * modemSerial, HartModemComConfig config){
    this->config = config;
    this->modemSerial = modemSerial;
    pinMode(this->config.optoPin, OUTPUT);
    pinMode(this->config.rtsPin, OUTPUT);
    clearRxModemBuffer();
    clearTxModemBuffer();
    modemSerial->begin(this->config.modemSerialSpeed, this->config.modemSerialConfig);
    isInitialized = true;    
}

/*
Demande d'écriture d'un message vers le modem AD5700. 
Retourne le nombre d'octets qui seront transmis.
*/
uint32_t HartModemCom::write(char * buffer, const uint32_t size){

    //Si le buffer de transmission n'est pas vide on ne peut pas envoyer le message.
    if(txModemBufferCurrentSize > 0)
    {
        if(debugSerial != nullptr) debugSerial->println("HartModemCom is busy !");
        return 0;
    } 
    if(size > TX_MODEM_MAX_BUFFER_SIZE)
    {
        if(debugSerial != nullptr) debugSerial->println("message too long !");
        return 0;    
    }
    //A ce stade là on a déjà sécurisé le fait que txModemBufferCurrentSize sera toujours inférieur à TX_MODEM_MAX_BUFFER_SIZE pour la méthode run();
    memcpy(txModemBuffer, buffer, size);

    //Permet de déclancher le passage en SEND_MESSAGE.
    txModemBufferCurrentSize = size;
    txModemBufferCurrentIndex = 0;
    return txModemBufferCurrentSize;
}

/*
Rempli le buffer avec les données reçus de l'AD5700.
Il faut avoir appeler la méthode available() pour connaitre la taille du buffer.
*/
void HartModemCom::read(char * buffer){
    if(rxModemBufferCurrentSize > 0)
    {
        memcpy(rxModemBuffer, buffer, rxModemBufferCurrentSize);
    }  
    clearRxModemBuffer();
}

uint32_t HartModemCom::available(){
    return rxModemBufferCurrentSize;
}

/*A appeller le plus fréquement possible*/
void HartModemCom::run(){

    if(comState != comStateOld)
    {
        comStateOld = comState;
        if(debugSerial != nullptr) debugSerial->println(HartModemCom::ComStateToString(comState));      
    }

    if(comState == WAIT_MESSAGE_TO_TRANSMIT && txModemBufferCurrentSize > 0)
    {
        comState = SEND_MESSAGE_TO_DEVICE;
    }
    else if(comState == SEND_MESSAGE_TO_DEVICE && modemSerial->availableForWrite() > 0 && txModemBufferCurrentIndex < txModemBufferCurrentSize)
    {       
        digitalWrite(config.rtsPin, HIGH);
        digitalWrite(config.optoPin, LOW);
        modemSerial->print(txModemBuffer[txModemBufferCurrentIndex++]);    
    }
    else if(comState == SEND_MESSAGE_TO_DEVICE && modemSerial->availableForWrite() >= (SERIAL_TX_BUFFER_SIZE -1))
    {
        digitalWrite(config.rtsPin, LOW);
        digitalWrite(config.optoPin, HIGH);
        clearTxModemBuffer();
        comState = WAIT_DEVICE_REPLY;
        startMillisWaitDeviceReply = millis();
    }
    else if(comState == WAIT_DEVICE_REPLY)
    {      
        if(millis() - startMillisWaitDeviceReply < config.deviceTimeOutMs && modemSerial->available() > 0)
        {
            comState = READ_DEVICE_REPLY;
        }
        else if(millis() - startMillisWaitDeviceReply > config.deviceTimeOutMs && modemSerial->available() == 0)
        {
            comState = DEVICE_REPLY_TIMEOUT;
        }         
    }
    else if(comState == READ_DEVICE_REPLY)
    {   
        if(modemSerial->available() > 0 && (millis() - startMillisReadDeviceReply > 5) && rxModemBufferCurrentIndex < RX_MODEM_MAX_BUFFER_SIZE)
        {            
            //Remplissage du buffer de réception
            startMillisReadDeviceReply = millis();
            rxModemBuffer[rxModemBufferCurrentIndex++] = (char)modemSerial->read();
        }
        else if((modemSerial->available() == 0 && (millis() - startMillisReadDeviceReply > 20)) || rxModemBufferCurrentSize >= RX_MODEM_MAX_BUFFER_SIZE)
        {
            comState = RETRANSMIT_DEVICE_REPLY;
        }
    }
    else if(comState == DEVICE_REPLY_TIMEOUT)
    {        
        comState = WAIT_MESSAGE_TO_TRANSMIT;
    }
    else if(comState == RETRANSMIT_DEVICE_REPLY)
    {     
        //On ne retransmet rien en soit car c'est à l'appelant de faire un read() si les données sont disponibles. 
        //rxModemBufferCurrentSize > 0 va permettre de savoir si des données sont disponibles.
        rxModemBufferCurrentSize = rxModemBufferCurrentIndex + 1;
        if(debugSerial != nullptr)
        {
            debugSerial->print("Buffer size : ");
            debugSerial->println(rxModemBufferCurrentSize);
            debugSerial->print("Buffer data : ");
            for (unsigned int i = 0; i < rxModemBufferCurrentSize; i++)
            {
                debugSerial->print(rxModemBuffer[i]);
            }
            debugSerial->println();
            debugSerial->flush(); //Attention bloquant quand le buffer est plein !
        }  
        comState = WAIT_MESSAGE_TO_TRANSMIT;
    }
}