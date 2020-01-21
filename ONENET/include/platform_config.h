#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

#include <stdint.h>


#define MAX_OBJECT_SUM 20   //���֧��obj������һ�㲻���޸�
#define DISCOVER_OBJECT_SUM 1   //discover obj���������ݵ�����Ϣ�޸ģ�һ��Ϊobj�������
#define OBSERVE_RESOURCE_SUM 20  //observe obj���������ݵ�����Ϣ�޸�
extern uint8_t Registered_Flag;

typedef struct _msgid_list_t
{
	uint16_t objid;
	uint8_t instid;
	uint32_t msgid;
}msgid_list_t;

#endif

