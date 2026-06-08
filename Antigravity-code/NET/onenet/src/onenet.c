/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名称： 	onenet.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.1
	*
	*	说明： 		与onenet平台数据交互接口部
	*
	*	修改记录：	V1.0：协议封装与物理接口隔离，可在不同协议串口间切换
	*				V1.1：提供统一接口给应用层使用，屏蔽底层协议细节。
	************************************************************
	************************************************************
	************************************************************
**/

// 单片机头文件，包含STM32F10x系列的寄存器定义
#include "stm32f10x.h"

// 外设驱动
#include "esp8266.h"
#include "OLED.h"

#define ESP8266_ONENET_INFO "AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"

// 协议文件
#include "mqttkit.h"
#include "usart.h"
#include "mqttkit.h"

// 算法与应用
#include "base64.h"
#include "hmac_sha1.h"
#include "cJSON.h"
extern float current;
extern float temp;
extern uint8_t g_battery_soc_upload;
#include "delay.h"
#include "IO_Init.h"
#include "mppt.h"

// C库
#include <string.h>
#include <stdio.h>


#define PROID			"dtk3h50J6V"

#define ACCESS_KEY		"ZXdtVzNvb29CRll3N1FsZUdiZlFxUmFaNUdpNVdyRnA="

#define DEVICE_NAME		"dachuang"

#define ONENET_UPLOAD_BUF_SIZE	384


char devid[16];

char key[48];

// 控制全局变量 (已选择移至 onenet.c)
_Bool g_braces_up_status = 0; // 默认为 false (停止)
_Bool g_braces_down_status = 0; // 默认为 false (停止)

extern unsigned char esp8266_buf[512];
// 由 main.c 更新的测量/控制值
extern float volt;
extern float current;
extern uint8_t g_battery_soc_upload;  // 电池SOC（0~100），由main.c更新
extern float temp;

/*
************************************************************
*	函数名称：	OTA_UrlEncode
*
*	函数功能：	sign需要进行URL编码
*
*	输入参数：	sign：待编码字符串
*
*	输出参数：	0-成功 其他-失败
*
*	说明：		+			%2B
*				空格		%20
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
*	函数名称：	OneNET_Authorization
*
*	函数功能：	生成Authorization
*
*	输入参数：	ver：版本号，目前支持格式 "2018-10-31"
*				res：产品id
*				et：失效时间，UTC秒值
*				access_key：产品主密钥
*				dev_name：设备名
*				authorization_buf：保存token的指针
*				authorization_buf_len：保存token的缓冲区长度(字节)
*
*	输出参数：	0-成功	其他-失败
*
*	说明：		目前仅支持sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//用于保存签名经过Base64编码及URL编码后的结果
	char hmac_sha1_buf[64];							//用于保存签名
	char access_key_base64[64];						//用于保存access_key的Base64解码结果
	char string_for_signature[72];					//用于保存签名方法中的待加密key

//----------------------------------------------------参数校验--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------对access_key进行Base64解码----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
	
//----------------------------------------------------构造string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
	
//----------------------------------------------------加密计算签名-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
//----------------------------------------------------将加密结果进行Base64编码------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------将Base64编码结果进行URL编码---------------------------------------------------
	OTA_UrlEncode(sign_buf);
	
//----------------------------------------------------构造Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
	
	return 0;

}

//==========================================================
//	函数名称：	OneNET_RegisterDevice
//
//	函数功能：	在产品下注册一个设备
//
//	输入参数：	无
//
//	输出参数：	0-成功		1-失败
//
//	说明：		
//==========================================================
_Bool OneNET_RegisterDevice(void)
{

	_Bool result = 1;
	unsigned short send_len = 11 + strlen(DEVICE_NAME);
	char *send_ptr = NULL, *data_ptr = NULL;
	
	char authorization_buf[144];													//生成的签名token
	
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
	
	data_ptr = (char *)ESP8266_GetIPD(250);							//等待平台响应
	
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
//	函数名称：	OneNet_DevLink
//
//	函数功能：	连接onenet平台
//
//	输入参数：	无
//
//	输出参数：	1-成功	0-失败
//
//	说明：		与onenet平台建立连接
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包
	unsigned char *dataPtr;
	
	char authorization_buf[160];
	
	_Bool status = 1;
	
	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, DEVICE_NAME,
								authorization_buf, sizeof(authorization_buf), 0);
	
	if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 128, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//上传至平台
		
		dataPtr = ESP8266_GetIPD(250);									//等待平台响应
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:	UsartPrintf(USART_DEBUG, " WARN:连接建立成功\r\n");status = 0;break;
					
					case 1:UsartPrintf(USART_DEBUG, "WARN:	连接失败，协议版本错误\r\n");break;
					case 2:UsartPrintf(USART_DEBUG, "WARN:	连接失败，非法的clientid\r\n");break;
					case 3:UsartPrintf(USART_DEBUG, "WARN:	连接失败，服务器不可用\r\n");break;
					case 4:UsartPrintf(USART_DEBUG, "WARN:	连接失败，用户名或密码错误\r\n");break;
					case 5:UsartPrintf(USART_DEBUG, "WARN:	连接失败，非法签名(比如token非法)\r\n");break;
					
					default:UsartPrintf(USART_DEBUG, "ERR:	连接失败，未知错误\r\n");break;
				}
			}
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//释放内存
	}
	else
		UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");
	
	return status;
	
}

//==========================================================
//	函数名称：	OneNet_FillBuf
//
//	函数功能：	填充上传数据的 JSON 缓存区
//
//	输入参数：	buf: 待写入数据的缓存指针
//				buf_size: 缓存大小
//
//	输出参数：	填充的长度
//==========================================================
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
//	函数名称：	OneNet_SendData
//
//	函数功能：	上传数据到平台
//
//	输入参数：	无
//
//	输出参数：	无
//
//	说明：		
//==========================================================
void OneNet_SendData(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包
	
	char buf[ONENET_UPLOAD_BUF_SIZE];
	
	short body_len = 0, i = 0;
	
	memset(buf, 0, sizeof(buf));
	
	body_len = OneNet_FillBuf(buf, sizeof(buf));			//获取当前需要发送的数据的JSON内容的总长度
	
	if(body_len)
	{
		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)	//打包数据
		{
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = buf[i];
			
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);					//上传数据到平台
			
			MQTT_DeleteBuffer(&mqttPacket);				//释放内存
		}
		else
			UsartPrintf(USART_DEBUG, "WARN:	EDP_NewBuffer Failed\r\n");
	}
	
}

//==========================================================
//	函数名称：	OneNET_Publish
//
//	函数功能：	发布消息
//
//	输入参数：	topic：发布的主题
//				msg：消息内容
//
//	输出参数：	无
//
//	说明：		
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送发布请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//释放内存
	}

}

//==========================================================
//	函数名称：	OneNET_Subscribe
//
//	函数功能：	订阅主题
//
//	输入参数：	无
//
//	输出参数：	无
//
//	说明：		
//==========================================================
void OneNET_Subscribe(void)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	char topic_buf[2][80];
	const char *topics[] = {topic_buf[0], topic_buf[1]};

	snprintf(topic_buf[0], sizeof(topic_buf[0]), "$sys/%s/%s/thing/property/set", PROID, DEVICE_NAME);
	snprintf(topic_buf[1], sizeof(topic_buf[1]), "$sys/%s/%s/thing/property/post/reply", PROID, DEVICE_NAME);

	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf[0]);
	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf[1]);

	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, topics, 2, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//释放内存
	}

}

//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台接收数据解析与处理
//
//	输入参数：	cmd: 接收到的命令数据流指针
//
//	输出参数：	无
//
//	说明：		
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

	cJSON *raw_json = NULL, *params_json = NULL, *relay_json, *relay_value_json;
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_PUBLISH:																//接收到Publish消息
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
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

		case MQTT_PKT_PUBACK:														//发送Publish消息后平台回复的Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
			{
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");				
			}
		break;
		
		case MQTT_PKT_SUBACK:																//发送Subscribe消息后平台回复的Ack
		
			if(MQTT_UnPacketSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe Err\r\n");
		
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//清空缓存
	
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
