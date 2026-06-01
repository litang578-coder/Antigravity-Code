/**
	************************************************************
	************************************************************
	************************************************************
	*	锟侥硷拷锟斤拷锟斤拷 	onenet.c
	*
	*	锟斤拷锟竭ｏ拷 		锟脚硷拷锟斤拷
	*
	*	锟斤拷锟节ｏ拷 		2017-05-08
	*
	*	锟芥本锟斤拷 		V1.1
	*
	*	说锟斤拷锟斤拷 		锟斤拷onenet平台锟斤拷锟斤拷锟捷斤拷锟斤拷锟接口诧拷
	*
	*	锟睫改硷拷录锟斤拷	V1.0锟斤拷协锟斤拷锟阶帮拷锟斤拷锟斤拷锟斤拷卸隙锟斤拷锟酵一锟斤拷锟侥硷拷锟斤拷锟斤拷锟揭诧拷同协锟斤拷涌诓锟酵锟斤拷
	*				V1.1锟斤拷锟结供统一锟接口癸拷应锟矫诧拷使锟矫ｏ拷锟斤拷锟捷诧拷同协锟斤拷锟侥硷拷锟斤拷锟斤拷装协锟斤拷锟斤拷氐锟斤拷锟斤拷荨锟?
	************************************************************
	************************************************************
	************************************************************
**/

// 锟斤拷片锟斤拷头锟侥硷拷锟斤拷锟斤拷锟斤拷STM32F10x系锟叫碉拷锟斤拷锟借定锟斤拷
#include "stm32f10x.h"

//锟斤拷锟斤拷锟借备
#include "esp8266.h"
#include "OLED.h"

#define ESP8266_ONENET_INFO "AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"

//协锟斤拷锟侥硷拷
#include "mqttkit.h"
#include "usart.h"
#include "mqttkit.h"
//#include "led.h"
//#include "brace_control.h"	

//锟姐法
#include "base64.h"
#include "hmac_sha1.h"
#include "cJSON.h"
extern float current;
extern float temp;
extern uint8_t g_battery_soc_upload;
#include "delay.h"
#include "IO_Init.h"
#include "mppt.h"
//#include "GUI.h"
//#include "Lcd_Driver.h"

//C锟斤拷
#include <string.h>
#include <stdio.h>


#define PROID			"dtk3h50J6V"

#define ACCESS_KEY		"ZXdtVzNvb29CRll3N1FsZUdiZlFxUmFaNUdpNVdyRnA="

#define DEVICE_NAME		"dachuang"

#define ONENET_UPLOAD_BUF_SIZE	384


char devid[16];

char key[48];

// 锟斤拷锟斤拷全锟街憋拷锟斤拷 (锟斤拷锟窖★拷锟斤拷锟斤拷 onenet.c)
_Bool g_braces_up_status = 0; // 默锟斤拷为 false (停止)
_Bool g_braces_down_status = 0; // 默锟斤拷为 false (停止)

extern unsigned char esp8266_buf[512];
// 锟斤拷 main.c 锟斤拷锟斤拷锟侥达拷锟斤拷锟斤拷/锟斤拷锟斤拷值
extern float volt;
extern float current;
extern uint8_t g_battery_soc_upload;  // 电池SOC（0~100），由main.c更新
extern float temp;
//extern u8 control_flag;
//extern uint32_t b_control,speed,heart_rate,steps;
/*
************************************************************
*	锟斤拷锟斤拷锟斤拷锟狡ｏ拷	OTA_UrlEncode
*
*	锟斤拷锟斤拷锟斤拷锟杰ｏ拷	sign锟斤拷要锟斤拷锟斤拷URL锟斤拷锟斤拷
*
*	锟斤拷诓锟斤拷锟斤拷锟?sign锟斤拷锟斤拷锟杰斤拷锟?
*
*	锟斤拷锟截诧拷锟斤拷锟斤拷	0-锟缴癸拷	锟斤拷锟斤拷-失锟斤拷
*
*	说锟斤拷锟斤拷		+			%2B
*				锟秸革拷		%20
*				/			%2F
*				?			%3F
*				%			%25
*				#			%23
*				&			%26
*				=			%3D
************************************************************
*/
static unsigned char OTA_UrlEncode(char *sign)
{

	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);
	
	if(sign == (void *)0 || sign_len < 28)
		return 1;
	
	for(; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;
	
	for(i = 0, j = 0; i < sign_len; i++)
	{
		switch(sign_t[i])
		{
			case '+':
				strcat(sign + j, "%2B");j += 3;
			break;
			
			case ' ':
				strcat(sign + j, "%20");j += 3;
			break;
			
			case '/':
				strcat(sign + j, "%2F");j += 3;
			break;
			
			case '?':
				strcat(sign + j, "%3F");j += 3;
			break;
			
			case '%':
				strcat(sign + j, "%25");j += 3;
			break;
			
			case '#':
				strcat(sign + j, "%23");j += 3;
			break;
			
			case '&':
				strcat(sign + j, "%26");j += 3;
			break;
			
			case '=':
				strcat(sign + j, "%3D");j += 3;
			break;
			
			default:
				sign[j] = sign_t[i];j++;
			break;
		}
	}
	
	sign[j] = 0;
	
	return 0;

}

/*
************************************************************
*	锟斤拷锟斤拷锟斤拷锟狡ｏ拷	OTA_Authorization
*
*	锟斤拷锟斤拷锟斤拷锟杰ｏ拷	锟斤拷锟斤拷Authorization
*
*	锟斤拷诓锟斤拷锟斤拷锟?ver锟斤拷锟斤拷锟斤拷锟斤拷姹撅拷牛锟斤拷锟斤拷诟锟绞斤拷锟侥壳帮拷锟街э拷指锟绞?2018-10-31"
*				res锟斤拷锟斤拷品id
*				et锟斤拷锟斤拷锟斤拷时锟戒，UTC锟斤拷值
*				access_key锟斤拷锟斤拷锟斤拷锟斤拷钥
*				dev_name锟斤拷锟借备锟斤拷
*				authorization_buf锟斤拷锟斤拷锟斤拷token锟斤拷指锟斤拷
*				authorization_buf_len锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷(锟街斤拷)
*
*	锟斤拷锟截诧拷锟斤拷锟斤拷	0-锟缴癸拷	锟斤拷锟斤拷-失锟斤拷
*
*	说锟斤拷锟斤拷		锟斤拷前锟斤拷支锟斤拷sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//锟斤拷锟斤拷签锟斤拷锟斤拷Base64锟斤拷锟斤拷锟斤拷 锟斤拷 URL锟斤拷锟斤拷锟斤拷
	char hmac_sha1_buf[64];							//锟斤拷锟斤拷签锟斤拷
	char access_key_base64[64];						//锟斤拷锟斤拷access_key锟斤拷Base64锟斤拷锟斤拷锟斤拷
	char string_for_signature[72];					//锟斤拷锟斤拷string_for_signature锟斤拷锟斤拷锟斤拷羌锟斤拷艿锟絢ey

//----------------------------------------------------锟斤拷锟斤拷锟较凤拷锟斤拷--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------锟斤拷access_key锟斤拷锟斤拷Base64锟斤拷锟斤拷----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
//	UsartPrintf(USART_DEBUG, "access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------锟斤拷锟斤拷string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
//	UsartPrintf(USART_DEBUG, "string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------锟斤拷锟斤拷-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
//	UsartPrintf(USART_DEBUG, "hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------锟斤拷锟斤拷锟杰斤拷锟斤拷锟斤拷锟紹ase64锟斤拷锟斤拷------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------锟斤拷Base64锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷URL锟斤拷锟斤拷---------------------------------------------------
	OTA_UrlEncode(sign_buf);
//	UsartPrintf(USART_DEBUG, "sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------锟斤拷锟斤拷Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
//	UsartPrintf(USART_DEBUG, "Token: %s\r\n", authorization_buf);
	
	return 0;

}

//==========================================================
//	锟斤拷锟斤拷锟斤拷锟狡ｏ拷	OneNET_RegisterDevice
//
//	锟斤拷锟斤拷锟斤拷锟杰ｏ拷	锟节诧拷品锟斤拷注锟斤拷一锟斤拷锟借备
//
//	锟斤拷诓锟斤拷锟斤拷锟?access_key锟斤拷锟斤拷锟斤拷锟斤拷钥
//				pro_id锟斤拷锟斤拷品ID
//				serial锟斤拷唯一锟借备锟斤拷
//				devid锟斤拷锟斤拷锟芥返锟截碉拷devid
//				key锟斤拷锟斤拷锟芥返锟截碉拷key
//
//	锟斤拷锟截诧拷锟斤拷锟斤拷	0-锟缴癸拷		1-失锟斤拷
//
//	说锟斤拷锟斤拷		
//==========================================================
_Bool OneNET_RegisterDevice(void)
{

	_Bool result = 1;
	unsigned short send_len = 11 + strlen(DEVICE_NAME);
	char *send_ptr = NULL, *data_ptr = NULL;
	
	char authorization_buf[144];													//锟斤拷锟杰碉拷key
	
	send_ptr = malloc(send_len + 240);
	if(send_ptr == NULL)
		return result;
	
	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80\r\n", "CONNECT"))
		delay_ms(500);
	
	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, NULL,
							authorization_buf, sizeof(authorization_buf), 1);
	
	snprintf(send_ptr, 240 + send_len, "POST /mqtt/v1/devices/reg HTTP/1.1\r\n"
					"Authorization:%s\r\n"
					"Host:ota.heclouds.com\r\n"
					"Content-Type:application/json\r\n"
					"Content-Length:%d\r\n\r\n"
					"{\"name\":\"%s\"}",
	
					authorization_buf, 11 + strlen(DEVICE_NAME), DEVICE_NAME);
	
	ESP8266_SendData((unsigned char *)send_ptr, strlen(send_ptr));
	
	/*
	{
	  "request_id" : "f55a5a37-36e4-43a6-905c-cc8f958437b0",
	  "code" : "onenet_common_success",
	  "code_no" : "000000",
	  "message" : null,
	  "data" : {
		"device_id" : "589804481",
		"name" : "mcu_id_43057127",
		
	"pid" : 282932,
		"key" : "indu/peTFlsgQGL060Gp7GhJOn9DnuRecadrybv9/XY="
	  }
	}
	*/
	
	data_ptr = (char *)ESP8266_GetIPD(250);							//锟饺达拷平台锟斤拷应
	
	if(data_ptr)
	{
		data_ptr = strstr(data_ptr, "device_id");
	}
	
	if(data_ptr)
	{
		char name[16];
		int pid = 0;
		
		if(sscanf(data_ptr, "device_id\" : \"%[^\"]\",\r\n\"name\" : \"%[^\"]\",\r\n\r\n\"pid\" : %d,\r\n\"key\" : \"%[^\"]\"", devid, name, &pid, key) == 4)
		{
			UsartPrintf(USART_DEBUG, "create device: %s, %s, %d, %s\r\n", devid, name, pid, key);
			result = 0;
		}
	}
	
	free(send_ptr);
	ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK");
	
	return result;

}

//==========================================================
//	锟斤拷锟斤拷锟斤拷锟狡ｏ拷	OneNet_DevLink
//
//	锟斤拷锟斤拷锟斤拷锟杰ｏ拷	锟斤拷onenet锟斤拷锟斤拷锟斤拷锟斤拷
//
//	锟斤拷诓锟斤拷锟斤拷锟?锟斤拷
//
//	锟斤拷锟截诧拷锟斤拷锟斤拷	1-锟缴癸拷	0-失锟斤拷
//
//	说锟斤拷锟斤拷		锟斤拷onenet平台锟斤拷锟斤拷锟斤拷锟斤拷
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协锟斤拷锟?

	unsigned char *dataPtr;
	
	char authorization_buf[160];
	
	_Bool status = 1;
	
	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, DEVICE_NAME,
								authorization_buf, sizeof(authorization_buf), 0);
	
//	UsartPrintf(USART_DEBUG, "OneNET_DevLink\r\n"
//							"NAME: %s,	PROID: %s,	KEY:%s\r\n"
//                        , DEVICE_NAME, PROID, authorization_buf);
	
	if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 128, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//锟较达拷平台
		
		dataPtr = ESP8266_GetIPD(250);									//锟饺达拷平台锟斤拷应
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:	UsartPrintf(USART_DEBUG, " WARN:锟斤拷锟斤拷锟斤拷锟接成癸拷\r\n");status = 0;break;
					
					case 1:UsartPrintf(USART_DEBUG, "WARN:	锟斤拷锟斤拷失锟杰ｏ拷协锟斤拷锟斤拷锟絓r\n");break;
					case 2:UsartPrintf(USART_DEBUG, "WARN:	锟斤拷锟斤拷失锟杰ｏ拷锟角凤拷锟斤拷clientid\r\n");break;
					case 3:UsartPrintf(USART_DEBUG, "WARN:	锟斤拷锟斤拷失锟杰ｏ拷锟斤拷锟斤拷锟斤拷失锟斤拷\r\n");break;
					case 4:UsartPrintf(USART_DEBUG, "WARN:	锟斤拷锟斤拷失锟杰ｏ拷锟矫伙拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟絓r\n");break;
					case 5:UsartPrintf(USART_DEBUG, "WARN:	锟斤拷锟斤拷失锟杰ｏ拷锟角凤拷锟斤拷锟斤拷(锟斤拷锟斤拷token锟角凤拷)\r\n");break;
					
					default:UsartPrintf(USART_DEBUG, "ERR:	锟斤拷锟斤拷失锟杰ｏ拷未知锟斤拷锟斤拷\r\n");break;
				}
			}
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//删锟斤拷
	}
	else
		UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");
	
	return status;
	
}
short OneNet_FillBuf(char *buf, unsigned short buf_size)
{
	int n;

	if(buf == (void *)0)
		return 0;
	if(buf_size == 0)
		return 0;

	n = snprintf(buf, buf_size, "{\"id\":\"123\",\"params\":{\"temp\":{\"value\":%.1f},\"volt\":{\"value\":%.2f},\"current\":{\"value\":%.3f},\"botton1\":{\"value\":%s},\"battery_soc\":{\"value\":%d},\"Relay_BAT\":{\"value\":%s},\"Um_comp\":{\"value\":%.2f},\"Im_comp\":{\"value\":%.3f},\"power\":{\"value\":%.3f}}}",
				 temp, volt, current, (Relay ? "true" : "false"), (int)g_battery_soc_upload, (Relay_BAT ? "true" : "false"), g_mppt.Um_comp, g_mppt.Im_comp, g_mppt.P_mpp_comp);

	if(n < 0)
		return 0;
	if(n >= buf_size)
		return 0;

	return (short)n;

}	

//==========================================================
//	锟斤拷锟斤拷锟斤拷锟狡ｏ拷	OneNet_SendData
//
//	锟斤拷锟斤拷锟斤拷锟杰ｏ拷	锟较达拷锟斤拷锟捷碉拷平台
//
//	锟斤拷诓锟斤拷锟斤拷锟?type锟斤拷锟斤拷锟斤拷锟斤拷锟捷的革拷式
//
//	锟斤拷锟截诧拷锟斤拷锟斤拷	锟斤拷
//
//	说锟斤拷锟斤拷		
//==========================================================
void OneNet_SendData(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协锟斤拷锟?
	
	char buf[ONENET_UPLOAD_BUF_SIZE];
	
	short body_len = 0, i = 0;
	
//	UsartPrintf(USART_DEBUG, "Tips:	OneNet_SendData-MQTT\r\n");
	
	memset(buf, 0, sizeof(buf));
	
	body_len = OneNet_FillBuf(buf, sizeof(buf));			//锟斤拷取锟斤拷前锟斤拷要锟斤拷锟酵碉拷锟斤拷锟斤拷锟斤拷锟斤拷锟杰筹拷锟斤拷
	
	if(body_len)
	{
		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)	//锟斤拷锟?
		{
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = buf[i];
			
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);					//锟较达拷锟斤拷锟捷碉拷平台
//			UsartPrintf(USART_DEBUG, "Send %d Bytes\r\n", mqttPacket._len);
			
			MQTT_DeleteBuffer(&mqttPacket);				//删锟斤拷
//			SendWaitForAck++;
		}
		else
			UsartPrintf(USART_DEBUG, "WARN:	EDP_NewBuffer Failed\r\n");
	}
	
}

//==========================================================
//	锟斤拷锟斤拷锟斤拷锟狡ｏ拷	OneNET_Publish
//
//	锟斤拷锟斤拷锟斤拷锟杰ｏ拷	锟斤拷锟斤拷锟斤拷息
//
//	锟斤拷诓锟斤拷锟斤拷锟?topic锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
//				msg锟斤拷锟斤拷息锟斤拷锟斤拷
//
//	锟斤拷锟截诧拷锟斤拷锟斤拷	锟斤拷
//
//	说锟斤拷锟斤拷		
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协锟斤拷锟?
	
	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//锟斤拷平台锟斤拷锟酵讹拷锟斤拷锟斤拷锟斤拷
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删锟斤拷
	}

}

//==========================================================
//	锟斤拷锟斤拷锟斤拷锟狡ｏ拷	OneNET_Subscribe
//
//	锟斤拷锟斤拷锟斤拷锟杰ｏ拷	锟斤拷锟斤拷
//
//	锟斤拷诓锟斤拷锟斤拷锟?锟斤拷
//
//	锟斤拷锟截诧拷锟斤拷锟斤拷	锟斤拷
//
//	说锟斤拷锟斤拷		
//==========================================================
void OneNET_Subscribe(void)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协锟斤拷锟?
	
	char topic_buf[2][80];
	const char *topics[] = {topic_buf[0], topic_buf[1]};

	snprintf(topic_buf[0], sizeof(topic_buf[0]), "$sys/%s/%s/thing/property/set", PROID, DEVICE_NAME);
	snprintf(topic_buf[1], sizeof(topic_buf[1]), "$sys/%s/%s/thing/property/post/reply", PROID, DEVICE_NAME);

	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf[0]);
	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf[1]);

	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, topics, 2, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//锟斤拷平台锟斤拷锟酵讹拷锟斤拷锟斤拷锟斤拷
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删锟斤拷
	}

}

//==========================================================
//	锟斤拷锟斤拷锟斤拷锟狡ｏ拷	OneNet_RevPro
//
//	锟斤拷锟斤拷锟斤拷锟杰ｏ拷	平台锟斤拷锟斤拷锟斤拷锟捷硷拷锟?
//
//	锟斤拷诓锟斤拷锟斤拷锟?dataPtr锟斤拷平台锟斤拷锟截碉拷锟斤拷锟斤拷
//
//	锟斤拷锟截诧拷锟斤拷锟斤拷	锟斤拷
//
//	说锟斤拷锟斤拷		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short topic_len = 0;
	unsigned short req_len = 0;
	
	unsigned char qos = 0;
	static unsigned short pkt_id = 0;
	
	unsigned char type = 0;
	
	short result = 0;

	char *dataPtr = NULL;
	char numBuf[10];
	int num = 0;
	
	cJSON *raw_json = NULL, *params_json = NULL, *speed_json, *control_json,
	*	braces_down_json,*braces_up_json, *relay_json, *relay_value_json;
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_PUBLISH:																//锟斤拷锟秸碉拷Publish锟斤拷息
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				char *data_ptr = NULL;
				
				UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
																	cmdid_topic, topic_len, req_payload, req_len);
				raw_json = cJSON_Parse(req_payload);
				if(raw_json != NULL)
				{
					params_json = cJSON_GetObjectItem(raw_json, "params");
					if(params_json != NULL)
					{
						relay_json = cJSON_GetObjectItem(params_json, "botton1");
						if(relay_json != NULL)
						{
							relay_value_json = cJSON_GetObjectItem(relay_json, "value");
							if(relay_value_json != NULL)
								relay_json = relay_value_json;
							
							if(relay_json->type == cJSON_True)
							{
								Relay = 1;
							}
							else if(relay_json->type == cJSON_False)
							{
								Relay = 0;
							}
						}

						relay_json = cJSON_GetObjectItem(params_json, "Relay_BAT");
						if(relay_json != NULL)
						{
							relay_value_json = cJSON_GetObjectItem(relay_json, "value");
							if(relay_value_json != NULL)
								relay_json = relay_value_json;

							if(relay_json->type == cJSON_True)
							{
								Relay_BAT = 1;
							}
							else if(relay_json->type == cJSON_False)
							{
								Relay_BAT = 0;
							}
						}

						if(cmdid_topic != NULL && strstr(cmdid_topic, "/thing/property/set") != NULL)
						{
							char reply_topic[80];
							char reply_msg[128];
							char id_buf[16];
							const char *request_id = "0";
							cJSON *id_json = cJSON_GetObjectItem(raw_json, "id");

							if(id_json != NULL)
							{
								if(id_json->type == cJSON_String && id_json->valuestring != NULL)
								{
									request_id = id_json->valuestring;
								}
								else if(id_json->type == cJSON_Number)
								{
									snprintf(id_buf, sizeof(id_buf), "%d", id_json->valueint);
									request_id = id_buf;
								}
							}

							snprintf(reply_topic, sizeof(reply_topic), "$sys/%s/%s/thing/property/set_reply", PROID, DEVICE_NAME);
							snprintf(reply_msg, sizeof(reply_msg), "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\"}", request_id);
							OneNET_Publish(reply_topic, reply_msg);
						}
					}
					
					cJSON_Delete(raw_json);
				}
			}

			
		break;

		case MQTT_PKT_PUBACK:														//锟斤拷锟斤拷Publish锟斤拷息锟斤拷平台锟截革拷锟斤拷Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
			{
//				SendWaitForAck = 0;
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");				
			}
		break;
		
		case MQTT_PKT_SUBACK:																//锟斤拷锟斤拷Subscribe锟斤拷息锟斤拷Ack
		
			if(MQTT_UnPacketSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe Err\r\n");
		
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//锟斤拷栈锟斤拷锟?
	
	if(result == -1)
		return;
	
	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}

extern u8 onenet_connected;

//==========================================================
//	函数名称：	OneNet_ReConnect
//
//	函数功能：	自动断线重连（非阻塞定时单次尝试）
//
//	输入参数：	无
//
//	输出参数：	无
//
//	说明：		当掉线后，由主循环定时（如每5秒）调用此函数尝试重连。
//				每次仅尝试一次，失败则立即退出，以防止卡死主循环导致
//				传感器读取和过温保护失效。
//==========================================================
void OneNet_ReConnect(void)
{
	
	UsartPrintf(USART_DEBUG, "Tips:	OneNet_ReConnect Attempt...\r\n");
	
	OLED_Clear();
	OLED_printf(0, 0, "Reconnecting...");

	// 1. 确保退出透传模式
	ESP8266_ExitTransparent();

	// 2. 发送 AT+CIPCLOSE 关闭可能残留的 TCP 连接
	ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK");

	// 3. 重新发起 TCP 连接（引入重试，给域名解析和握手预留时间以越过 500ms 命令超时限制）
	u8 tcp_retry = 0;
	while(ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT") != 0)
	{
		tcp_retry++;
		if(tcp_retry >= 10) // 尝试 10 次，每次间隔 500ms，最大等待 5 秒
		{
			UsartPrintf(USART_DEBUG, "WARN:	TCP Reconnect Failed\r\n");
			OLED_printf(0, 2, "TCP Connect Err");
			return;
		}
		delay_ms(500);
	}
	
	// 4. 重新进入透传模式
	if(ESP8266_EnterTransparent() != 0)
	{
		UsartPrintf(USART_DEBUG, "WARN:	Enter Transparent Failed\r\n");
		OLED_printf(0, 2, "Trans Mode Err");
		return;
	}

	// 5. 进行 MQTT 协议连接握手
	if(OneNet_DevLink() != 0)
	{
		UsartPrintf(USART_DEBUG, "WARN:	MQTT Reconnect Link Failed\r\n");
		OLED_printf(0, 2, "MQTT Link Err");
		return;
	}

	// 6. 重新订阅相关主题
	OneNET_Subscribe();

	onenet_connected = 1;
	
	OLED_Clear();
	OLED_printf(0, 0, "Reconnect OK!");
	delay_ms(500);
	OLED_Clear();
	
	UsartPrintf(USART_DEBUG, "Tips:	OneNet_ReConnect Success!\r\n");

}
