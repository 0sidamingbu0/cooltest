#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <wiringPi.h>
#include <softPwm.h>
#include <wiringSerial.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <string.h>
#include "json.h"
#include <time.h>
#include <stddef.h>
#include <curl.h>
#include <unistd.h>

#include   <sys/ioctl.h> 
#include   <sys/socket.h> 
#include   <netinet/in.h> 
#include   <net/if.h> 

#include <sys/syscall.h>  

char * softversion = "2018071600";
char   returnstr[200]={0};
char hostname[50]={0};
char startTime[50]={0};
char mac[6];

int mutex_flag = 0;

int working = 0;

uint8_t hardwareVersion = 0;
uint8_t firmwareVersion = 0;
uint8_t channel = 0;

int data_start[200];
int data_last[200];
int data_stop[200];
int max_start = 0;
int max_last = 0;
uint8_t pinstatus[50]={0};

#define JDQ1     			 8//gpio p:3
#define	JDQ2     			 9//gpio p:5
#define JDQ3     			 7//gpio p:7
#define JDQ4     			 0//gpio p:11
#define JDQ5     			 2//gpio p:13
#define JDQ6     			 3//gpio p:15
#define JDQ7     			 12//gpio p:19
#define JDQ8    			 13//gpio p:21
#define JDQ9    			 14//gpio p:23
#define JDQ10    			 30//gpio p:27
#define JDQ11    			 21//gpio p:29
#define JDQ12     			 22//gpio p:31
#define JDQ13     			 23//gpio p:33
#define JDQ14     			 24//gpio p:35
#define JDQ15     			 25//gpio p:37
#define JDQ16     			 29//gpio p:40
#define JDQ17     			 28//gpio p:38
#define JDQ18     			 27//gpio p:36
#define JDQ19     			 26//gpio p:32
#define JDQ20     			 31//gpio p:28
#define JDQ21     			 11//gpio p:26
#define JDQ22     			 10//gpio p:24
#define JDQ23     			 6//gpio p:22
#define JDQ24     			 5//gpio p:18
#define JDQ25     			 4//gpio p:16
//#define JDQ26     			 1//gpio p:12
//#define JDQ27     			 15//gpio p:10
//#define JDQ28     			 16//gpio p:8
//#define JDQ29     			 //gpio p:
//#define JDQ30     			 //gpio p:

#define SH_CP 				 15 //DS IN  L -> H
#define ST_CP 				 16 // OUT   L -> H
#define DS				 	 1



typedef unsigned char   uint8_t;     //?¡ª ??|?¡¤8????¡ã
typedef unsigned short uint16_t;  

uint8_t jdq[30]={8,9,7,0,2,3,12,13,14,30,21,22,23,24,25,29,28,27,26,31,11,10,6,5,4,1,15,16,0,0};
void ctrl_jdq(uint8_t,uint8_t);

#define WW_STATE      0x55
#define AA_STATE      0xaa
#define CMD_STATE1     0x02
#define LEN_STATE      0x03
#define DATA_STATE     0x04
#define FCS_STATE      0x05

#define PORT            8888
#define PORT_CLIENT     10080

#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     3000
#define MAXANSWERSIZE   512
#define GET             0
#define POST            1


struct   ifreq   ifreq; 
int   sock; 



int usart_fd;
  
uint8_t state = WW_STATE;
uint8_t  LEN_Token;
uint8_t  FSC_Token;
uint8_t  tempDataLen = 0;
uint8_t rxbuf[200];
uint8_t len_global=0;
int led_state = 0;


time_t now;
struct tm *tblock; 

CURL* posturl;
FILE *sp;

void send_usart(uint8_t *data,uint8_t len);

void printf_file(char * str);



typedef struct
{
  uint8_t data[100];
  uint8_t len;
} MXJFIFO;

#define FIFO_SIZE 100
MXJFIFO fifo[FIFO_SIZE];

uint8_t fifo_size = 0;
uint8_t fifo_jinwei = 0;
uint8_t fifo_start = 0;
uint8_t fifo_end = 0;


int post_type;

struct connection_info_struct
{
  int connectiontype;
  char *answerstring;
  struct MHD_PostProcessor *postprocessor;
};

const char *askpage = "{\"code\":\"0\",\"message\":\"success\"}";

const char *greetingpage =
  "<html><body><h1>Welcome, %s!</center></h1></body></html>";

const char *errorpage =
  "<html><body>This doesn't seem to be right.</body></html>";

const char *idolpage ;


long writer(void *data, int size, int nmemb, char *content)
{
	return 1;
}

static int
send_page (struct MHD_Connection *connection, const char *page)
{
	
  int ret;
  struct MHD_Response *response;


  response =
    MHD_create_response_from_buffer (strlen (page), (void *) page,
				     MHD_RESPMEM_PERSISTENT);
  if (!response)
    return MHD_NO;

  //MHD_add_response_header (response, "Content-Type", "text/html;charset=UTF-8");

  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}


static int
iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
              const char *filename, const char *content_type,
              const char *transfer_encoding, const char *data, uint64_t off,
              size_t size)
{
	
  struct connection_info_struct *con_info = coninfo_cls;
  if(size == 0) return MHD_NO;
	
  return MHD_YES;
}



static void
request_completed (void *cls, struct MHD_Connection *connection,
                   void **ptr, enum MHD_RequestTerminationCode toe)
{
 // struct connection_info_struct *con_info = *con_cls;
 // if (NULL == con_info)
 //   return;

  //if (con_info->connectiontype == POST)
  //  {
  //    MHD_destroy_post_processor (con_info->postprocessor);
   // }
    //if(!con_info)
  		//free (con_info);
  //*con_cls = NULL;
	
}


static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **ptr)
{
	//printf("The ID of this thread is: %ld\n", (long int)syscall(224)); 
	while(mutex_flag==1);
	mutex_flag = 1;
	//struct connection_info_struct *con_info = *con_cls;
int i=-1;
int len2 =0;

static int dummy;
char *ToUserName;
char *CreateTime;
char *FromUserName;

char *re_body;
  
  //printf ("====New %s request for %s using version %s\n", method, url, version);
 
  time(&now);
  tblock = localtime(&now);

if (&dummy != *ptr)
    {
      /* The first time only the headers are valid,
         do not respond in the first round... */
      *ptr = &dummy;
      mutex_flag = 0;
      return MHD_YES;
    }
  


  if (0 == strcmp (method, "GET"))
    {	
    	//int len=0;
    	gethostname(hostname,50);
    	//hostname[len]='\0';
		if(ioctl(sock,SIOCGIFADDR,&ifreq)<0) 
		{   //这里涉及ioctl函数对于网络文件的控制（下面会介绍）
		    printf("Ip Not set\n");
		}
		sprintf(returnstr,"[%02x:%02x:%02x:%02x:%02x:%02x] [%d.%d.%d.%d] Channel=[%d] HardwareVersion=[%d] FirmVersion=[%d] SoftwareVersion=[%s] hostname=[%s] startTime=%s\n\0",
			(unsigned   char)mac[0], (unsigned   char)mac[1],  (unsigned   char)mac[2], (unsigned   char)mac[3], (unsigned   char)mac[4], (unsigned   char)mac[5],
			(unsigned char)ifreq.ifr_addr.sa_data[2],(unsigned char)ifreq.ifr_addr.sa_data[3],(unsigned char)ifreq.ifr_addr.sa_data[4],(unsigned char)ifreq.ifr_addr.sa_data[5],
			channel,hardwareVersion,firmwareVersion,softversion,hostname,startTime);
		mutex_flag = 0;
		return send_page (connection, returnstr);
    }
  
  //printf("method= %s\n", method);

  if (0 == strcmp (method, "POST"))
  	{  		
	  int ret;
		
	  
	  if (*upload_data_size != 0)
	  {

		
		
		const char* length = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_LENGTH); 	
			const char* body = MHD_lookup_connection_value (connection, MHD_POSTDATA_KIND, NULL);	

			//printf("length=%s\n",length);
			//printf("body=%s\n",body);
			//printf("url=%s\n",url); 
			if(length != NULL && body != NULL)
			{		  
				len2 = atoi(length);
				//printf("len2=%d\n",len2); 
				re_body = (uint8_t*)calloc(len2,sizeof(uint8_t)+8);
				strncpy(re_body,body,len2);
				re_body[len2]='\0';
				printf("[%d-%d-%d %d:%d:%d] POST RECIEVE len=%d url=%s version=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len2, url, version,body);
			}
		
			if ((sp = fopen("/var/log/idol.log","a+")) != NULL)
			{
				fprintf(sp,"[%d-%d-%d %d:%d:%d] POST RECIEVE len=%d url=%s version=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len2, url, version,body);
				fclose(sp);
			}
			//printf("len2= %d\n",len2);
			if(len2 != 0)
			{
				json_object *my_object = json_tokener_parse(re_body);
				
				free(re_body);
				
				if(json_object_to_json_string(my_object) != NULL && (0 != strcmp (json_object_to_json_string(my_object), "null")))
				{
				  uint16_t address=0;
				  uint8_t index=0,resourceSum=0,commandint=255,commandData=0;
				  const char * command;
				  const char * deviceType;
				  post_type = 1;
				  
				  json_object *Jaddress = NULL;
				  json_object_object_get_ex(my_object, "address",&Jaddress);
				  json_object *Jindex = NULL;
				  json_object_object_get_ex(my_object, "index",&Jindex);
				  json_object *Jcommand = NULL;
				  json_object_object_get_ex(my_object, "command",&Jcommand);
				  json_object *JcommandData = NULL;
				  json_object_object_get_ex(my_object, "data",&JcommandData);
				  json_object *JresourceSum = NULL;
				  json_object_object_get_ex(my_object, "resourceSum",&JresourceSum);
				  json_object *JdeviceType = NULL;
				  json_object_object_get_ex(my_object, "deviceType",&JdeviceType);
				  
				  address = (uint16_t)json_object_get_int(Jaddress);
				  index = (uint16_t)json_object_get_int(Jindex);
				  command = json_object_to_json_string(Jcommand);
				  commandData = (uint16_t)json_object_get_int(JcommandData);
				  resourceSum = (uint16_t)json_object_get_int(JresourceSum);
				  deviceType = json_object_to_json_string(JdeviceType);
				  //printf("%s",deviceType);
				  //printf("address=%d\n", address);
				  //printf("index=%d\n", index);
				  //printf("command=%s\n", command);
				  //printf("resourceSum=%d\n", resourceSum);
				  //printf("recieve post JcommandData= %s\n", json_object_to_json_string(JcommandData));
				  	int i;
					
					if(working == 1)
		  			{
		  				//working = 1;
		  			}
		  			else
		  			{
		  				max_start = 0;
		  				max_last = 0;

						for(i=0; i < json_object_array_length(JcommandData); i++)
						{
							json_object *obj = json_object_array_get_idx(JcommandData, i);
							json_object *obj_value = NULL;
					  		json_object_object_get_ex(obj, "start",&obj_value);
							data_start[i] = (int)json_object_get_int(obj_value);

							json_object *obj2 = json_object_array_get_idx(JcommandData, i);
							json_object *obj_value2 = NULL;
					  		json_object_object_get_ex(obj2, "last",&obj_value2);
							data_last[i] = (int)json_object_get_int(obj_value2);
							//printf("JcommandData[%d]=%s\n", i, json_object_to_json_string(obj));
								

							data_stop[i] = data_start[i] + data_last[i];

							if(max_start < data_start[i])
							{
								max_start = data_start[i];
							}

							if(max_last < data_last[i])
							{
								max_last = data_last[i];
							}
							
							printf("data_start[%d]=%d  data_last[%d]=%d data_stop[%d]=%d \n", i, data_start[i],i,data_last[i],i,data_stop[i]);
						}	

						working = 1;
					}							
		  			

		  			mutex_flag = 0;
		  			//MXJ_SendIrCode(address,data,json_object_array_length(JcommandData));
		  			mutex_flag = 1;	

				  	if(address == 0xfffe)
				  	{
				  		if(0 == strcmp (url, "/start"))
					  	{
					  		
					  		if(0 == strcmp (command, "\"On\""))
					  		{
					  			
					  			
					  			mutex_flag = 0;
					  			//alarm_sendState(1,1);
					  			mutex_flag = 1;
					  		}
					  	
					  	}
				  	}
				  	
				}				
			}
		
		*upload_data_size = 0;
		mutex_flag = 0;
		return MHD_YES;
	  }
	  else if(post_type == 2)
	  {
	  	
	  		*ptr = NULL; /* clear context pointer */
	  	mutex_flag = 0;
	  	  return send_page (connection,askpage);
	  }
	  else if(post_type == 1)
	  {
	  	
	  	*ptr = NULL; /* clear context pointer */
	  	mutex_flag = 0;
		  return send_page (connection,askpage);
	  }
	  else
	  {
	  	
	  	*ptr = NULL; /* clear context pointer */
	  	mutex_flag = 0;
		  return send_page (connection, askpage);  
	  }
  	}
		
	*ptr = NULL; /* clear context pointer */
		mutex_flag = 0;
  return send_page (connection, errorpage);
}



uint8_t fifo_add(uint8_t *data,uint8_t len)
{
	int i;
	while(mutex_flag==1);
	mutex_flag = 1;
	if ((sp = fopen("/var/log/idol.log","a+")) != NULL)
	{
		fprintf(sp,"[%d-%d-%d %d:%d:%d] USART SEND len=%d data=", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len);
		for(i=0;i<len;i++)
		  {
		    fprintf(sp,"%02x ",data[i]);
		  }
		  fprintf(sp,"\n");
		  fclose(sp);
	}

		for(i=0;i<len;i++)
	{
		serialPutchar(usart_fd,data[i]);
		//printf("%02x ",txbuf[i]);
	}

	
	
	mutex_flag = 0;
	
  return 1;
}



void send_usart(uint8_t *data,uint8_t len) //id,state1,state2,state3 1=?a,0=1?,2=¡À¡ê3?
{
  uint8_t txbuf[100];
  uint8_t i=0;
  uint8_t crc = 0;
  txbuf[0] = 0x55;
  txbuf[1] = 0xaa;
  
  led_state = 0;
  for(i=0;i<len;i++)
  {
    txbuf[i+2] = data[i];
    crc += data[i];
  }
  txbuf[len+2] = crc;
  //printf("\n");
  //printf("fifo_add %d:\n",len + 3);

  fifo_add(txbuf,len+3);
  
}




//========thread start=========//
//AA C0 IDH IDL STATE1 STATE2 STATE3 REV CRC AB


void recieve_usart(uint8_t *rx,uint8_t len)
{
	while(mutex_flag==1);
	mutex_flag = 1;
	int i=0,j=0;
	int id=0,cid=0;
	uint8_t endpoint = 1;
	char eventDataStr[50];
	uint32_t eventData32 = 0;
	
	led_state = 0;
	time(&now);
	tblock = localtime(&now);
	//printf("\n");
	//printf("recieved:%d - at: %d-%d-%d %d:%d:%d\n",len,tblock->tm_year, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
	printf("USART RECIEVE:time=%d-%d-%d %d:%d:%d len=%d data=", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len);
	for(i=0;i<len;i++)
	  printf("%02x ",rx[i]);
	printf("\n");

	posturl = curl_easy_init();
		struct curl_slist *list = NULL;

	if(posturl){

		list = curl_slist_append(list, "Content-Type: application/json");
		curl_easy_setopt(posturl, CURLOPT_HTTPHEADER, list);
		curl_easy_setopt(posturl, CURLOPT_URL, "127.0.0.1:10080/api/devices");
		curl_easy_setopt(posturl, CURLOPT_HTTPPOST, 1L);
		curl_easy_setopt(posturl, CURLOPT_TIMEOUT, 1);
		//curl_easy_setopt(posturl, CURLOPT_WRITEFUNCTION, writer);
		//curl_easy_setopt(posturl, CURLOPT_WRITEDATA, &headerStr);
	}
	else
	{
		return;
	}
  
  

  if ((sp = fopen("/var/log/idol.log","a+")) != NULL)
	{
		fprintf(sp,"[%d-%d-%d %d:%d:%d] USART RECIEVE len=%d data=", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len);
		for(i=0;i<len;i++)
    		fprintf(sp,"%02x ",rx[i]);
		fprintf(sp,"\n");
		fclose(sp);
	}
  

//USART RECIEVE:time=2016-8-19 17:14:24 len=23 data=01 16 27 1a 00 0c 01 06 01 08 00 1b 73 7c 00 12 4b 00 07 dc 27 58 89
//POST SEND:time=2016-8-19 17:14:24 url=127.0.0.1:10080/device/API/command body={"address":"10010","indaddressex":"1","event":"ReportCardId","linkQuality":"137"}

//USART RECIEVE:time=2016-8-19 17:15:18 len=23 data=01 16 27 1a 00 0c 01 06 01 09 02 00 01 06 00 12 4b 00 07 dc 27 58 94
//POST SEND:time=2016-8-19 17:15:18 url=127.0.0.1:10080/device/API/command body={"address":"10010","indaddressex":"1","event":"ReportPassword","linkQuality":"148"}
 
 
  if(len>=4)
	{
		id=rx[2];
		id <<= 8;
		id |= rx[3];
	}

  if(len>=6)
	{
		cid=rx[4];
		cid <<= 8;
		cid |= rx[5];
	}
  
  endpoint = rx[len - 10];


  switch(rx[0])
  {
  	case 0x55://command
	{
		if(rx[1]==4)
		{
			hardwareVersion = rx[2];
			firmwareVersion = rx[3];
			channel = rx[4];
		}
		break;
	}

    


    case 1:
	{
		if(len == 18)
		{
			char str[200]={0};
			char str_url[200]={0};
			sprintf(str,"{\"address\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
			sprintf(str_url,"127.0.0.1:%d/device/API/feedback/ping",PORT_CLIENT);
			curl_easy_setopt(posturl, CURLOPT_URL, str_url);
			curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(posturl);
			printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
			if ((sp = fopen("/var/log/idol.log","a+")) != NULL)
			{
				fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
				  fclose(sp);
			}    
		}

		
    break;
	}
 
  }

  curl_easy_cleanup(posturl);

  mutex_flag = 0;
}

void thread(void)
{
	uint8_t ch = 0;
	uint8_t i = 0;
  uint8_t temp2=0;
  uint16_t id=0;
  
	while (1)
	{		
		ch = serialGetchar(usart_fd);
   //printf("recieve ch=%02x\n",ch);
		 switch (state)
    {
      case WW_STATE:
        if (ch == 0x55)
          state = AA_STATE;
          //printf("55");
        break;
        
      case AA_STATE:
        if (ch == 0xaa)
          state = CMD_STATE1;
        else
          state = WW_STATE;
        break;
        
      case CMD_STATE1:
        rxbuf[0] = ch;
        state = LEN_STATE;
        break;
        
      case LEN_STATE:
        LEN_Token = ch;

        tempDataLen = 0;     
          rxbuf[1] = LEN_Token;
          state = DATA_STATE;

        break;


      case DATA_STATE:

        // Fill in the buffer the first byte of the data 
        rxbuf[2 + tempDataLen++] = ch;

        // If number of bytes read is equal to data length, time to move on to FCS 
        if ( tempDataLen == LEN_Token - 1 )
            state = FCS_STATE;

        break;

      case FCS_STATE:

        FSC_Token = ch;
        uint8_t ii;
        uint8_t crcout;
        // Make sure it's correct 
        crcout = 0;
        for(ii=0;ii<1 + LEN_Token;ii++)
          crcout += rxbuf[ii];

        rxbuf[LEN_Token + 1] = crcout;
        if (crcout == FSC_Token)
        {
          //printf("recieve:");
          len_global = LEN_Token+1;
          for(ii=0;ii<LEN_Token+1;ii++)
            //printf("%02x",rxbuf[ii]);
            
          //printf("\n");
          id=rxbuf[2];
          id<<=8;
          id|=rxbuf[3];
          

          recieve_usart(rxbuf,LEN_Token+1);
          
          
          
        }
        else
        {
          // deallocate the msg 
          printf("crc wrong\n");
        }

        // Reset the state, send or discard the buffers at this point 
        state = WW_STATE;

        break;

      default:
        state = WW_STATE;
       break;
    }
	}

}

int main(void)
{
	pthread_t id;
//    pthread_t send_usart_pr; 
  
  struct MHD_Daemon *daemon;
  int i=0;
	printf_file("idol STARTING...\n");

	system("amixer cset numid=3 1");

  if((sock=socket(AF_INET,SOCK_STREAM,0)) <0) 
    { 
        printf( "socket error\n"); 
        return   1; 
    } 

    strcpy(ifreq.ifr_name,"eth0");


	if(wiringPiSetup() < 0)
	{
		printf("wiringpi setup failed!\n");
		return 1;
	}
	if((usart_fd = serialOpen("/dev/ttyAMA0",115200)) < 0)
	{
		printf("serial open failed!\n");
		return 1;
	}
	printf("serial test start ...\n");


	if(pthread_create(&id,NULL,(void *) thread,NULL)!=0)
	{
		printf ("Create pthread error!\n");
		return (1);
	}
	
   


  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                             &answer_to_connection, NULL,
                             MHD_OPTION_NOTIFY_COMPLETED, NULL,
                             NULL, MHD_OPTION_END);
  if (NULL == daemon)
  {
    //printf("server creat failed!\n");
    return 1;
  }


  for(i=0;i<30;i++)
  {
  	pinMode (jdq[i], OUTPUT);
  }
  pinMode (SH_CP, OUTPUT);
  pinMode (ST_CP, OUTPUT);
  pinMode (DS, OUTPUT);

   digitalWrite(SH_CP,LOW);
   digitalWrite(ST_CP,LOW);
   digitalWrite(DS,LOW);

  for(i=0;i<5;i++)
  {
  	ctrl_jdq (i+25, LOW);
  }
  //
  //MXJ_GetIdxMessage( 0xe768 );
  //sleep(1);
  //MXJ_GetIdxMessage( 0x968f );
  //sleep(1);

  	//digitalWrite(R1_LED,LOW);
  	

	if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) 
    { 
        printf( "ioctl error\n"); 
        return   1; 
    } 
    for(i=0;i<6;i++)
    {
    	mac[i]=ifreq.ifr_hwaddr.sa_data[i];
    }

  	usleep(500000);
  	printf_file("idol STARTED\n");
	
	time(&now);
	tblock = localtime(&now);
	int flag_run = 0;
	
	while(1)
	{  
		if(working == 1)
		{
			if(flag_run > max_start + max_last)
			{
				working = 0;
				flag_run = 0;
			}

			for(i=0;i<30;i++)
			{
				if(data_start[i] == flag_run)
				{
					ctrl_jdq(i,HIGH);
				}
				if(data_stop[i] == flag_run)
				{
					ctrl_jdq(i,LOW);
				}
			}

			flag_run++;
		}


		
		usleep(1000);
		
		
		
	}
	

	serialClose(usart_fd);
	return (0);
}




void printf_file(char * str)
{
	while(mutex_flag==1);
	mutex_flag = 1;
	time(&now);
	tblock = localtime(&now);

	if ((sp = fopen("/var/log/idol.log","a+")) != NULL)
	{
		fprintf(sp,"[%d-%d-%d %d:%d:%d] ", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
		fprintf(sp,str);			
		fclose(sp);
	}
	mutex_flag = 0;
}

void ctrl_jdq(uint8_t pin,uint8_t action)
{
	int i=0;
	char str[200]={0};
	sprintf(str,"pin:%d,%d\n",pin,action);
	printf_file(str);

	if(pin < 25)
	{
		
		digitalWrite(jdq[pin],action);
		pinstatus[pin] = action;
	}
	else
	{
		//printf_file("pin>=25\n");
		digitalWrite(SH_CP,LOW);
		digitalWrite(ST_CP,LOW);

		pinstatus[pin] = action;

		for(i=0;i<5;i++)
		{
			if(pinstatus[29-i] == 0)
			{
				digitalWrite(DS,LOW);
			}
			else
			{
				digitalWrite(DS,HIGH);
			}

			digitalWrite(SH_CP,HIGH);
			digitalWrite(SH_CP,LOW);
		}

		digitalWrite(ST_CP,HIGH);

		digitalWrite(SH_CP,LOW);
		digitalWrite(ST_CP,LOW);
	}
}






