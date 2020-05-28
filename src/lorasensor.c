/**
 * Author: Dragino 
 * Date: 16/01/2018
 * 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.dragino.com
 *
 * 
*/

#include <signal.h>		/* sigaction */
#include <errno.h>		/* error messages */

#include "radio.h"
#include "LoRaProtocol.h"

#define TX_MODE 0
#define RX_MODE 1
#define RADIOHEAD 1

#define RADIO1    "/dev/spidev1.0"
#define RADIO2    "/dev/spidev2.0"

typedef enum{
    TEMPER = 1,
    PRESS1,
    PRESS7
}Sensor_t;


static char ver[8] = "0.1";

/* lora configuration variables */
static char sf[8] = "7";
static char bw[8] = "125000";
static char cr[8] = "5";
static char wd[8] = "72";
static char prlen[8] = "8";
static char power[8] = "20";
static char freq[16] = "868000000";            /* frequency of radio */
static char radio[16] = RADIO2;
static char filepath[32] = {'\0'};
static int mode = RX_MODE;
static int payload_format = 0; 
static int device = 50;
static bool getversion = false;

static char radioTx[16] = RADIO1;

/* signal handling variables */
volatile bool exit_sig = false; /* 1 -> application terminates cleanly (shut down hardware, close open files, etc) */
volatile bool quit_sig = false; /* 1 -> application terminates without shutting down the hardware */

/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

static void sig_handler(int sigio) {
    if (sigio == SIGQUIT) {
	    quit_sig = true;;
    } else if ((sigio == SIGINT) || (sigio == SIGTERM)) {
	    exit_sig = true;
    }
}

void print_help(void) {
    printf("Usage: my_lora	       without argument");
    printf("                           [-T] set as temperature sensor\n");
    printf("                           [-P] set as pressure sensor (1: DM5001E, 7:DM5007A)\n");
    printf("                           [-h] show this help and exit \n");
}

int DEBUG_INFO = 0;       

/* -------------------------------------------------------------------------- */
/* --- MAIN FUNCTION -------------------------------------------------------- */

int main(int argc, char *argv[])
{

    struct sigaction sigact; /* SIGQUIT&SIGINT&SIGTERM signal handling */
	
    int c, i;

    char input[128] = {'\0'};

    FILE *fp = NULL;
    uint16_t sensor = 0, divValue = 1000;

    // Make sure only one copy of the daemon is running.
    //if (already_running()) {
      //  MSG_DEBUG(DEBUG_ERROR, "%s: already running!\n", argv[0]);
      //  exit(1);
    //}

    while ((c = getopt(argc, argv, "TP:")) != -1) {
        switch (c) {
	    case 'T':
		sensor = TEMPER;
		divValue = 10;
		break;
	    case 'P':
		if(*optarg == '7'){
		    sensor = PRESS7;
		    divValue = 160;
		}
		else
		    sensor = PRESS1;
		break;
            case 'h':
            case '?':
            default:
                print_help();
                exit(0);
        }
    }

	
    if (getversion) {
		printf("lg02_single_rx_tx ver: %s\n",ver);
        exit(0);
    }	

	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    /* radio device SPI_DEV init */
    radiodev *loradev, *loradevTx;
    loradev = (radiodev *) malloc(sizeof(radiodev));
    loradevTx = (radiodev *) malloc(sizeof(radiodev));


        loradevTx->nss = 15;
	loradevTx->rst = 8;
	loradevTx->dio[0] = 7;
	loradevTx->dio[1] = 6;
	loradevTx->dio[2] = 0;
	strncpy(radioTx, RADIO1, sizeof(radio));


	loradev->nss = 24;
	loradev->rst = 23;
	loradev->dio[0] = 22;
	loradev->dio[1] = 20;
	loradev->dio[2] = 0;
	strncpy(radio, RADIO2, sizeof(radio));	


    loradev->spiport = lgw_spi_open(radio);
    loradevTx->spiport = lgw_spi_open(radioTx);

    if (loradev->spiport < 0) { 
        printf("opening %s error!\n",radio);
        goto clean;
    }
    if (loradevTx->spiport < 0) {
	printf("opening %s error!\n",radioTx);
	goto clean;
    }

    loradev->freq = atol(freq);
    loradev->sf = atoi(sf);
    loradev->bw = atol(bw);
    loradev->cr = atoi(cr);
    loradev->power = atoi(power);
    loradev->syncword = atoi(wd);
    loradev->nocrc = 1;  /* crc check */
    loradev->prlen = atoi(prlen);
    loradev->invertio = 0;
    strcpy(loradev->desc, "RFDEV");	

    loradevTx->freq = atol(freq);
    loradevTx->sf = atoi(sf);
    loradevTx->bw = atol(bw);
    loradevTx->cr = atoi(cr);
    loradevTx->power = atoi(power);
    loradevTx->syncword = atoi(wd);
    loradevTx->nocrc = 1;  /* crc check */
    loradevTx->prlen = atoi(prlen);
    loradevTx->invertio = 0;
    strcpy(loradevTx->desc, "RFDEV");

    MSG("Radio struct: spi_dev=%s, spiport=%d, freq=%ld, sf=%d, bw=%ld, cr=%d, pr=%d, wd=0x%2x, power=%d\n", radio, loradev->spiport, loradev->freq, loradev->sf, loradev->bw, loradev->cr, loradev->prlen, loradev->syncword, loradev->power);

    if(!get_radio_version(loradev))  
        goto clean;

    /* configure signal handling */
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = sig_handler;
    sigaction(SIGQUIT, &sigact, NULL); /* Ctrl-\ */
    sigaction(SIGINT, &sigact, NULL); /* Ctrl-C */
    sigaction(SIGTERM, &sigact, NULL); /* default "kill" command */

    setup_channel(loradev);
    setup_channel(loradevTx);

    if ( mode == TX_MODE ){
		uint8_t payload[256] = {'\0'};
		uint8_t payload_len;

		if (strlen(input) < 1)
			strcpy(input, "HELLO DRAGINO");	

		if ( payload_format == RADIOHEAD ) {
			input[strlen((char *)input)] = 0x00;
			payload_len = strlen((char *)input) + 5;			
			payload[0] = 0xff;
			payload[1] = 0xff;
			payload[2] = 0x00;
			payload[3] = 0x00;
			for (int i = 0; i < payload_len - 4; i++){
				payload[i+4] = input[i];
			}
		}
		else {
			snprintf(payload, sizeof(payload), "%s", input);
			payload_len = strlen((char *)payload);
		}

		printf("Trasmit Data(ASCII): %s\n", payload);
		printf("Trasmit Data(HEX): ");
		for(int i = 0; i < payload_len; i++) {
            printf("%02x", payload[i]);
        }
		printf("\n");
		single_tx(loradev, payload, payload_len);
    } else if ( mode == RX_MODE){

        struct pkt_rx_s rxpkt;

        FILE * chan_fp = NULL;
        char tmp[256] = {'\0'};
        char chan_path[32] = {'\0'};
        char *chan_id = NULL;
        char *chan_data = NULL;
        static int id_found = 0;
        static unsigned long next = 1, count_ok = 0, count_err = 0;
        int i, data_size;

	uint8_t bufferSend[256];
	uint8_t	bufferSendSize;

        rxlora(loradev, RXMODE_SCAN);

        if (strlen(filepath) > 0) 
            fp = fopen(filepath, "w+");
	int stopFlag = 0;

        MSG("\nListening at SF%i on %.6lf Mhz. port%i\n", loradev->sf, (double)(loradev->freq)/1000000, loradev->spiport);
        fprintf(stdout, "REC_OK: %d,    CRCERR: %d\n", count_ok, count_err);

	struct timeval currentTime;
	uint32_t timerTransmit;
	uint8_t isNeedSend = 0, numSenderMsg = 0;
	const uint32_t dataResSize = 2;
	int16_t dataRes[dataResSize];

	while (!exit_sig && !quit_sig) {
            if(digitalRead(loradev->dio[0]) == 1) {
                memset(rxpkt.payload, 0, sizeof(rxpkt.payload));
                if (received(loradev->spiport, &rxpkt)) {
		    uint8_t isDevice = rxpkt.payload[1];
		    if(isDevice != 0xBF){
			data_size = rxpkt.size;

			memset(tmp, 0, sizeof(tmp));

			for (i = 0; i < rxpkt.size; i++) {
			    tmp[i] = rxpkt.payload[i];
			}


////--received data processing function---///
			uint8_t res = procResData(bufferSend, &bufferSendSize, tmp, data_size, dataRes, dataResSize);
			if( res == 1){
			    isNeedSend = 1; //flag for send message
			    timerTransmit = 0;
			    numSenderMsg = 0;
			}
			else{			    
			    //if END_DATA
			    if(res == 2){
				isNeedSend = 1; //flag for send message
				timerTransmit = 0;
				numSenderMsg = 0;
				switch(dataRes[1]){
				    case (0xA * 1000):
					fprintf(stdout, "Limit!\n");
					break;
				    case (0xB * 1000):
					fprintf(stdout, "Disconnect!\n");
					break;
				    default:
					switch(sensor){
					    case TEMPER:
						fprintf(stdout, "current: %.2f mA, temper: %.1f\n", (float) dataRes[0]/100, (float) dataRes[1]/divValue);
						break;
					    case PRESS1:
						fprintf(stdout, "current: %.2f mA, pressure: %.3f\n", (float) dataRes[0]/100, (float) dataRes[1]/divValue);
						break;
					    case PRESS7:
						fprintf(stdout, "current: %.2f mA, pressure: %.3f\n", (float) dataRes[0]/100, (float) dataRes[1]/1000*divValue);
						break;
					    default:
						fprintf(stdout, "current: %.2f mA, normal current: %.3f\n", (float) dataRes[0]/100, (float) dataRes[1]/divValue);
						break;
					    }
					break;
				}
				fprintf(stdout, "RSSI: %.0f dBm\n\n", rxpkt.rssi);
			    }
			    else
				//other variants
				isNeedSend = 0;
			}
////------------------------------/////


    ///WF maybe delete
//			if(tmp[1] == 0xDF){
//			    isNeedSend = 1;

//			}

			if (fp) {
			    fprintf(fp, "%s\n", rxpkt.payload);
			    fflush(fp);
			}

			if (tmp[2] == 0x00 && tmp[3] == 0x00) /* Maybe has HEADER ffff0000 */
			     chan_data = &tmp[4];
			else
			     chan_data = tmp;

			chan_id = chan_data;   /* init chan_id point to payload */

			/* the format of payload maybe : <chanid>chandata like as <1234>hello  */
			for (i = 0; i < 16; i++) { /* if radiohead lib then have 4 byte of RH_RF95_HEADER_LEN */
			    if (tmp[i] == '<' && id_found == 0) {  /* if id_found more than 1, '<' found  more than 1 */
				chan_id = &tmp[i + 1];
				++id_found;
			    }

			    if (tmp[i] == '>') {
				tmp[i] = '\0';
				chan_data = tmp + i + 1;
				data_size = data_size - i;
				++id_found;
			    }

			    if (id_found == 2) /* found channel id */
				break;

			}

			if (id_found == 2)
			    sprintf(chan_path, "/var/iot/channels/%s", chan_id);
			else {
			    srand((unsigned)time(NULL));
			    next = next * 1103515245 + 12345;
			    sprintf(chan_path, "/var/iot/channels/%ld", (unsigned)(next/65536) % 32768);
			}

			id_found = 0;  /* reset id_found */

			chan_fp  = fopen(chan_path, "w+");
			if ( NULL !=  chan_fp ) {
			    //fwrite(chan_data, sizeof(char), data_size, fp);
			    fprintf(chan_fp, "%s\n", chan_data);
			    fflush(chan_fp);
			    fclose(chan_fp);
			} else
			    fprintf(stderr, "ERROR~ canot open file path: %s\n", chan_path);
    //////////////////////////////
			//---------write to console-----------//

//			fprintf(stdout, "Received: %s\n", chan_data);
//			fprintf(stdout, "RSSI: %.0f dBm\n", rxpkt.rssi);
//			fprintf(stdout, "count_OK: %d, count_CRCERR: %d\n", ++count_ok, count_err);
			//-----------------------------------//

    //		    uint8_t isDevice = chan_data[1];
    //		    if(isDevice != 0xBF && !stopFlag){
    //			uint8_t testData[4]={48,0xBF,1,2};
    //			single_tx(loradevTx, testData, 4);
    //			stopFlag = 1;
    //		    }
    //		    fprintf(stdout, "Is Ok!");
		    }
                } else                                             
                    fprintf(stdout, "REC_OK: %d, CRCERR: %d\n", count_ok, ++count_err);

            }

	    //function send
	    if(isNeedSend){
		gettimeofday(&currentTime, NULL); // get current time
		if(currentTime.tv_sec == timerTransmit || timerTransmit == 0)
		    if(numSenderMsg < 4){
			single_tx(loradevTx, bufferSend, bufferSendSize); // transmit data
//			gettimeofday(&currentTime, NULL);
			timerTransmit = currentTime.tv_sec + 5; //current time in sec + 5
			numSenderMsg++;
		    }
		    else{
			numSenderMsg = isNeedSend = 0;
		    }
	    }
        }

    }

clean:
    if(fp)
        fclose(fp);

    free(loradev);
    free(loradevTx);
	
    MSG("INFO: Exiting %s\n", argv[0]);
    exit(EXIT_SUCCESS);
}
