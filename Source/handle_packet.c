/*
 ============================================================================
 Name        : handle-packet.c
 Author      : Crazy Boy Feng
 Version     :
 Copyright   : GNU General Public License
 Description : Agent X One packet data handler in C, ANSI-style
 ============================================================================
 */
#include "agentx1.h"
static void set_property(unsigned char *buffer,int length,unsigned char type, unsigned char *data, int size) {//设置包携带信息
	int i;
	for(i=0x46;i<length;i=i+4+buffer[i+5]){
		if(buffer[i]==0x1a){
			i=i+2;
		}
		if(buffer[i+4]==type){
			memcpy(buffer+i+4+buffer[i+5]-size,data,size);
			break;
		}
	}
}
static unsigned char *get_code(unsigned char *buffer, int length) { //将字节的8位颠倒并取反
	unsigned char *result; //临时缓存
	result = malloc(length); //申请空间
	memcpy(result, buffer, length); //赋值
	int i;
	for (i = 0; i < length; i++) { //长度遍历
		unsigned char temp = 0; //临时字节
		int j;
		for (j = 0; j < 8; j++) { //颠倒一个字节的8位
			temp <<= 1;
			temp |= result[i] & 0x01;
			result[i] >>= 1;
		}
		result[i] = ~temp; //取反
	}
	return result;
}
unsigned int echo_key=0;	//心跳key
unsigned int echo_count=0;	//心跳计数
void get_success(unsigned char *data) {//初始化中继变量
	unsigned short int length;	//原消息长度
	memcpy(&length, data + 0x1a, 2);	//得到原长度
	length = ntohs(length);	//转换字节序
	memcpy(&echo_key, get_code(data + 0x1c + length + 0x69 + 0x18, 4), 4); //译码参数
	echo_key = ntohl(echo_key); //字节序
	printf("\tEcho work key: %d\n",echo_key);
	if(repeat_lan==0){//尚未初始化
		echo_count =0x0000102a;	//初始化
		printf("\tEcho work count: %d\n",echo_count);
	}
}
void set_echo(unsigned char *data) { //修改echo包
	echo_count++; //计数器
	unsigned int echo_temp; //参数
	unsigned char *echo_code; //数组
	echo_temp = htonl(echo_key + echo_count); //计算并更改字序
	echo_code = (unsigned char *) &echo_temp; //key+count
	memcpy(data + 0x18, get_code(echo_code, 4), 4); //译码
	echo_temp = htonl(echo_count); //更改字序
	echo_code = (unsigned char *) &echo_temp; //count
	memcpy(data + 0x22, get_code(echo_code, 4), 4); //译码
}
void get_interval(unsigned char *data) { //得到时间间隔
	long int time_temp;//临时时间节点
	time(&time_temp);//当前时间
	if (time_lan != 0 && interval > 10) {	//存在时间标志且已经获得间隔变量
		interval = (difftime(time_temp,time_lan) + interval) / 2;	//时间差的平均值
	} else if (time_lan != 0) {	//第一次取到间隔变量
		interval = difftime(time_temp,time_lan)+0;	//间隔时间
	}
	time(&time_lan);//当前时间
	printf("\tEcho repeat interval: %d\n",interval);
}
void get_echo(unsigned char *data) { //从echo得到中继变量
	unsigned int echo_kc; //key+count
	memcpy(&echo_kc, get_code(data + 0x18, 4), 4); //译码参数
	unsigned int echo_c; //count
	memcpy(&echo_c, get_code(data + 0x22, 4), 4); //译码参数
	unsigned int echo_count_temp = ntohl(echo_c); //新的count
	unsigned int echo_key_temp = ntohl(echo_kc) - echo_count_temp; //新的key
	repeat_lan =
			(echo_count_temp == echo_count + 1 && echo_key_temp == echo_key) ?
					1 : 0;	//所得变量正确1错误0
	echo_count = echo_count_temp;	//转移变量
	echo_key = echo_key_temp;	//转移变量
	printf("\tEcho work key: %d\n",echo_key);
	printf("\tEcho work count: %d\n",echo_count);
}
void set_head(unsigned char *data, int size) {	//修改加密位
	unsigned short int length;	//填充长度
	memcpy(&length, data + 0x10, 2);	//得到长度
	length = ntohs(length);	//字节序
	int size_temp = 0x12 + length;
	if (size >= size_temp + 0x17) {	//长度足够
		unsigned char *data_temp = data + size_temp;	//padding之后的指针
		memcpy(data_temp, get_code(data_temp, 0x17), 0x17);	//译码
		data_temp[0x04] = dhcp_wan == 0 ? 0x00 : 0x01;	// DHCP位，使用1、不使用0
		memcpy(data_temp + 0x05, &ip_wan, 4);	//IP
		memcpy(data_temp + 0x09, &netmask_wan, 4);	//MASK
		memcpy(data_temp + 0x0D, &gateway_wan, 4);	//GATEWAY
		memcpy(data_temp + 0x11, &dns_wan, 4);	//DNS
		memset(data_temp+0x15, 0, 2); //填充0准备校验和
		unsigned char dict[] = { 0x00, 0x00, 0x21, 0x10, 0x42, 0x20, 0x63, 0x30,
					0x84, 0x40, 0xA5, 0x50, 0xC6, 0x60, 0xE7, 0x70, 0x08, 0x81, 0x29,
					0x91, 0x4A, 0xA1, 0x6B, 0xB1, 0x8C, 0xC1, 0xAD, 0xD1, 0xCE, 0xE1,
					0xEF, 0xF1, 0x31, 0x12, 0x10, 0x02, 0x73, 0x32, 0x52, 0x22, 0xB5,
					0x52, 0x94, 0x42, 0xF7, 0x72, 0xD6, 0x62, 0x39, 0x93, 0x18, 0x83,
					0x7B, 0xB3, 0x5A, 0xA3, 0xBD, 0xD3, 0x9C, 0xC3, 0xFF, 0xF3, 0xDE,
					0xE3, 0x62, 0x24, 0x43, 0x34, 0x20, 0x04, 0x01, 0x14, 0xE6, 0x64,
					0xC7, 0x74, 0xA4, 0x44, 0x85, 0x54, 0x6A, 0xA5, 0x4B, 0xB5, 0x28,
					0x85, 0x09, 0x95, 0xEE, 0xE5, 0xCF, 0xF5, 0xAC, 0xC5, 0x8D, 0xD5,
					0x53, 0x36, 0x72, 0x26, 0x11, 0x16, 0x30, 0x06, 0xD7, 0x76, 0xF6,
					0x66, 0x95, 0x56, 0xB4, 0x46, 0x5B, 0xB7, 0x7A, 0xA7, 0x19, 0x97,
					0x38, 0x87, 0xDF, 0xF7, 0xFE, 0xE7, 0x9D, 0xD7, 0xBC, 0xC7, 0xC4,
					0x48, 0xE5, 0x58, 0x86, 0x68, 0xA7, 0x78, 0x40, 0x08, 0x61, 0x18,
					0x02, 0x28, 0x23, 0x38, 0xCC, 0xC9, 0xED, 0xD9, 0x8E, 0xE9, 0xAF,
					0xF9, 0x48, 0x89, 0x69, 0x99, 0x0A, 0xA9, 0x2B, 0xB9, 0xF5, 0x5A,
					0xD4, 0x4A, 0xB7, 0x7A, 0x96, 0x6A, 0x71, 0x1A, 0x50, 0x0A, 0x33,
					0x3A, 0x12, 0x2A, 0xFD, 0xDB, 0xDC, 0xCB, 0xBF, 0xFB, 0x9E, 0xEB,
					0x79, 0x9B, 0x58, 0x8B, 0x3B, 0xBB, 0x1A, 0xAB, 0xA6, 0x6C, 0x87,
					0x7C, 0xE4, 0x4C, 0xC5, 0x5C, 0x22, 0x2C, 0x03, 0x3C, 0x60, 0x0C,
					0x41, 0x1C, 0xAE, 0xED, 0x8F, 0xFD, 0xEC, 0xCD, 0xCD, 0xDD, 0x2A,
					0xAD, 0x0B, 0xBD, 0x68, 0x8D, 0x49, 0x9D, 0x97, 0x7E, 0xB6, 0x6E,
					0xD5, 0x5E, 0xF4, 0x4E, 0x13, 0x3E, 0x32, 0x2E, 0x51, 0x1E, 0x70,
					0x0E, 0x9F, 0xFF, 0xBE, 0xEF, 0xDD, 0xDF, 0xFC, 0xCF, 0x1B, 0xBF,
					0x3A, 0xAF, 0x59, 0x9F, 0x78, 0x8F, 0x88, 0x91, 0xA9, 0x81, 0xCA,
					0xB1, 0xEB, 0xA1, 0x0C, 0xD1, 0x2D, 0xC1, 0x4E, 0xF1, 0x6F, 0xE1,
					0x80, 0x10, 0xA1, 0x00, 0xC2, 0x30, 0xE3, 0x20, 0x04, 0x50, 0x25,
					0x40, 0x46, 0x70, 0x67, 0x60, 0xB9, 0x83, 0x98, 0x93, 0xFB, 0xA3,
					0xDA, 0xB3, 0x3D, 0xC3, 0x1C, 0xD3, 0x7F, 0xE3, 0x5E, 0xF3, 0xB1,
					0x02, 0x90, 0x12, 0xF3, 0x22, 0xD2, 0x32, 0x35, 0x42, 0x14, 0x52,
					0x77, 0x62, 0x56, 0x72, 0xEA, 0xB5, 0xCB, 0xA5, 0xA8, 0x95, 0x89,
					0x85, 0x6E, 0xF5, 0x4F, 0xE5, 0x2C, 0xD5, 0x0D, 0xC5, 0xE2, 0x34,
					0xC3, 0x24, 0xA0, 0x14, 0x81, 0x04, 0x66, 0x74, 0x47, 0x64, 0x24,
					0x54, 0x05, 0x44, 0xDB, 0xA7, 0xFA, 0xB7, 0x99, 0x87, 0xB8, 0x97,
					0x5F, 0xE7, 0x7E, 0xF7, 0x1D, 0xC7, 0x3C, 0xD7, 0xD3, 0x26, 0xF2,
					0x36, 0x91, 0x06, 0xB0, 0x16, 0x57, 0x66, 0x76, 0x76, 0x15, 0x46,
					0x34, 0x56, 0x4C, 0xD9, 0x6D, 0xC9, 0x0E, 0xF9, 0x2F, 0xE9, 0xC8,
					0x99, 0xE9, 0x89, 0x8A, 0xB9, 0xAB, 0xA9, 0x44, 0x58, 0x65, 0x48,
					0x06, 0x78, 0x27, 0x68, 0xC0, 0x18, 0xE1, 0x08, 0x82, 0x38, 0xA3,
					0x28, 0x7D, 0xCB, 0x5C, 0xDB, 0x3F, 0xEB, 0x1E, 0xFB, 0xF9, 0x8B,
					0xD8, 0x9B, 0xBB, 0xAB, 0x9A, 0xBB, 0x75, 0x4A, 0x54, 0x5A, 0x37,
					0x6A, 0x16, 0x7A, 0xF1, 0x0A, 0xD0, 0x1A, 0xB3, 0x2A, 0x92, 0x3A,
					0x2E, 0xFD, 0x0F, 0xED, 0x6C, 0xDD, 0x4D, 0xCD, 0xAA, 0xBD, 0x8B,
					0xAD, 0xE8, 0x9D, 0xC9, 0x8D, 0x26, 0x7C, 0x07, 0x6C, 0x64, 0x5C,
					0x45, 0x4C, 0xA2, 0x3C, 0x83, 0x2C, 0xE0, 0x1C, 0xC1, 0x0C, 0x1F,
					0xEF, 0x3E, 0xFF, 0x5D, 0xCF, 0x7C, 0xDF, 0x9B, 0xAF, 0xBA, 0xBF,
					0xD9, 0x8F, 0xF8, 0x9F, 0x17, 0x6E, 0x36, 0x7E, 0x55, 0x4E, 0x74,
					0x5E, 0x93, 0x2E, 0xB2, 0x3E, 0xD1, 0x0E, 0xF0, 0x1E };
		int i;
		for (i = 0; i < 0x15; i++) {//计算校验和
			int temp = data_temp[0x15] ^ data_temp[i];
			data_temp[0x15] = data_temp[0x16] ^ dict[temp * 2 + 1];
			data_temp[0x16] = dict[temp * 2];
		}
		memcpy(data_temp, get_code(data_temp, 0x17), 0x17);	//译码
		size_temp=size-size_temp;//从padding开始到最后的长度
		set_property(data_temp,size_temp,0x18,&dhcp_wan,1);//XXX dhcp 使用1 不使用0
		set_property(data_temp,size_temp,0x2d,mac_wan,6);//修改发包mac
	}
}
int set_success(unsigned char *data, int size) {	//修改携带信息
	unsigned char data_temp[1024];//临时转换空间
	memcpy(data_temp,data,size);//备份原数据
	unsigned short int length_temp = 173;	//新消息长度
	unsigned short int length_temp_net = htons(length_temp);	//新长度转换字节序
	unsigned char *length_temp_buf = (unsigned char *) &length_temp_net; //转换成数组
	memcpy(data + 0x1a, length_temp_buf, 2);	//将新长度写入
	unsigned char info_temp[] = { 0xd5, 0xfd, 0xd4, 0xda, 0xca, 0xb9, 0xd3,
				0xc3, 0xd7, 0xd4, 0xd3, 0xc9, 0xc8, 0xed, 0xbc, 0xfe, 0xa1, 0xb0,
				0xd2, 0xbb, 0xba, 0xc5, 0xcc, 0xd8, 0xb9, 0xa4, 0xa1, 0xb1, 0xb4,
				0xfa, 0xc0, 0xed, 0xc8, 0xcf, 0xd6, 0xa4, 0xa1, 0xa3, 0xc8, 0xe7,
				0xd3, 0xd0, 0xd2, 0xe2, 0xbc, 0xfb, 0xbb, 0xf2, 0xbd, 0xa8, 0xd2,
				0xe9, 0xbb, 0xb6, 0xd3, 0xad, 0xb7, 0xb4, 0xc0, 0xa1, 0xa3, 0xba,
				0xd6, 0xf7, 0xd2, 0xb3, 0x20, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
				0x2f, 0x72, 0x65, 0x66, 0x2e, 0x73, 0x6f, 0x2f, 0x68, 0x30, 0x67,
				0x7a, 0x33, 0x20, 0x47, 0x4e, 0x55, 0x20, 0xcd, 0xa8, 0xd3, 0xc3,
				0xb9, 0xab, 0xb9, 0xb2, 0xd0, 0xed, 0xbf, 0xc9, 0xd6, 0xa4, 0x20,
				0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x72, 0x65, 0x66, 0x2e,
				0x73, 0x6f, 0x2f, 0x69, 0x66, 0x34, 0x79, 0x68, 0x20, 0xb4, 0xcb,
				0xc8, 0xed, 0xcf, 0xd7, 0xd3, 0xe8, 0xce, 0xd2, 0xb5, 0xc4, 0xc5,
				0xf3, 0xd3, 0xd1, 0xa3, 0xac, 0xd2, 0xd4, 0xcd, 0xec, 0xc7, 0xe0,
				0xb4, 0xba, 0xb2, 0xbb, 0xc9, 0xe1, 0xd6, 0xe7, 0xd2, 0xb9, 0xa1,
				0xad, 0xa1, 0xad, 0x20, 0x40, 0xbf, 0xf1, 0xc4, 0xd0, 0xb7, 0xe7,
				0x00 };	//转换信息串
	memcpy(data + 0x1c, info_temp, length_temp);	//将新消息复制到包中
	unsigned short int length;	//原消息长度
	memcpy(&length, data_temp + 0x1a, 2);	//得到原长度
	length=ntohs(length);
	unsigned int head = 0x1c + length;	//取消息之后部分的指针
	memcpy(data + 0x1c + length_temp, data_temp + head, size - head);//将消息后部分复制到包中
	return size - length + length_temp;
}
