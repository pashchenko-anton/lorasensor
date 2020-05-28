#include <stdint.h>
#include <string.h>

#define BASE_STATION 0xBF
#define PERIPHERAL_DEVICE 0xDF
#define SIZE_MSG 20
static const uint16_t MY_ID = (uint16_t) 0; 



typedef enum{
	CONNECT = 48,
	DATA_TRANSMIT,
	DATA_REPEAT,
        DATA_END,
        DISCONNECT
} CommandAdress_t;

typedef struct {
	uint8_t idCommand;
	uint8_t typeDevice;
	uint16_t idDevice;
	uint8_t numMsg;
	int8_t bodyMsg[SIZE_MSG];
}Message_s;

	
uint8_t setConnectMsgStruct(Message_s *Msg_s, uint16_t idDevice);

void convertMsgStructToArr(uint8_t arrMsg[], Message_s *Msg_s, uint8_t sizeData);

uint8_t procResData(uint8_t msgSend[], uint8_t *sizeMsgSend, uint8_t msgRes[], uint8_t sizeMsgRes, int16_t dataDevice[], uint32_t* sizeData);

void convertArrToMsgStruct(Message_s *Msg_s, uint8_t arrMsg[], uint8_t sizeBuffer);

uint8_t setTrsansmitMsg(uint8_t tranMsg[], uint8_t command, uint8_t numMsg, int16_t data[], uint32_t* sizeData);
