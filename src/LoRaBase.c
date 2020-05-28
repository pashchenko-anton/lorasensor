#include "LoRaProtocol.h"

uint8_t lastCommand = DATA_END, _BFNumMsg = 48;
uint16_t _BFidDevice = 0;

void resetBF(void);

uint8_t setConnectMsgStruct(Message_s *Msg_s, uint16_t idDevice){
	
	Msg_s->idCommand = CONNECT;
	Msg_s->typeDevice = BASE_STATION;
	Msg_s->idDevice = idDevice;
	
	lastCommand = CONNECT;
	_BFNumMsg = 30;
	
	return 4; //sizeMsg
}
//

void convertMsgStructToArr(uint8_t arrMsg[], Message_s *Msg_s, uint8_t sizeData){
	if(sizeData > 0){
		arrMsg[0] = Msg_s->idCommand;
		sizeData--;
		if(sizeData > 0){
			arrMsg[1] = Msg_s->typeDevice;
			sizeData--;
			if(sizeData > 1){
				arrMsg[2] = (Msg_s->idDevice>>8);
				arrMsg[3] = Msg_s->idDevice;
				sizeData-=2;
				if(sizeData > 0){
					arrMsg[4] = Msg_s->numMsg;
					sizeData--;
					if(sizeData > 0){
						for(uint8_t i=0; i<sizeData;i++)
						        arrMsg[i+5] = Msg_s->bodyMsg[i];
					}
				}
			}
		}
	}
}

//

uint8_t procResData(uint8_t msgSend[], uint8_t *sizeMsgSend, uint8_t msgRes[], uint8_t sizeMsgRes, int16_t dataDevice[], uint32_t* sizeData){
	uint8_t isMsg = 0;
	Message_s resMsg_s;
	convertArrToMsgStruct(&resMsg_s, msgRes, sizeMsgRes);
	if(_BFidDevice == 0 || _BFidDevice == resMsg_s.idDevice){
		_BFidDevice = resMsg_s.idDevice;
//		if(resMsg_s.idCommand == CONNECT && lastCommand == resMsg_s.idCommand)
		{
			switch(resMsg_s.idCommand)
			{
			        case CONNECT:
					*sizeMsgSend = setConnectMsgStruct(&resMsg_s, _BFidDevice);
					convertMsgStructToArr(msgSend, &resMsg_s, *sizeMsgSend);
					isMsg = 1;
					break;
			        case DATA_TRANSMIT:
					if((resMsg_s.numMsg == 30 || resMsg_s.numMsg > _BFNumMsg) && 
						(lastCommand == CONNECT || lastCommand == resMsg_s.idCommand))
					{
						_BFNumMsg = resMsg_s.numMsg;
						*sizeMsgSend = setTrsansmitMsg(msgSend, DATA_TRANSMIT, _BFNumMsg, 0, 0);
						dataDevice[0] = (msgRes[5] << 8) + msgRes[6];
						dataDevice[1] = (msgRes[7] << 8) + msgRes[8];
//						dataDevice[0] = (resMsg_s.bodyMsg[0] << 8) + resMsg_s.bodyMsg[1];
//						dataDevice[1] = /*(resMsg_s.bodyMsg[2] << 8) + */resMsg_s.bodyMsg[3];
						isMsg = 1;
					}
					break;
				case DATA_END:
					if(lastCommand == DATA_TRANSMIT){
						*sizeMsgSend = setTrsansmitMsg(msgSend, DATA_END, 0, 0, 0);
						resetBF();
						isMsg = 2;
					}
					break;
			        case DISCONNECT:
				        isMsg = 0;
				    break;
			}
		}
	}
	
	return isMsg;
}
//

void convertArrToMsgStruct(Message_s *Msg_s, uint8_t arrMsg[], uint8_t sizeBuffer){
	if(sizeBuffer > 0)
	{
		Msg_s->idCommand = arrMsg[0];
		sizeBuffer--;
		if(sizeBuffer > 0)
		{
			Msg_s->typeDevice = arrMsg[1];
			sizeBuffer--;
			if(sizeBuffer > 1)
			{
				Msg_s->idDevice = (arrMsg[2] << 8) + arrMsg[3];
				sizeBuffer-=2;
				if(sizeBuffer > 0)
				{
					Msg_s->numMsg = arrMsg[4];
					sizeBuffer--;
					if(sizeBuffer > 0)
						for(uint8_t i=0; i < sizeBuffer; i++)
							Msg_s->bodyMsg[i] = arrMsg[i+5];
				}
			}
		}
	}
}
//

uint8_t setTrsansmitMsg(uint8_t tranMsg[], uint8_t command, uint8_t numMsg, int16_t data[], uint32_t* sizeData){
	
	tranMsg[0] = lastCommand = command;
	tranMsg[1] = (uint8_t) BASE_STATION;
	tranMsg[2] = (uint8_t) (_BFidDevice >> 8);
	tranMsg[3] = (uint8_t) _BFidDevice;
	

/*	for(int i = (numMsg-48)*SIZE_MSG/2; (i < sizeData) && (sizeMsg < SIZE_MSG); sizeMsg+=2, i++)
	{
		tranMsg[5+j] = (uint8_t) (data[i]>>8);
		tranMsg[5+j+1] = (uint8_t) (data[i]);
	}
	if(sizeMsg!=0){
		sizeMsg+=5;
	}
	else{
		tranMsg[0] = lastCommand = DATA_END;
		sizeMsg+=4;
	}
*/
	uint8_t sizeMsg = 0;
	if(command == DATA_TRANSMIT){
		tranMsg[4] = numMsg;
		sizeMsg = 5;
	}
	else if(command == DATA_END)
		sizeMsg = 4;
	
	return sizeMsg;
}
//
void resetBF(){
    _BFidDevice = 0;
}
//


