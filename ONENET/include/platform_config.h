#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

#include <stdint.h>


#define MAX_OBJECT_SUM 20   //最大支持obj个数，一般不用修改
#define DISCOVER_OBJECT_SUM 1   //discover obj个数，根据调试信息修改，一般为obj种类个数
#define OBSERVE_RESOURCE_SUM 20  //observe obj个数，根据调试信息修改
extern uint8_t Registered_Flag;

typedef struct _msgid_list_t
{
	uint16_t objid;
	uint8_t instid;
	uint32_t msgid;
}msgid_list_t;

#endif

