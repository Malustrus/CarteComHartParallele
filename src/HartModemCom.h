#include <Arduino.h>

#ifndef HartModemCom_H
#define HartModemCom_H

#define RX_MODEM_MAX_BUFFER_SIZE 512
#define TX_MODEM_MAX_BUFFER_SIZE 512

#define DEFAULT_HART_MODEM_COM_CONFIG { 7, 6, 8000, 1200, SERIAL_8O2  }

enum ComState{WAIT_MESSAGE_TO_TRANSMIT, SEND_MESSAGE_TO_DEVICE, WAIT_DEVICE_REPLY, DEVICE_REPLY_TIMEOUT, READ_DEVICE_REPLY, RETRANSMIT_DEVICE_REPLY};

struct HartModemComConfig
{
    uint32_t rtsPin;
    uint32_t optoPin;
    uint32_t deviceTimeOutMs;
    uint64_t modemSerialSpeed;
    uint8_t modemSerialConfig;
};

class HartModemCom
{
private:
    HardwareSerial * modemSerial;
    HardwareSerial * debugSerial;
    
    ComState comState;
    ComState comStateOld;

    HartModemComConfig config; 
   
    //Buffer de réception et transmission avec le modem AD5700.
    char rxModemBuffer[RX_MODEM_MAX_BUFFER_SIZE];
    char txModemBuffer[TX_MODEM_MAX_BUFFER_SIZE];
    
    //Représente la taille des données "utile" dans les buffers.
    uint32_t rxModemBufferCurrentSize;
    uint32_t txModemBufferCurrentSize;

    //Variable de position de lecture/ecriture des buffers.
    uint32_t rxModemBufferCurrentIndex;
    uint32_t txModemBufferCurrentIndex;

    //Variables de gestion des timing de vidage/remplissage des buffers.
    uint64_t startMillisWaitDeviceReply;
    uint64_t startMillisReadDeviceReply;

    //Debug
    static char const * ComStateToString(ComState state);

    void clearRxModemBuffer();
    void clearTxModemBuffer();
    bool isInitialized;
public:

    HartModemCom();
    ~HartModemCom();
    void init(HardwareSerial * modemSerial, HartModemComConfig config);
    void init(HardwareSerial * debugSerial, HardwareSerial * modemSerial, HartModemComConfig config);
    uint32_t write(char * buffer, const uint32_t size);
    void read(char * buffer);
    uint32_t available();
    void run();
};

#endif