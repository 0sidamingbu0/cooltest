/*
//gcc zbclient.c -o zbclient -lwiringPi -lcurl -lpthread -ljson-c -lmicrohttpd
sudo cp json-c/* /usr/lib/

#build libmicrohttp.so
donwload src
cp to raspi
sudo apt-get install autoconf
sudo apt-get install litool
autoreconf -fi
./configure --disable-curl
sudo make
sudo make install
sudo cp /usr/local/lib/libmicrohttp.* /lib/arm-linux-gnueabihf/
regcc .c
ok

=====
sudo rm /usr/arm-linux-gnueabihf/libmicrohttpd.so
sudo cp libmicrohttpd.so.12.37.0 /usr/arm-linux-gnueabihf/
sudo ln -s libmicrohttpd.so.12.37.0 libmicrohttpd.so.12
sudo ln -s libmicrohttpd.so.12 libmicrohttpd.so

----
ls -l |grep libmicrohttpd
sudo find / -name libmicrohttpd*
*/
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

#include   <sys/ioctl.h> 
#include   <sys/socket.h> 
#include   <netinet/in.h> 
#include   <net/if.h> 

#include <sys/syscall.h>  

char * softversion = "ZB2017060800";
char   returnstr[200]={0};
char hostname[50]={0};
char startTime[50]={0};
char mac[6];
int alarm_flag = 0;
int music_state = 0;
int music_thread_alive = 0;
int vol_flag = 3;
int vol = 80;
int shefang = 0;
int mutex_flag = 0;
uint8_t alarm_state1=0,alarm_state2=0;
#define DEBUG() printf("lines: %d\n", __LINE__); fflush(stdout);

#define POWER_2530     		0
uint8_t hardwareVersion = 0;
uint8_t firmwareVersion = 0;
uint8_t channel = 0;

#define R2_LED     			8 //gpio p:29 //1,2fan le 
#define G2_LED     			2 //gpio p:36
#define B2_LED     			3 //gpio p:35

#define R1_LED     			8 //gpio p:19
#define G1_LED     			2 //gpio p:21
#define B1_LED     			3  //gpio p:22

#define R3_LED     			8  //gpio p:13
#define G3_LED     			2  //gpio p:16
#define B3_LED     			3  //gpio p:15

#define R4_LED     			8  //gpio p:7
#define G4_LED     			2  //gpio p:12
#define B4_LED     			3  //gpio p:11

#define MUTE     			23  //gpio p:33



typedef unsigned char   uint8_t;     //?¡ª ??|?¡¤8????¡ã
typedef unsigned short uint16_t;  


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

#define MXJ_GATEWAY_CLUSTERID        0x01
  
#define MXJ_PM25_CLUSTERID           0x02
#define MXJ_HUMAN_CLUSTERID          0x03
#define MXJ_SWITCH_CLUSTERID         0x04
#define MXJ_LIGHT_CLUSTERID          0x05
#define MXJ_POWER_CLUSTERID          0x06
#define MXJ_REMOTE_CLUSTERID         0x07
#define MXJ_LIGHT_SENSOR_CLUSTERID   0x08
#define MXJ_TEMPERATURE_CLUSTERID    0x09
#define MXJ_DOOR_CLUSTERID           0x0A
#define MXJ_REV1_CLUSTERID           0x0B
#define MXJ_REV2_CLUSTERID           0x0C
#define MXJ_REV3_CLUSTERID           0x0D
#define MXJ_REV4_CLUSTERID           0x0E
#define MXJ_REV5_CLUSTERID           0x0F
  
#define MXJ_CTRL_DOWN                0x00
#define MXJ_CTRL_UP                  0x01
#define MXJ_REGISTER_REQUEST         0x02
#define MXJ_REGISTER_OK              0x03
#define MXJ_REGISTER_FAILED          0x04
#define MXJ_GET_STATE                0x05
#define MXJ_SEND_STATE               0x06
#define MXJ_SENSOR_DATA              0x07
#define MXJ_CONFIG_WRITE             0x08
#define MXJ_CONFIG_RETURN            0x09
#define MXJ_CONFIG_GET               0x0A //USE
#define MXJ_GET_IDXS                 0x0B
#define MXJ_SEND_IDXS                0x0C
#define MXJ_SEND_RESPONSE            0x0D 
#define MXJ_PING_REQUEST             0x0F
#define MXJ_DEVICE_ANNCE             0x0E
#define MXJ_PING_RESPONSE            0x10
#define MXJ_SEND_RESET               0x11  
#define MXJ_XIAOMI18                 0x18
#define MXJ_XIAOMI1C                 0x1C
#define MXJ_SEND_DATA                0xff
#define MXJ_VERSION                  0x55

struct   ifreq   ifreq; 
int   sock; 


int permitjoin = 0;

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
void MXJ_SendRegisterMessage( uint16_t , uint8_t );
void MXJ_SendPingMessage( uint16_t id );
void MXJ_GetIdxMessage( uint16_t id );

void MXJ_SendCtrlMessage( uint16_t ,uint8_t ,uint8_t len, uint8_t , uint8_t , uint8_t );
void MXJ_SendStateMessage( uint16_t );

void MXJ_GetStateMessage( uint16_t id );
void printf_file(char * str);
void alarm_register(void);
void alarm_sendState(uint8_t one,uint8_t two);
void thread_alarm(void);

void MXJ_ReadCongifMessage( uint16_t id );
void MXJ_WriteCongifMessage( uint16_t id ,uint8_t data);

void device_reset(void);
void device_factory(void);
void device_permit(void);
void device_getversion(void);
void MXJ_LearnIrCode( uint16_t id);
void MXJ_SendIrCode( uint16_t id,uint8_t *data2,uint8_t len );
void time_response(uint16_t id);


void music_sendState(uint8_t one,uint8_t two,uint8_t three);
void music_register(void);
void thread_music(void);
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
	DEBUG();
  int ret;
  struct MHD_Response *response;

DEBUG();
  response =
    MHD_create_response_from_buffer (strlen (page), (void *) page,
				     MHD_RESPMEM_PERSISTENT);
  if (!response)
    return MHD_NO;

  //MHD_add_response_header (response, "Content-Type", "text/html;charset=UTF-8");
DEBUG();
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
DEBUG();
  return ret;
}


static int
iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
              const char *filename, const char *content_type,
              const char *transfer_encoding, const char *data, uint64_t off,
              size_t size)
{
	DEBUG();
  struct connection_info_struct *con_info = coninfo_cls;
  if(size == 0) return MHD_NO;
	DEBUG();
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
	DEBUG();
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
DEBUG();	//struct connection_info_struct *con_info = *con_cls;
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
DEBUG();
  if (0 == strcmp (method, "POST"))
  	{  		
	  int ret;
		
	  if(0 == strcmp (url, "/zbClient/API/permit"))
		{
			permitjoin = 1;
			mutex_flag = 0;
			device_permit();
			mutex_flag = 1;
			printf("[%d-%d-%d %d:%d:%d] POST RECIEVE len=0 url=%s version=%s body=NULL\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len2, url, version);
		 	if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		    {
			    fprintf(sp,"[%d-%d-%d %d:%d:%d] POST RECIEVE len=0 url=%s version=%s body=NULL\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, url, version);
			    fclose(sp);
		    }
		}
		if(0 == strcmp (url, "/zbClient/API/factory"))
		{
			mutex_flag = 0;
			device_factory();
			mutex_flag = 1;
			printf("[%d-%d-%d %d:%d:%d] POST RECIEVE len=0 url=%s version=%s body=NULL\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len2, url, version);
		 	if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		    {
			    fprintf(sp,"[%d-%d-%d %d:%d:%d] POST RECIEVE len=0 url=%s version=%s body=NULL\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, url, version);
			    fclose(sp);
		    }
		}
		if(0 == strcmp (url, "/zbClient/API/getVersion"))
		{
			mutex_flag = 0;
			device_getversion();
			mutex_flag = 1;
			printf("[%d-%d-%d %d:%d:%d] POST RECIEVE len=0 url=%s version=%s body=NULL\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len2, url, version);
		 	if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		    {
			    fprintf(sp,"[%d-%d-%d %d:%d:%d] POST RECIEVE len=0 url=%s version=%s body=NULL\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, url, version);
			    fclose(sp);
		    }
		}
		if(0 == strcmp (url, "/zbClient/API/restart"))
		{
			mutex_flag = 0;
			device_reset();
			mutex_flag = 1;
			printf("[%d-%d-%d %d:%d:%d] POST RECIEVE len=0 url=%s version=%s body=NULL\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len2, url, version);
		 	if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		    {
			    fprintf(sp,"[%d-%d-%d %d:%d:%d] POST RECIEVE len=0 url=%s version=%s body=NULL\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, url, version);
			    fclose(sp);
		    }
		}

		DEBUG();
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
		DEBUG();
			if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
			{
				fprintf(sp,"[%d-%d-%d %d:%d:%d] POST RECIEVE len=%d url=%s version=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len2, url, version,body);
				fclose(sp);
			}
		DEBUG();
			if(len2 != 0)
			{
				json_object *my_object = json_tokener_parse(re_body);
				DEBUG();
				free(re_body);
				DEBUG();
				if(json_object_to_json_string(my_object) != NULL && (0 != strcmp (json_object_to_json_string(my_object), "null")))
				{
				  uint16_t address=0;
				  uint8_t index=0,resourceSum=0,commandint=255,commandData=0;
				  const char * command;
				  const char * deviceType;
				  post_type = 1;
				  DEBUG();
				  json_object *Jaddress = NULL;
				  json_object_object_get_ex(my_object, "address",&Jaddress);
				  json_object *Jindex = NULL;
				  json_object_object_get_ex(my_object, "index",&Jindex);
				  json_object *Jcommand = NULL;
				  json_object_object_get_ex(my_object, "command",&Jcommand);
				  json_object *JcommandData = NULL;
				  json_object_object_get_ex(my_object, "commandData",&JcommandData);
				  json_object *JresourceSum = NULL;
				  json_object_object_get_ex(my_object, "resourceSum",&JresourceSum);
				  json_object *JdeviceType = NULL;
				  json_object_object_get_ex(my_object, "deviceType",&JdeviceType);
				  DEBUG();
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
				  //printf("recieve post json= %s\n", json_object_to_json_string(my_object));
				  DEBUG();
				  	if(address == 0xfffe)
				  	{
				  		if(0 == strcmp (url, "/zbClient/API/send"))
					  	{
					  		
					  		if(0 == strcmp (command, "\"On\""))
					  		{
					  			
					  			if(index ==1)
					  			{
					  				alarm_state1 = 1;
									shefang = 1;								
					  			}

					  			if(index ==2)
					  			{
					  				alarm_state2 = 1;
									alarm_flag = 1;
					  			}	
					  			mutex_flag = 0;
					  			alarm_sendState(alarm_state1,alarm_state2);
					  			mutex_flag = 1;
					  		}
					  		if(0 == strcmp (command, "\"Off\""))
					  		{
					  			if(index ==1)
					  			{
					  				alarm_state1 = 0;
					  				shefang = 0;
					  			}

					  			if(index ==2)
					  			{
					  				alarm_state2 = 0;
									alarm_flag = 0;
					  			}
					  			mutex_flag = 0;
					  			alarm_sendState(alarm_state1,alarm_state2);
					  			mutex_flag = 1;
					  		}
					  	}
				  	}
				  	else if(address == 0x0001)
				  	{
				  		if(0 == strcmp (url, "/zbClient/API/send"))
					  	{
					  		
					  		if(0 == strcmp (command, "\"On\""))
					  		{
					  			
					  			if(index ==1)
					  			{					  				
					  				music_state = 1;								
					  			}

					  			if(index ==2)
					  			{
					  				vol_flag = 1;
					  			}
					  			if(index ==3)
					  			{
					  				vol_flag = 0;
					  			}	
					  			
					  		}
					  		if(0 == strcmp (command, "\"Off\""))
					  		{
					  			if(index ==1)
					  			{					  			
					  				music_state = 0;
					  			}

					  			
					  			
					  		}
					  	}
				  	}
				  	else
				  	{
					    if(0 == strcmp (url, "/zbClient/API/feedback/register"))
					  	{
					  		//printf("register ok\n");
					  		mutex_flag = 0;
							MXJ_SendRegisterMessage( address, MXJ_REGISTER_OK);
							mutex_flag = 1;
					  	}
						if(0 == strcmp (url, "/zbClient/API/delete"))
					  	{
					  		//printf("delete\n");
					  		mutex_flag = 0;
							MXJ_SendRegisterMessage( address, MXJ_REGISTER_FAILED);
							mutex_flag = 1;
					  	}
						if(0 == strcmp (url, "/zbClient/API/get/state"))
					  	{
					  		//printf("get state\n");
					  		mutex_flag = 0;
							MXJ_GetStateMessage(address);
							mutex_flag = 1;
					  	}
						if(0 == strcmp (url, "/zbClient/API/send"))
					  	{
					  		//printf("control\n");
					  		if(0 == strcmp (command, "\"On\""))
					  		{
					  			commandint = 1;
					  			if(commandData <= 255&&commandData>0)
					  				commandint = commandData;

					  		}
					  		if(0 == strcmp (command, "\"Off\""))
					  		{
					  			commandint = 0;
					  		}
					  		if(0 == strcmp (command, "\"Hold\""))
					  		{
					  			commandint = 2;
					  		}							
					  		if(0 == strcmp (command, "\"Reverse\""))
					  		{
					  			commandint = 3;
					  		}
							//printf("commandint=%d\n",commandint);
							if(0 == strcmp (deviceType, "\"IrRemote\""))
							{
								
								//uint8_t data[200];//={0,8,(uint8_t)(address>>8),(uint8_t)address,6,2,0x11,sequenceNumber++,commandint};	
								if(0 == strcmp (command, "\"Learn\""))
						  		{
						  			//printf("Learn\n");
						  			mutex_flag = 0;						  			
						  			MXJ_LearnIrCode(address);
						  			mutex_flag = 1;							  			
						  		}
						  		else if(0 == strcmp (command, "\"Send\""))
						  		{

									int i;
									uint8_t data[200];
									for(i=0; i < json_object_array_length(JcommandData); i++)
									{
										json_object *obj = json_object_array_get_idx(JcommandData, i);
										json_object *obj_value = NULL;
								  		json_object_object_get_ex(obj, "value",&obj_value);
										data[i] = (uint8_t)json_object_get_int(obj_value);
										//printf("JcommandData[%d]=%s\n", i, json_object_to_json_string(obj));
										printf("data[%d]=%d", i, data[i]);					
									}								
						  			mutex_flag = 0;
						  			MXJ_SendIrCode(address,data,json_object_array_length(JcommandData));
						  			mutex_flag = 1;	
						  		}							  										
							}
							else 
							{
								if(commandint!=255)
								{
									static uint8_t sequenceNumber = 0;
									
									if(index == 1)
									{
										mutex_flag = 0;

										if(0 == strcmp (deviceType, "\"N_SwitchLightPanel_Mi\""))
										{
											
											uint8_t data[9]={0,8,(uint8_t)(address>>8),(uint8_t)address,6,2,0x11,sequenceNumber++,commandint};	

	  										send_usart(data,9);
										}
										else if(0 == strcmp (deviceType, "\"PowerPanel_Mi\""))
										{
											
											uint8_t data[9]={0,8,(uint8_t)(address>>8),(uint8_t)address,6,1,0x11,sequenceNumber++,commandint};	

	  										send_usart(data,9);
										}
										else
										{
											MXJ_SendCtrlMessage(address ,1, resourceSum,commandint , 2 , 2 );
										}

										mutex_flag = 1;
									}
									else if(index == 2)
									{
										mutex_flag = 0;
										if(0 == strcmp (deviceType, "\"N_SwitchLightPanel_Mi\""))
										{
											
											uint8_t data[9]={0,8,(uint8_t)(address>>8),(uint8_t)address,6,3,0x11,sequenceNumber++,commandint};						
	  										send_usart(data,9);
										}
										else
										{
											MXJ_SendCtrlMessage(address ,1, resourceSum, 2 ,commandint, 2 );
										}
										mutex_flag = 1;
									}
									else if(index == 3)
									{
										mutex_flag = 0;
										MXJ_SendCtrlMessage(address ,1, resourceSum, 2, 2 , commandint );
										mutex_flag = 1;
									}
								}
							}
					  	}
						if(0 == strcmp (url, "/zbClient/API/info/get"))
					  	{
					  		//printf("get device\n");
					  		mutex_flag = 0;
							MXJ_GetIdxMessage(address);
							mutex_flag = 1;
					  	}
						if(0 == strcmp (url, "/zbClient/API/ping"))
					  	{
					  		//printf("PING request\n");
					  		mutex_flag = 0;
							MXJ_SendPingMessage(address);
							mutex_flag = 1;
					  	}
						if(0 == strcmp (url, "/zbClient/API/value/get"))
					  	{
					  		//printf("get data\n");
					  		mutex_flag = 0;
							MXJ_GetStateMessage(address);
							mutex_flag = 1;
					  	}

					  	if(0 == strcmp (url, "/zbClient/API/config/write"))
					  	{
					  		//printf("get device\n");
					  		mutex_flag = 0;
					  		MXJ_WriteCongifMessage(address,(uint8_t)commandData);							
							mutex_flag = 1;
					  	}

					  	if(0 == strcmp (url, "/zbClient/API/config/read"))
					  	{
					  		//printf("get device\n");
					  		mutex_flag = 0;
							MXJ_ReadCongifMessage(address);
							mutex_flag = 1;
					  	}
					}
				}				
			}
		DEBUG();
		*upload_data_size = 0;
		mutex_flag = 0;
		return MHD_YES;
	  }
	  else if(post_type == 2)
	  {
	  	DEBUG();
	  		*ptr = NULL; /* clear context pointer */
	  	mutex_flag = 0;
	  	  return send_page (connection,askpage);
	  }
	  else if(post_type == 1)
	  {
	  	DEBUG();
	  	*ptr = NULL; /* clear context pointer */
	  	mutex_flag = 0;
		  return send_page (connection,askpage);
	  }
	  else
	  {
	  	DEBUG();
	  	*ptr = NULL; /* clear context pointer */
	  	mutex_flag = 0;
		  return send_page (connection, askpage);  
	  }
  	}
		DEBUG();
	*ptr = NULL; /* clear context pointer */
		mutex_flag = 0;
  return send_page (connection, errorpage);
}



uint8_t fifo_add(uint8_t *data,uint8_t len)
{
	int i;
	while(mutex_flag==1);
	mutex_flag = 1;
	if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
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

	softPwmWrite(G1_LED,0);
	softPwmWrite(G2_LED,0);
	softPwmWrite(G3_LED,0);
	softPwmWrite(G4_LED,0);	
	
	mutex_flag = 0;
	/*
	uint8_t ii;
	fifo[fifo_end].len=len;
  for(ii=0;ii<len;ii++)
	{
		fifo[fifo_end].data[ii]=data[ii];
	}
  
  fifo_end++;
  if(fifo_end >= FIFO_SIZE)
  {
      fifo_end=0;
      fifo_jinwei=1;
  }
  */
  return 1;
}

uint8_t fifo_read(uint8_t **data1,uint8_t *len)
{
  if(fifo_jinwei == 0)
  {
    if(fifo_start >= fifo_end)
    {
      return 0;
    }
  }
  
  *data1 = fifo[fifo_start].data;
  *len = fifo[fifo_start].len;
  fifo_start++;
  if(fifo_start >= FIFO_SIZE)
  {
    fifo_start = 0;
    fifo_jinwei = 0;
  }
  return 1;
}



void send_usart(uint8_t *data,uint8_t len) //id,state1,state2,state3 1=?a,0=1?,2=¡À¡ê3?
{
  uint8_t txbuf[100];
  uint8_t i=0;
  uint8_t crc = 0;
  txbuf[0] = 0x55;
  txbuf[1] = 0xaa;
  softPwmWrite(B1_LED,0);
  softPwmWrite(B2_LED,0);
  softPwmWrite(B3_LED,0);
  softPwmWrite(B4_LED,0);
	if(digitalRead (R1_LED) == 0)
	{
	  softPwmWrite(G1_LED,10);
	  softPwmWrite(G2_LED,10);
	  softPwmWrite(G3_LED,10);
	  softPwmWrite(G4_LED,10);
	}
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
  	/*
  for(i=0;i<len+3;i++)
  {
    serialPutchar(usart_fd,txbuf[i]);
    printf("%02x ",txbuf[i]);
  }
  printf("\n");
  */
}

void thread_send(void)
{
	/*
	while(1)
	{
		uint8_t i;
		uint8_t *txbuf;
		uint8_t len=0;	

		
		if(fifo_read(&txbuf,&len))
		{			
			time(&now);
			tblock = localtime(&now);
			
			if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
			{
				fprintf(sp,"[%d-%d-%d %d:%d:%d] USART SEND len=%d data=", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len);
				for(i=0;i<len;i++)
				  {
				    fprintf(sp,"%02x ",txbuf[i]);
				  }
				  fprintf(sp,"\n");
				  fclose(sp);
			}
			
			printf("[%d-%d-%d %d:%d:%d] USART SEND len=%d data=", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len);

			for(i=0;i<len;i++)
			{
				serialPutchar(usart_fd,txbuf[i]);
				printf("%02x ",txbuf[i]);
			}
			printf("\n");
		}

		if(permitjoin == 1)
		{
			digitalWrite(22,LOW);
			permitjoin++;
		}
		if(permitjoin >1)
		{			
			permitjoin++;
		}		  

		usleep(5000);
		if(permitjoin > 40)
		{
			permitjoin = 0;
			digitalWrite(22,HIGH);
		}

		
		led_state++;
		if(led_state >= 10)
		{
			softPwmWrite(G1_LED,0);
			softPwmWrite(G2_LED,0);
			softPwmWrite(G3_LED,0);
			softPwmWrite(G4_LED,0);			
			led_state = 0;
		}
		
		//sleep(1);
	}
	*/
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
	digitalWrite(G1_LED,HIGH);
	digitalWrite(G2_LED,HIGH);
	digitalWrite(G3_LED,HIGH);
	digitalWrite(G4_LED,HIGH);
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
  
  

  if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
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

    case MXJ_CTRL_UP:
	{
		char* event;
		eventDataStr[0] = '\0';
		switch(rx[9])
		{
			case 0: event = "PressDown";break;
			case 1: event = "PressUp";break;
			case 2: event = "DoubleClick";break;
			case 3: event = "Click";break;
			case 4: event = "BodyMove";break;
			case 10: event = "BodyLeave";break;
			case 5: event = "ReportData";break;
			case 6: event = "LongPress";break;
			case 7: event = "TriggerByCloud";break;	
			case 8: event = "ReportCardId";
					eventData32 = rx[10];
					eventData32 <<= 8;
					eventData32 |= rx[11];
					eventData32 <<= 8;
					eventData32 |= rx[12];
					eventData32 <<= 8;
					eventData32 |= rx[13];
					sprintf(eventDataStr, "%u", eventData32);
					//eventDataStr[4] = '\0';
			break;
			case 9: event = "ReportPassword";
					//eventDataStr[0] = '\0';
					for(i=0;i<rx[7]-2;i++)
					{
						char strtemp[2];
						sprintf(strtemp, "%d", rx[10+i]);
						strcat(eventDataStr,strtemp);
					}
					eventDataStr[rx[7]-2] = '\0';
					
			break;	
			default : event = "Unknow event";
		}
		
		char str[200]={0};
		char str_url[200]={0};
		sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"eventData\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,rx[8],event,eventDataStr,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
		sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
		curl_easy_setopt(posturl, CURLOPT_URL, str_url);
		curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
		curl_easy_perform(posturl);	
		
		printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
		if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		{
			fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
			  fclose(sp);
		}
						
	break;
	}

	case MXJ_CONFIG_GET:
	{
		char* event;


		if(rx[8]==0xff)
			rx[8] = 0;
		if(len!= 19)
			break;

		event = "ReportConfig";
		char str[200]={0};
		char str_url[200]={0};
		sprintf(str,"{\"address\":\"%d\",\"event\":\"%s\",\"eventData\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,event,rx[8],rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
		sprintf(str_url,"127.0.0.1:%d/device/API/config",PORT_CLIENT);
		curl_easy_setopt(posturl, CURLOPT_URL, str_url);
		curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
		curl_easy_perform(posturl);	
		
		printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
		if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		{
			fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
			  fclose(sp);
		}

		//printf("MXJ_CONFIG_GET\n len=%d",len);
		break;
    }

    case MXJ_REGISTER_REQUEST: //rx[7]=zb:len rx[8]=zbdata[0] rx[9]=zbdata[1]
      	//MXJ_SendRegisterMessage(id,MXJ_REGISTER_OK);
    {
      	char* deviceType;
      	//if(cid == 6)break;//close powerpanel register message /alarm,background music

      	if((id == 0xfffe || id == 0x0001)&&(rx[len-1] != 255))break;//close powerpanel register message /alarm,background music

      	switch(cid)
  		{
			case 1: deviceType = "Gateway";break;
			case 2: deviceType = "PM2.5Sensor";break;
			case 3: deviceType = "BodySensor";break;
			case 4: deviceType = "N_SwitchLightPanel";break;
			case 5: deviceType = "FIVE";break;
			case 6: deviceType = "PowerPanel";break;
			case 7: deviceType = "N_Button";break;
			case 8: deviceType = "LightSensor";break;
			case 9: deviceType = "TemperatureSensor";break;
			case 10: deviceType = "MagnetSensor";break;
			case 11: deviceType = "SmokeSensor";break;
			case 12: deviceType = "DoorController";break;
			case 13: deviceType = "SoundSensor";break;
			case 14: deviceType = "MiButton";break;
			case 17: deviceType = "MultiSensor";break;
			case 18: deviceType = "MotionDetecterSwitch";break;
			case 19: deviceType = "ACSwitchPanel";break;
			case 20: deviceType = "AirCleanerPanel";break;
			case 21: deviceType = "IrRemote";break;
			default : deviceType = "Unknow device";
  		}
  		char str[200]={0};
		char str_url[200]={0};
  		if(rx[7]==2&&rx[9]>=1)//version=1
  		{
  			sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"version\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,deviceType,rx[8],rx[len-1],rx[9],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
			sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
  		}
		else
		{	
			sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"version\":\"0\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,deviceType,rx[8],rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
			sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
		}
		curl_easy_setopt(posturl, CURLOPT_URL, str_url);
		curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
		curl_easy_perform(posturl);
		printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
		if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		{
			fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
			  fclose(sp);
		}
		
		//MXJ_SendRegisterMessage( id, MXJ_REGISTER_OK);

    break;
	}

    case MXJ_SEND_STATE:
	{
		char str[2000]={0};
		char strtemp[2000]={0};
		char str_url[200]={0};
		int i=0;
		sprintf(str,"{\"address\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\",\"status\":[",id,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
		for(i=0;i<rx[7];i++)
		{
			sprintf(strtemp,"{\"value\":\"%d\"}",rx[8+i]);
			strcat (str,strtemp);
			if(i!=rx[7]-1&&rx[7]>1)
			{
				strcat (str,",");
			}
		}
		strcat (str,"]}");
		
		sprintf(str_url,"127.0.0.1:%d/device/API/feedback/status",PORT_CLIENT);
		curl_easy_setopt(posturl, CURLOPT_URL, str_url);
		curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
		curl_easy_perform(posturl);
		printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
		if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		{
			fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
			  fclose(sp);
		}	
	break;
	}


    case MXJ_SENSOR_DATA:
	{
		char* type;
		uint16_t tempdata=0;
		float tempdatafloat=0;
		char strtemp[200]={0};
		tempdata=rx[10];
		tempdata <<= 8;
		tempdata |= rx[11];
		tempdatafloat = tempdata;
		if(tempdata == 65535)
			break;
		switch(rx[9])
		{
			case 0: type = "Temperature";tempdatafloat = tempdata/10;break;
			case 1: type = "Humidity";tempdatafloat = tempdata/10;break;
			case 2: type = "PM2.5";break;
			case 3: type = "PM10";break;
			case 4: type = "Sound";break;
			case 5: type = "Light";break;
			case 6: type = "Smoke";break;
			case 7: type = "Hcho";tempdatafloat = tempdata/1000;break;
			case 8: type = "CO2";break;		
			case 10: type = "IrCode";break;					
			default : type = "Unknow type";
		}
		char str[200]={0};
		char str_url[200]={0};

		if(rx[9] == 10)
		{
			sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"type\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\",\"data\":[",id,rx[8],type,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
			for(i=0;i<rx[7];i++)
			{				
				sprintf(strtemp,"{\"value\":\"%d\"}",rx[10+i]);
				strcat (str,strtemp);
				if((i!=rx[7]-1)&&rx[7]>1)
				{
					strcat (str,",");
				}
			}
			strcat (str,"]}");
		}
		else
		{
			sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"data\":\"%f\",\"type\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,rx[8],tempdatafloat,type,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);	
		}		
		
		sprintf(str_url,"127.0.0.1:%d/device/API/value",PORT_CLIENT);
		curl_easy_setopt(posturl, CURLOPT_URL, str_url);
		curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
		curl_easy_perform(posturl);	
		
		printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
		if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		{
			fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
			  fclose(sp);
		}
						
	break;
	}


    case MXJ_PING_RESPONSE:
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
			if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
			{
				fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
				  fclose(sp);
			}    
		}

		if(len == 21)
		{			
			time_response(id);						
		}
    break;
	}

    case MXJ_DEVICE_ANNCE:
	{
		char str[200]={0};
		char str_url[200]={0};
		sprintf(str,"{\"address\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,rx[11],rx[10],rx[9],rx[8],rx[7],rx[6],rx[5],rx[4]);
		sprintf(str_url,"127.0.0.1:%d/device/API/report",PORT_CLIENT);
		curl_easy_setopt(posturl, CURLOPT_URL, str_url);
		curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
		curl_easy_perform(posturl);
		printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
		if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		{
			fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
			  fclose(sp);
		}

    break;
	}	  
	
	case MXJ_SEND_RESET:
	{
		char str[200]={0};
		char str_url[200]={0};
		sprintf(str,"{\"address\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
		sprintf(str_url,"127.0.0.1:%d/device/API/rest",PORT_CLIENT);
		curl_easy_setopt(posturl, CURLOPT_URL, str_url);
		curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
		curl_easy_perform(posturl);
		printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
		if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		{
			fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
			  fclose(sp);
		}		
    break;
	} 

		
    case MXJ_XIAOMI18:	

	if(cid == 6)
	{
		//if(len != 13 + 1 + 8)break;
		//printf("control up - find id = %d\n",i);
		//printf("id:%4x\n",id);
		uint8_t ep=1;
		if(endpoint == 5)
			ep = 2;
		if(len == 13 + 1 + 8 + 1)
		{
			if(rx[11] == 0x20)
			{
				//printf("double kick\n");
				char str[200]={0};
				char str_url[200]={0};
				sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,ep,"DoubleClick",rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
				sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
				curl_easy_setopt(posturl, CURLOPT_URL, str_url);
				curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
				curl_easy_perform(posturl);	
				printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
				if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
				{
					fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
					  fclose(sp);
				}			
			}
			else
			{
				//printf("action = %d\n",rx[12]);			
				char* event;
				switch(rx[12])
				{
					case 0: event = "PressDown";break;
					case 1: event = "PressUp";break;
					case 2: event = "DoubleClick";break;
					case 3: event = "Click";break;
					case 4: event = "BodyMove";break;
					case 5: event = "ReportData";break;
					case 6: event = "LongPress";break;
					case 7: event = "TriggerByCloud";break; 
					default : event = "Unknow event";
				}
				char str[200]={0};
				char str_url[200]={0};
				sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,ep,event,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
				sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
				curl_easy_setopt(posturl, CURLOPT_URL, str_url);
				curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
				curl_easy_perform(posturl); 
				printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
				if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
				{
					fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
					  fclose(sp);
				}

			}
		}
		else if(len == 17 + 1 + 8 + 1)//12=1,16=0
		{
			if(rx[12] == 1 && rx[16] == 0)
			{
				if(endpoint == 3)
				{
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,1,"Click",rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}	

					sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,2,"Click",rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}

				}
				else
				{
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,endpoint,"Click",rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}	
				}		
			}

			if(rx[12] == 0 && rx[16] == 1)
			{
				if(endpoint == 6)
				{
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,1,"Click",rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}	

					sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,2,"Click",rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}

				}
				else if(endpoint == 4)
				{
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,1,"Click",rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}	
				}	
				else if(endpoint == 5)
				{
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,2,"Click",rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}	
				}	
			}
		}
		else if(len == 30)
		{
			if(endpoint == 1)
			{
				
				char str[200]={0};
				char str_url[200]={0};
				char strtemp[200]={0};
				int i=0;

				sprintf(str,"{\"address\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\",\"status\":[{\"value\":\"%d\"}]}",id,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2],rx[12]);				
				sprintf(str_url,"127.0.0.1:%d/device/API/feedback/status",PORT_CLIENT);
				curl_easy_setopt(posturl, CURLOPT_URL, str_url);
				curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
				curl_easy_perform(posturl);
				printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
				if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
				{
					fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
					  fclose(sp);
				}
			}
			else if(endpoint == 2)
			{
				char str[200]={0};
				char strtemp[200]={0};
				char str_url[200]={0};
				int i=0;
				sprintf(str,"{\"address\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\",\"status\":[{\"value\":\"%d\"},{\"value\":\"%d\"}]}",id,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2],rx[12],2);
				
				sprintf(str_url,"127.0.0.1:%d/device/API/feedback/status",PORT_CLIENT);
				curl_easy_setopt(posturl, CURLOPT_URL, str_url);
				curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
				curl_easy_perform(posturl);
				printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
				if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
				{
					fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
					  fclose(sp);
				}
			}
			else if(endpoint == 3)
			{
				char str[200]={0};
				char strtemp[200]={0};
				char str_url[200]={0};
				int i=0;
				sprintf(str,"{\"address\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\",\"status\":[{\"value\":\"%d\"},{\"value\":\"%d\"}]}",id,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2],2,rx[12]);
				
				sprintf(str_url,"127.0.0.1:%d/device/API/feedback/status",PORT_CLIENT);
				curl_easy_setopt(posturl, CURLOPT_URL, str_url);
				curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
				curl_easy_perform(posturl);
				printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
				if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
				{
					fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
					  fclose(sp);
				}
			}	
		}		
	}
	else if(cid == 0x406)
	{
		if(len != 13 + 1 + 8 + 1)break;
		//printf("control up - find id = %d\n",i);
		//printf("id:%4x\n",id);
		//printf("human detected\n");
		char str[200]={0};
		char str_url[200]={0};
		sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"event\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,1,"BodyMove",rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
		sprintf(str_url,"127.0.0.1:%d/device/API/command",PORT_CLIENT);
		curl_easy_setopt(posturl, CURLOPT_URL, str_url);
		curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
		curl_easy_perform(posturl);	
		printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
		if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		{
			fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
			  fclose(sp);
		}		
	}
	else if(cid == 0x402)
	{
		int16_t temp = 0;
		if(len != 14 + 1 + 8 + 1)break;
		static uint8_t temp_flag = 0;
		temp = rx[13];
		temp <<= 8;
		temp |= rx[12];
		//printf("temperature up - find id = %d\n",i);
		//printf("id:%4x\n",id);
		//printf("temperature = %02f\n",(float)temp/100);
		char str[200]={0};
		char str_url[200]={0};
		sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"type\":\"%s\",\"data\":\"%f\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,1,"Temperature",(float)temp/100,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
		sprintf(str_url,"127.0.0.1:%d/device/API/value",PORT_CLIENT);
		curl_easy_setopt(posturl, CURLOPT_URL, str_url);
		curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
		curl_easy_perform(posturl);	
		printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
		if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		{
			fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
			  fclose(sp);
		}		

	}
	else if(cid == 0x405)
	{
	 	uint16_t temp = 0;
		if(len != 14 + 1 + 8 + 1)break;
		temp = rx[13];
		temp <<= 8;
		temp |= rx[12];
		//printf("humidity up - find id = %d\n",i);
		//printf("id:%4x\n",id);
		//printf("humidity = %02f\n",(float)temp/100);
		char str[200]={0};
		char str_url[200]={0};
		sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"type\":\"%s\",\"data\":\"%f\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,1,"Humidity",(float)temp/100,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
		sprintf(str_url,"127.0.0.1:%d/device/API/value",PORT_CLIENT);
		curl_easy_setopt(posturl, CURLOPT_URL, str_url);
		curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
		curl_easy_perform(posturl);		
		printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
		if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
		{
			fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
			  fclose(sp);
		}		
	}
  	
	
	if(len>=27 && len < 32 + 1 + 8 + 3 + 1 + 2)
		{
			if(cid == 0 && rx[12] <= len - 13)
			{
				 uint8_t *temp_str = NULL;
				/*¡¤????¨²¡ä?????*/
				temp_str = (uint8_t*)calloc(rx[12],sizeof(uint8_t));
				strncpy(temp_str, &rx[13],rx[12]);
				//printf("temp_str = %s\n",temp_str);
				if(id == 0xfffe || id == 0x0001)break;//close powerpanel register message /alarm,background music

				if (0 == strcmp (temp_str, "lumi.sensor_86sw1"))
				{
					//printf("switch join\n");
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,"N_SwitchButton_Mi",1,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}					
				}
				else if (0 == strcmp (temp_str, "lumi.sensor_86sw2"))
				{
					//printf("switch join\n");
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,"N_SwitchButton_Mi",2,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}					
				}
				else if (0 == strcmp (temp_str, "lumi.ctrl_neutral1"))
				{
					//printf("switch join\n");
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,"N_SwitchLightPanel_Mi",1,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}					
				}
				else if (0 == strcmp (temp_str, "lumi.ctrl_neutral2"))
				{
					//printf("switch join\n");
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,"N_SwitchLightPanel_Mi",2,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}					
				}
				else if (0 == strcmp (temp_str, "lumi.sensor_switch"))
				{
					//printf("switch join\n");
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,"MiButton",1,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}					
				}
				else if (0 == strcmp (temp_str, "lumi.sensor_magnet"))
				{
					//printf("magnet join\n");
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,"MagnetSensor",1,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);			
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}					
				}
				else if (0 == strcmp (temp_str, "lumi.sensor_motion"))
				{
					//printf("motion join\n");
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,"BodySensor",1,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}					
				}
				else if (0 == strcmp (temp_str, "lumi.sensor_ht"))
				{
					//printf("ht join\n");
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,"TemperatureSensor",1,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}					
				}
				else if (0 == strcmp (temp_str, "lumi.ctrl_86plug"))
				{
					//printf("ht join\n");
					char str[200]={0};
					char str_url[200]={0};
					sprintf(str,"{\"address\":\"%d\",\"deviceType\":\"%s\",\"resourceSum\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,"PowerPanel_Mi",1,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
					sprintf(str_url,"127.0.0.1:%d/device/API/deviceReg",PORT_CLIENT);
					curl_easy_setopt(posturl, CURLOPT_URL, str_url);
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);	
					printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
					if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
					{
						fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
						  fclose(sp);
					}					
				}
				
			}
		}

		if(cid == 0 && len == 40 + 1 + 8 + 1)
		{
			char str[200]={0};
			char strtemp[200]={0};
			char str_url[200]={0};
			int i=0;
			sprintf(str,"{\"address\":\"%d\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
			
			sprintf(str_url,"127.0.0.1:%d/device/API/feedback/status",PORT_CLIENT);
			curl_easy_setopt(posturl, CURLOPT_URL, str_url);
			curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(posturl);
			printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
			if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
			{
				fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
				  fclose(sp);
			}
		}

		if(cid == 0x0c && len ==26 && endpoint == 2)
		{
			//float dianliang = rx[12-15]
			float dianliang=0;
			uint8_t * tempp = (uint8_t *)&dianliang;
			
			*tempp = rx[12];
			tempp++;*tempp = rx[13];
			tempp++;*tempp = rx[14];
			tempp++;*tempp = rx[15];
			char str[200]={0};
			char str_url[200]={0};
			sprintf(str,"{\"address\":\"%d\",\"indaddressex\":\"%d\",\"data\":\"%f\",\"type\":\"%s\",\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,1,dianliang,"power",rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
			sprintf(str_url,"127.0.0.1:%d/device/API/value",PORT_CLIENT);
			curl_easy_setopt(posturl, CURLOPT_URL, str_url);
			curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(posturl);	
			
			printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
			if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
			{
				fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
				  fclose(sp);
			}
		}		
		
 	
    break;

	case MXJ_XIAOMI1C:
		if(cid == 0 && ((len == 35 + 1 + 8 + 1)||(len == 51)||(len == 66)||(len == 74)))
		{
			char str[200]={0};
			char strtemp[200]={0};
			char str_url[200]={0};
			int i=0;
			sprintf(str,"{\"address\":\"%d\",\"status\":[{\"value\":\"2\"},{\"value\":\"2\"},{\"value\":\"2\"},{\"value\":\"2\"},{\"value\":\"2\"}],\"linkQuality\":\"%d\",\"macAddr\":\"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\"}",id,rx[len-1],rx[len-9],rx[len-8],rx[len-7],rx[len-6],rx[len-5],rx[len-4],rx[len-3],rx[len-2]);
			
			sprintf(str_url,"127.0.0.1:%d/device/API/feedback/status",PORT_CLIENT);
			curl_easy_setopt(posturl, CURLOPT_URL, str_url);
			curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(posturl);
			printf("[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);
			if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
			{
				fprintf(sp,"[%d-%d-%d %d:%d:%d] POST SEND url=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,str_url,str);			
				  fclose(sp);
			}
		}
	break;    
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
    pthread_t alarm_pr;
    pthread_t music_pr = NULL;
  struct MHD_Daemon *daemon;
  int i=0;
	printf_file("ZBCLIENT STARTING...\n");

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
	
   //printf("start server ...\n");

//if(pthread_create(&send_usart_pr,NULL,(void *) thread_send,NULL)!=0)
//	{
//		printf ("Create pthread error!\n");
//		return (1);
//	}

if(pthread_create(&alarm_pr,NULL,(void *) thread_alarm,NULL)!=0)
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



   pinMode (25, INPUT) ;
   pinMode (22, OUTPUT);
   pinMode (MUTE, OUTPUT);
   digitalWrite(MUTE,HIGH);
   pinMode (POWER_2530, OUTPUT);
	digitalWrite(POWER_2530,HIGH);
	
   pinMode (R1_LED, OUTPUT);
   pinMode (G1_LED, OUTPUT);
   pinMode (B1_LED, OUTPUT);
   pinMode (R2_LED, OUTPUT);
   pinMode (G2_LED, OUTPUT);
   pinMode (B2_LED, OUTPUT);
   pinMode (R3_LED, OUTPUT);
   pinMode (G3_LED, OUTPUT);
   pinMode (B3_LED, OUTPUT);
   pinMode (R4_LED, OUTPUT);
   pinMode (G4_LED, OUTPUT);
   pinMode (B4_LED, OUTPUT);
  //
  //MXJ_GetIdxMessage( 0xe768 );
  //sleep(1);
  //MXJ_GetIdxMessage( 0x968f );
  //sleep(1);

  	digitalWrite(R1_LED,LOW);
  	digitalWrite(G1_LED,LOW);
  	digitalWrite(B1_LED,LOW);
  	digitalWrite(R2_LED,LOW);
  	digitalWrite(G2_LED,LOW);
  	digitalWrite(B2_LED,LOW);
  	digitalWrite(R3_LED,LOW);
  	digitalWrite(G3_LED,LOW);
  	digitalWrite(B3_LED,LOW);
  	digitalWrite(R4_LED,LOW);
  	digitalWrite(G4_LED,LOW);
  	digitalWrite(B4_LED,LOW);

	softPwmCreate(B1_LED,0,100);
	softPwmCreate(B2_LED,0,100);
	softPwmCreate(B3_LED,0,100);
	softPwmCreate(B4_LED,0,100);
	softPwmCreate(R1_LED,0,100);
	softPwmCreate(R2_LED,0,100);
	softPwmCreate(R3_LED,0,100);
	softPwmCreate(R4_LED,0,100);
	softPwmCreate(G1_LED,0,100);
	softPwmCreate(G2_LED,0,100);
	softPwmCreate(G3_LED,0,100);
	softPwmCreate(G4_LED,0,100);
	if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) 
    { 
        printf( "ioctl error\n"); 
        return   1; 
    } 
    for(i=0;i<6;i++)
    {
    	mac[i]=ifreq.ifr_hwaddr.sa_data[i];
    }

	device_getversion();
  	usleep(500000);
  	printf_file("ZBCLIENT STARTED\n");
	
	time(&now);
	tblock = localtime(&now);
	sprintf(startTime,"[%d-%d-%d %d:%d:%d] ", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);

	static int flag_led = 1;

	char str[200];
	//printf_file("MUSIC VOL DOWN\n");

	
	int pwm=0;
	int pwmdir=1;
	int alarmnum=0;
	int alarmflag=1;

	while(1)
	{  
		static int alarm_register_flag=0;
		static int time_flag=0;
		if(alarm_flag == 1)
		{
			alarmnum++;
			if(alarmnum >= 10)
			{
				softPwmWrite(B1_LED,0);
				softPwmWrite(B2_LED,0);
				softPwmWrite(B3_LED,0);
				softPwmWrite(B4_LED,0);
				alarmnum = 0;
				alarmflag = 3 - alarmflag;
				if(alarmflag==1)
				{
					softPwmWrite(R1_LED,50);
					softPwmWrite(R2_LED,50);
					softPwmWrite(R3_LED,50);
					softPwmWrite(R4_LED,50);

				}
				else
				{
					softPwmWrite(R1_LED,0);
					softPwmWrite(R2_LED,0);
					softPwmWrite(R3_LED,0);
					softPwmWrite(R4_LED,0);

				}
			}
		}
		else
		{
			if(pwm==40)pwmdir=0;
			if(pwm==0)
				{
					pwmdir=1;
					sleep(1);
				}
			if(pwmdir == 1)pwm++;
			if(pwmdir == 0)pwm--;			
			softPwmWrite(B1_LED,pwm);
			softPwmWrite(B2_LED,pwm);
			softPwmWrite(B3_LED,pwm);
			softPwmWrite(B4_LED,pwm);
			if(shefang == 1)
			{
				softPwmWrite(R1_LED,pwm/2);
				softPwmWrite(R2_LED,pwm/2);
				softPwmWrite(R3_LED,pwm/2);
				softPwmWrite(R4_LED,pwm/2);
			}
			else
			{
				softPwmWrite(R1_LED,0);
				softPwmWrite(R2_LED,0);
				softPwmWrite(R3_LED,0);
				softPwmWrite(R4_LED,0);
			}
		}
		usleep(30000);
		///=====
		if(music_state == 1)
		{

			if(music_thread_alive == 0)
			{
				//printf("pthread_create: %lu \n",music_pr);
				

				if(pthread_create(&music_pr,NULL,(void *) thread_music,NULL)!=0)
				{
					music_state = 0;
					music_thread_alive = 0;
					music_sendState(music_state,0,0);
					printf ("Create pthread error!\n");
					return (1);
				}
				else
				{
					//music_state = 0;
					printf_file("PLAYING MUSIC\n");
					music_thread_alive = 1;
					music_sendState(music_state,0,0);
				}
			}
		}
		else if(music_state == 0)
		{
			if(music_thread_alive == 1)
			{
				
				//music_thread_alive = 1;
				if(pthread_kill(music_pr,0) == 0)
				{
									
				}
				else
				{
					//music_state = 1;
					//music_thread_alive = 1;
				}
				system("kill $(pidof mplayer)");
				printf_file("STOP MUSIC\n");
				digitalWrite(MUTE,HIGH);
				music_thread_alive = 0;	
				music_sendState(music_state ,0,0);
				//printf("pthread_kill: %lu \n",music_pr);
			}
		}


		if(vol_flag == 1)
		{
			vol_flag=3;
			vol+=2;
			if(vol>=100)vol = 100;	
			char str[200];
			printf_file("MUSIC VOL UP\n");
			sprintf(str,"amixer set 'PCM' %d%%",vol);
			system(str);	
			music_sendState(music_state,0,0);	
		}
		else if(vol_flag == 0)
		{
			vol-=2;
			vol_flag=3;
			if(vol<=20)vol = 20;
			char str[200];
			printf_file("MUSIC VOL DOWN\n");
			sprintf(str,"amixer set 'PCM' %d%%",vol);
			system(str);
			music_sendState(music_state,0,0);
		}
		
		///=====

		//VolCtrl(vol_flag);


		if(alarm_register_flag < 1000)
		{
			alarm_register_flag++;
		}
		else if(alarm_register_flag == 1000)
		{
			alarm_register_flag++;
			alarm_register();
			usleep(10000);
			music_register();
			usleep(10000);
			alarm_sendState(alarm_state1,alarm_state2);
			usleep(10000);
			music_sendState(music_state,0,0);
		}

		time_flag++;
		if(time_flag>=60000)
		{
			time_flag=0;
			alarm_sendState(alarm_state1,alarm_state2);
			usleep(10000);
			music_sendState(music_state,0,0);
		}
	}
	

	serialClose(usart_fd);
	return (0);
}

void thread_music(void)
{
	digitalWrite(MUTE,LOW);
	//while(1)
	{
		system("mplayer -playlist /home/pi/Music/music.lst -loop 0");
	}	
	//digitalWrite(MUTE,HIGH);
}

void thread_alarm(void)
{
	int oldshefang = shefang;
	while(1)
	{
		if(alarm_flag == 1)
		{
			digitalWrite(MUTE,LOW);
			system("mplayer /home/pi/alarm.wav");
			digitalWrite(MUTE,HIGH);
		}
		if(oldshefang!=shefang)
		{
			oldshefang = shefang;
			if(shefang==1)
			{
				digitalWrite(MUTE,LOW);
				system("mplayer /home/pi/she.wav");
				digitalWrite(MUTE,HIGH);
			}
			else
			{
				digitalWrite(MUTE,LOW);
				system("mplayer /home/pi/che.wav");
				digitalWrite(MUTE,HIGH);
			}
		}
		
		if(permitjoin == 1)
		{
			digitalWrite(22,LOW);
			permitjoin++;
		}
		if(permitjoin >1)
		{			
			permitjoin++;
		}		  
		
		if(permitjoin > 500)
		{
			permitjoin = 0;
			digitalWrite(22,HIGH);
		}
		
		usleep(1000);
	}
}


	//command len[1(len)+2(id)+1(cid)+1(ep)+datalength] idh idl ep data[]
	//zigbee data length + 5 = len
	//data[len] = len + 1
	//command is not use in zigbee msg,is only a flag between 2530 and pi in usart
void MXJ_SendIrCode( uint16_t id,uint8_t *data2,uint8_t len )
{
	uint8_t i=0;
  uint8_t data[8+len];//={0,len+7,(uint8_t)(id>>8),(uint8_t)id,1,endpoint,0,len,msg1,msg2,msg3};
  data[0] = 0;
  data[1] = len+6;
  data[2] = (uint8_t)(id>>8);
  data[3] = (uint8_t)id;
  data[4] = 1;
  data[5] = 1;
  data[6] = 0;
  
  for(i=0;i<len;i++)
  {
  	data[i+7] = data2[i];
  }
  send_usart(data,7+len);
}

void MXJ_LearnIrCode( uint16_t id )
{

  uint8_t data[9]={0,1+7,(uint8_t)(id>>8),(uint8_t)id,1,1,0,1,0};
  send_usart(data,8+1);
}

void MXJ_SendCtrlMessage( uint16_t id ,uint8_t endpoint ,uint8_t len,uint8_t msg1 , uint8_t msg2 , uint8_t msg3 )
{

  uint8_t data[11]={0,len+7,(uint8_t)(id>>8),(uint8_t)id,1,endpoint,0,len,msg1,msg2,msg3};
  send_usart(data,8+len);
}

void MXJ_SendRegisterMessage( uint16_t id, uint8_t state )
{
  uint8_t data[8]={0,7,(uint8_t)(id>>8),(uint8_t)id,1,1,88,0};
  if(state == MXJ_REGISTER_OK)
  {
    data[0]=3;
    data[6]=3;
    send_usart(data,8);
  }
  else if(state == MXJ_REGISTER_FAILED)
  {
    data[0]=4;
    data[6]=4;
    send_usart(data,8);
  }
}

void MXJ_SendPingMessage( uint16_t id )
{
  uint8_t data[8]={0,7,(uint8_t)(id>>8),(uint8_t)id,1,1,0x0f,0};//¨¨?a????1¡ë??¡ã??
  send_usart(data,8);
}

void MXJ_WriteCongifMessage( uint16_t id ,uint8_t data2)
{
  uint8_t data[9]={0,8,(uint8_t)(id>>8),(uint8_t)id,1,1,MXJ_CONFIG_WRITE,1,data2};//¨¨?a????1¡ë??¡ã??
  send_usart(data,9);
}

void MXJ_ReadCongifMessage( uint16_t id )
{
  uint8_t data[8]={0,7,(uint8_t)(id>>8),(uint8_t)id,1,1,MXJ_CONFIG_WRITE,0};//¨¨?a????1¡ë??¡ã??
  send_usart(data,8);
}



void MXJ_GetIdxMessage( uint16_t id )
{
  uint8_t data[8]={0,7,(uint8_t)(id>>8),(uint8_t)id,1,1,0x0b,0};//¨¨?a????1¡ë??¡ã??
  send_usart(data,8);
}

void MXJ_GetStateMessage( uint16_t id )
{
  uint8_t data[8]={0,7,(uint8_t)(id>>8),(uint8_t)id,1,1,5,0};//¡Á??¡§¨°?¨ºy?Y
  send_usart(data,8);
}


void printf_file(char * str)
{
	while(mutex_flag==1);
	mutex_flag = 1;
	time(&now);
	tblock = localtime(&now);

	if ((sp = fopen("/var/log/zbclient.log","a+")) != NULL)
	{
		fprintf(sp,"[%d-%d-%d %d:%d:%d] ", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
		fprintf(sp,str);			
		fclose(sp);
	}
	mutex_flag = 0;
}

void alarm_register(void)
{
	char tx1[12]={0x0e,0x0b,0xff,0xfe,0,0,mac[5],mac[4],mac[3],mac[2],mac[1],mac[0]};
	//0e 0b ac 36 20 71 b2 0c 00 4b 12 00 =12
	recieve_usart(tx1,12);
	char tx2[18]={0x02,0x11,0xff,0xfe,0x00,0x04,0x02,0x01,0x02,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],0,0,0xff};
	//02 11 ac 36 00 04 02 01 02 00 12 4b 00 0c b2 71 20 62 =18
	recieve_usart(tx2,18);
}

void alarm_sendState(uint8_t one,uint8_t two)
{
	char tx[19]={0x06,0x12,0xff,0xfe,0x00,0x04,0x06,0x02,one,two,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],0,0,0xff};
	//06 12 ac 36 00 04 06 02 01 00 00 12 4b 00 0c b2 71 20 53 = 19
	recieve_usart(tx,19);
}

void device_reset(void)
{
	uint8_t data[4]={0x55,3,0,2};
  	send_usart(data,4);
}
void device_factory(void)
{
	uint8_t data[4]={0x55,3,0,1};
  	send_usart(data,4);
}
void device_permit(void)
{
	uint8_t data[4]={0x55,3,0,3};
  	send_usart(data,4);
}
void device_getversion(void)
{
	uint8_t data[4]={0x55,3,0,4};
  	send_usart(data,4);
}
void time_response(uint16_t id)
{
	static uint8_t counter =0; 	
	uint32_t t = time(NULL);
	//printf("\n %u\n",t);
	t = t - 946655973;
	uint8_t *tempp = (uint8_t*)&t;
	uint8_t data[17]={0,16,(uint8_t)(id>>8),(uint8_t)id,0x0a,1,0x18,counter,1,0,0,0,0xe2,tempp[0],tempp[1],tempp[2],tempp[3]};  //cmd(unuse) [len]len-1 idh idl cid ep
	//printf("\n %u\n",t);
	if(counter ==255)
	{
		counter =0;
	}
	counter ++;
	mutex_flag = 0;
	send_usart(data,17);
	mutex_flag = 1;
}
void music_register(void)
{
	char tx1[12]={0x0e,0x0b,0,0x01,1,0,mac[5],mac[4],mac[3],mac[2],mac[1],mac[0]};
	//0e 0b ac 36 20 71 b2 0c 00 4b 12 00 =12
	recieve_usart(tx1,12);
	char tx2[18]={0x02,0x11,0,0x01,0x00,0x04,0x02,0x01,0x03,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],0,1,0xff};
	//02 11 ac 36 00 04 02 01 02 00 12 4b 00 0c b2 71 20 62 =18
	recieve_usart(tx2,18);
}

void music_sendState(uint8_t one,uint8_t two,uint8_t three)
{
	char tx[20]={0x06,0x13,0,0x01,0x00,0x04,0x06,0x03,one,two,three,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],0,1,0xff};
	//06 12 ac 36 00 04 06 02 01 00 00 12 4b 00 0c b2 71 20 53 = 19
	recieve_usart(tx,20);
}


void VolCtrl(void)//1=up,0=down
{
	static vol = 10;
	if(vol_flag == 1)
	{
		vol_flag=3;
		vol+=5;
		if(vol>=25)vol = 25;			
	}
	else if(vol_flag == 0)
	{
		vol-=5;
		vol_flag=3;
		if(vol<=5)vol = 5;
	}
	char str[200];
	sprintf(str,"amixer set 'Lineout volume control' %d",vol);
	system(str);
}
