/**
 * Copyright (c) 2017 China Mobile IOT.
 * All rights reserved.
**/

#include "internal.h"
#include "stdio.h"
#include "bcxx.h"
#include "platform_config.h"
#include "led.h"


msgid_list_t msgid_list[OBSERVE_RESOURCE_SUM]={0};
uint8_t msgid_list_pos = 0;
uint8_t Registered_Flag=0;

uint8_t discovered_num = 0;

uint8_t Attrcount = 0;
uint8_t Actcount = 0;
char insBitmap[10];



int nbiot_device_create(nbiot_device_t           **dev,
                        int                      life_time,
                        nbiot_write_callback_t   write_func,
                        nbiot_read_callback_t    read_func,
                        nbiot_execute_callback_t execute_func)
{
	nbiot_device_t *tmp;

	tmp = (nbiot_device_t*)nbiot_malloc(sizeof(nbiot_device_t));

	if ( !tmp )
	{
		return NBIOT_ERR_NO_MEMORY;
	}
	else
	{
		nbiot_memzero(tmp,sizeof(nbiot_device_t));
	}

	*dev = tmp;

	tmp->next_mid = nbiot_rand();
    tmp->first_mid = tmp->next_mid;
	tmp->state = STATE_DEREGISTERED;
    tmp->life_time = life_time;
	tmp->registraction = nbiot_time();
	tmp->nodes = NULL;
	tmp->observes = NULL;
	tmp->transactions = NULL;
    tmp->write_func = write_func;
    tmp->read_func = read_func;
    tmp->execute_func = execute_func;

	bcxx_create();

	return NBIOT_ERR_OK;
}


static void device_free_nodes(nbiot_device_t *dev)
{
	uint8_t delobj = 0;
    nbiot_node_t *obj;
    nbiot_node_t *inst;

    obj = dev->nodes;

    while (obj)
    {
        inst = (nbiot_node_t*)obj->data;

		delobj = 1;
		
        while ( inst )
        {
            NBIOT_LIST_FREE( inst->data );
			
			if(delobj == 1)
			{
				bcxx_delobj( obj->id);
			}

            inst = inst->next;
			
			delobj = 0;
        }

        NBIOT_LIST_FREE( obj->data );
        obj = obj->next;
    }

    NBIOT_LIST_FREE(dev->nodes);
    dev->nodes = NULL;
}

static void device_free_observes(nbiot_device_t *dev)
{
    nbiot_observe_t *obj;
    nbiot_observe_t *inst;

    obj = dev->observes;

    while ( obj )
    {
        inst = (nbiot_observe_t*)obj->list;

        while ( inst )
        {
            NBIOT_LIST_FREE( inst->list );
            inst = inst->next;
        }

        NBIOT_LIST_FREE( obj->list );
        obj = obj->next;
    }

    NBIOT_LIST_FREE( dev->observes );
    dev->observes = NULL;
}

static void device_free_transactions(nbiot_device_t *dev)
{
    nbiot_transaction_t *transaction;

    transaction = dev->transactions;

    while (transaction)
    {
        nbiot_free(transaction->buffer);
        transaction = transaction->next;
    }

    NBIOT_LIST_FREE(dev->transactions);
    dev->transactions = NULL;
}

static void device_free_msgid(void)
{
	uint8_t i = 0;
	
    for(i = 0; i < OBSERVE_RESOURCE_SUM; i ++)
	{
		msgid_list[i].objid = 0;
		msgid_list[i].instid = 0;
		msgid_list[i].msgid = 0;
	}
	
	msgid_list_pos = 0;
}

void nbiot_device_destroy(nbiot_device_t *dev)
{
    device_free_transactions(dev);
    device_free_observes(dev);
    device_free_nodes(dev);
    nbiot_free(dev);
	
	Attrcount = 0;
	Actcount = 0;
	memset(insBitmap,0,10);
	
	device_free_msgid();
	
	discovered_num = 0;
}

static void handle_read(nbiot_device_t    *dev,
                        nbiot_uri_t       *uri,
                        uint8_t           *buffer,
                        size_t            buffer_len)
{
	int err = NBIOT_ERR_OK;
	
    do
    {
        int ret;
        nbiot_node_t *node;

        node = nbiot_node_find(dev, uri);

        if (!node)
        {
            break;
        }

		if(dev->read_func!=NULL)
			(dev->read_func)(uri->objid,uri->instid,uri->resid,node->data);

        ret = nbiot_node_read(node,
                              uri,
				              uri->flag,
                              buffer,
				              buffer_len,
                              false);

		err = nbiot_send_buffer(uri,buffer,ret,false);
		
		if(err == NBIOT_ERR_ERROR)
		{
			dev->state = STATE_NOTIFY_FAILED;
		}
    }
	while(0);
}

static void handle_write( nbiot_device_t         *dev,
                          nbiot_uri_t            *uri,
                          uint16_t               ackid,
                          uint8_t                *buffer,
                          size_t                 buffer_len,
                          nbiot_write_callback_t write_func )
{
    do
    {
        nbiot_node_t *node;

        node = nbiot_node_find(dev, uri);

        if ( !node )
        {
            bcxx_write_rsp(2, ackid);
            break;
        }

        /* write */
        nbiot_node_write( node,
                          uri,
                          ackid,
                          buffer,
                          buffer_len,
                          write_func );
    }
	while(0);
}

static void handle_execute( nbiot_device_t           *dev,
                            nbiot_uri_t              *uri,
                            uint16_t                 ackid,
                            uint8_t                  *buffer,
                            size_t                   buffer_len,
                            nbiot_execute_callback_t execute_func )
{
	do
    {
        nbiot_node_t *node;
        nbiot_value_t *data;

        if (!(uri->flag&NBIOT_SET_RESID))
        {
            break;
        }

        node = nbiot_node_find(dev, uri);
        if (!node)
        {
            break;
        }

        data = (nbiot_value_t*)node->data;

        if (!(data->flag&NBIOT_EXECUTABLE))
        {
            bcxx_execute_rsp(0,ackid);

            break;
        }

        if ( execute_func )
        {
            execute_func(uri->objid,
                         uri->instid,
                         uri->resid,
                         (nbiot_value_t*)node->data,
                         buffer,
                         buffer_len);
        }

		bcxx_execute_rsp(2,ackid);

    }
	while (0);
}



static void handle_observe( nbiot_device_t    *dev,
                            const nbiot_uri_t *uri)
{
	nbiot_observe_t *obs = NULL;

	do
    {
        nbiot_node_t *node;

//		bcxx_observe_rsp(1,*uri);
        node = nbiot_node_find(dev, uri);

        if (!node)
        {
            break;
        }

        obs = nbiot_observe_add(dev,uri);

		if (!obs)
			break;
    }
	while(0);
}

static void handle_discover(const nbiot_uri_t *uri,
                            size_t            lenth,
                            char              *value)
{
	do
	{
		discovered_num ++;

		if(discovered_num == DISCOVER_OBJECT_SUM)
		{
			Registered_Flag = 1;
			discovered_num = 0;
		}

		bcxx_discover_rsp(uri,lenth,value);

	}
	while(0);
}

static void handle_request( nbiot_device_t    *dev,
                            uint16_t           code,
                            uint8_t           *buffer,
                            size_t             buffer_len,
                            size_t             max_buffer_len )
{
	nbiot_uri_t uri;
	uint16_t  msgid;
	uint16_t  length = 0;
	char tmp[10],i = 0;
	char *msg = NULL;
	int msg_len = 0;
	uint8_t w_r_e = 0;
	
	msg = strstr((char *)buffer,"+MIPLWRITE");
	
	if((msg = strstr((char *)buffer,"+MIPLWRITE")) != NULL)
	{
		w_r_e = 0;
	}
	else if((msg = strstr((char *)buffer,"+MIPLREAD")) != NULL)
	{
		w_r_e = 1;
	}
	else if((msg = strstr((char *)buffer,"+MIPLEXECUTE")) != NULL)
	{
		w_r_e = 2;
	}

	msg = strstr((char *)buffer,": 0,");

	if(msg == NULL)
		return;

	msg = msg + 4;

	while(*msg != ',')
	tmp[i ++] = *(msg ++);
	tmp[i] = '\0';
	i = 0;
	msg = msg + 1;
	msgid = uri.msgid = nbiot_atoi(tmp,strlen(tmp));

	while(*msg != ',')
	tmp[i ++] = *(msg ++);
	tmp[i] = '\0';
	i = 0;
	msg = msg + 1;
	uri.objid = nbiot_atoi(tmp,strlen(tmp));

	while(*msg != ',')
	tmp[i ++] = *(msg++);
	tmp[i] = '\0';
	i = 0;
	msg = msg + 1;
	uri.instid = nbiot_atoi(tmp,strlen(tmp));

	while((*msg != ',') && (*msg != 0x0D))
	tmp[i ++] = *(msg ++);
	tmp[i] = '\0';
	i = 0;
	msg = msg + 1;
	uri.resid = nbiot_atoi(tmp,strlen(tmp));
	
	if(w_r_e == 0)
	{
		while(*msg != ',')
		tmp[i ++] = *(msg++);
		tmp[i] = '\0';
		i = 0;
		msg = msg + 1;
		msg_len = nbiot_atoi(tmp,strlen(tmp));
		
		while(*msg != ',')
		tmp[i ++] = *(msg++);
		tmp[i] = '\0';
		i = 0;
		msg = msg + 1;
		msg_len = nbiot_atoi(tmp,strlen(tmp));
	}

	uri.flag |= NBIOT_SET_OBJID;

	if(uri.instid != -1)
	uri.flag |= NBIOT_SET_INSTID;

	if(uri.resid != -1)
	uri.flag |= NBIOT_SET_RESID;

	if (COAP_READ == code)
	{
#ifdef DEBUG_LOG
		printf("read objid %d instid %d resid %d msgid %d\r\n",uri.objid,uri.instid,uri.resid,uri.msgid);
#endif
		memset(buffer,0,max_buffer_len);
		handle_read(dev,&uri,buffer,max_buffer_len);
	}

	if (COAP_WRITE == code)
	{
#ifdef DEBUG_LOG
		printf("write objid %d instid %d resid %d\r\n",uri.objid,uri.instid,uri.resid);
#endif
		handle_write(dev,
		             &uri,
		             msgid,
		             (unsigned char *)msg,
		             msg_len,
		             dev->write_func);
	}

	if (COAP_EXECUTE == code)
	{
#ifdef DEBUG_LOG
		printf("execute objid %d instid %d resid %d\r\n",uri.objid,uri.instid,uri.resid);
#endif
		while(*msg != ',')
		tmp[i ++] =* (msg ++);
		tmp[i] = '\0';
		i = 0;
		msg = msg + 2;
		length = nbiot_atoi(tmp,strlen(tmp));
		
		handle_execute(dev,
		               &uri,
		               msgid,
		               (unsigned char *)msg,
		               length,
		               dev->execute_func);
	}
}

static void handle_transaction(nbiot_device_t  *dev,
	                           uint16_t        code,
                               uint8_t         *buffer,
                               size_t          buffer_len,
                               size_t          max_buffer_len)
{
	char *msg = NULL,i = 0,tmp[64];
	uint16_t  mid;
	nbiot_uri_t  uri;
	nbiot_transaction_t *transaction = NULL;

	nbiot_node_t *obj;
	nbiot_node_t *res;
	nbiot_node_t *inst;
	char add = 1;
	uint16_t resid[20],rescont = 0;
	char buf[128];

	if(COAP_OBSERVE == code)
	{
		msg = strstr((char *)buffer,",");

		if(msg == NULL)
			return;

		msg = msg + 1;

		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		i = 0;
		msg = msg + 3;
		uri.msgid = nbiot_atoi(tmp,strlen(tmp));

		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		i = 0;
		msg = msg + 1;
		uri.objid = nbiot_atoi(tmp,strlen(tmp));

		if(uri.objid > 0)
		uri.flag |= NBIOT_SET_OBJID;
		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		i = 0;
		msg = msg + 1;
		uri.instid = nbiot_atoi(tmp,strlen(tmp));

		if(uri.instid >= 0)
		uri.flag |= NBIOT_SET_INSTID;
		while(*msg != '\r')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		i = 0;
		msg = msg + 1;
		uri.resid = nbiot_atoi(tmp,strlen(tmp));

		uri.flag |= NBIOT_SET_RESID;
		
		msgid_list[msgid_list_pos].objid= uri.objid;
		msgid_list[msgid_list_pos].instid = uri.instid;
		msgid_list[msgid_list_pos].msgid = uri.msgid;

		msgid_list_pos ++;

		handle_observe(dev,&uri);
	}
	else if(COAP_DISCOVER == code)
	{
		msg = strstr((char *)buffer,",");

		if(msg == NULL)
			return;

		msg = msg + 1;

		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		i = 0;
		msg = msg + 1;
		uri.msgid = nbiot_atoi(tmp,strlen(tmp));

		while(*msg != '\r')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		i = 0;
		msg = msg + 1;
		uri.objid = nbiot_atoi(tmp,strlen(tmp));

		memset(buf,0,sizeof(buf));
		obj = (nbiot_node_t*)dev->nodes;

		while(obj != NULL)
		{
			if(obj->id == uri.objid)
			{
				inst = (nbiot_node_t*)obj->data;

				while(inst != NULL)
				{
					res=(nbiot_node_t*)inst->data;

					while(res != NULL)
					{
						add = 1;

						for(i = 0; i < rescont; i ++)
						{
							if(resid[i] == res->id)
							{
								add = 0;

								break;
							}
						}

						if(add == 1)
							resid[rescont ++] = res->id;

						res = res->next;
					}

					inst = inst->next;
				}

				for(i = 0; i < rescont; i ++)
				{
					nbiot_itoa(resid[i],tmp,10);
					strcat(buf,tmp);

					if(i != rescont - 1)
						strcat(buf,";");
				}

				rescont = 0;
#ifdef DEBUG_LOG
				printf("discover: msgid %d objid %d\r\n",uri.msgid,uri.objid);
#endif
				handle_discover(&uri,strlen(buf),buf);

				break;
			}

			obj = obj->next;
		}
	}
	else if(COAP_EVENT == code)
	{
		msg = strstr((char *)buffer,":");

		if(msg == NULL)
			return;

		while(*msg != '\0')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		
		transaction = (nbiot_transaction_t*)NBIOT_LIST_GET( dev->transactions, dev->first_mid);

		if(strstr(tmp,",1\r\n"))
		{
			if(
			   dev->state == STATE_REGISTERED ||
			   dev->state == STATE_REG_UPDATE_PENDING ||
			   dev->state == STATE_REG_UPDATE_NEEDED)
			{
				dev->state = STATE_REG_FAILED;
			}
		}
		else if(strstr(tmp,",3\r\n") != NULL ||
		   strstr(tmp,",5\r\n") != NULL)
		{
			dev->state = STATE_REG_FAILED;
		}
		else if(strstr(tmp,",6\r\n") != NULL)
		{
			if(transaction != NULL)
				nbiot_transaction_del(dev,
				                      true,
				                      dev->first_mid);
		}
		else if(strstr(tmp,",8\r\n") != NULL)
		{
			if(dev->state == STATE_REG_PENDING)
			{
				if(transaction)
					nbiot_transaction_del(dev,
					                      true,
					                      dev->next_mid);
			}
			
			dev->state = STATE_REG_FAILED;
		}
		else if(strstr(tmp,",11\r\n") != NULL)
		{
			if(dev->state == STATE_REG_UPDATE_PENDING)
			{
				if(transaction)
					nbiot_transaction_del(dev,
					                      true,
					                      dev->next_mid);
			}
		}
		else if(strstr(tmp,",12\r\n")!=NULL)
		{
			if(dev->state == STATE_REG_UPDATE_PENDING)
			{
//				if(transaction)
//					nbiot_transaction_del(dev,
//					                      true,
//					                      dev->next_mid);
			}
			else
			{
				dev->state = STATE_REG_UPDATE_NEEDED;
			}
		}
		else if(strstr(tmp,",13\r\n")!=NULL)
		{
			if(dev->state == STATE_REG_UPDATE_PENDING)
			{
//				if(transaction)
//					nbiot_transaction_del(dev,
//					                      true,
//					                      dev->next_mid);
			}
			else
			{
				dev->state = STATE_REG_UPDATE_NEEDED;
			}
		}
		else if(strstr(tmp,",14,") != NULL)
		{
			if(dev->state == STATE_REG_UPDATE_PENDING)
			{
				if(transaction)
					nbiot_transaction_del(dev,
					                      true,
					                      dev->next_mid);
			}
			
			dev->state = STATE_REG_UPDATE_NEEDED;
		}
		else if(strstr(tmp,"0,15\r\n")!=NULL)
		{
			if(dev->state == STATE_DEREG_PENDING)
			{
				if(transaction)
					nbiot_transaction_del(dev,
					                      true,
					                      dev->next_mid);
			}
			else
			{
				dev->state = STATE_REG_FAILED;
			}
		}
		else if(strstr(tmp,",20\r\n") != NULL)
		{
			nbiot_transaction_del(dev,
			                      true,
			                      dev->next_mid);
		}
		else if(strstr(tmp,",25\r\n") != NULL)
		{
			dev->state = STATE_REG_FAILED;
		}
		else if(strstr(tmp,",26\r\n") != NULL)
		{
			msg = strstr((char *)buffer,"26,");
			if(msg != NULL)
			{
				msg = msg + 3;
				while(*msg != '\0')
				tmp[i ++] =* (msg++);
				tmp[i] = '\0';
				i = 0;
				mid = nbiot_atoi(tmp,strlen(tmp));
				transaction = (nbiot_transaction_t*)NBIOT_LIST_GET( dev->transactions, mid);

				if(transaction)
				{
					transaction->ack = 1;

					nbiot_transaction_del( dev,
											true,
											mid);
				}
			}
		}
		else if(strstr(tmp,"40\r\n") != NULL)
		{
//			while(1)
//			{
//				nbiot_sleep(1000);
//			}
		}
	}
}

static void nbiot_handle_buffer(nbiot_device_t *dev,
                                uint8_t        *buffer,
                                size_t          buffer_len,
                                size_t          max_buffer_len)
{
	uint16_t code = 0;
	char *read = NULL,*write = NULL,*excute = NULL;
	char *discover = NULL,*observe = NULL,*event = NULL;

	read = strstr((const char *)buffer, "+MIPLREAD");
	write = strstr((const char *)buffer, "+MIPLWRITE");
	excute = strstr((const char *)buffer, "+MIPLEXECUTE");
	discover = strstr((const char *)buffer, "+MIPLDISCOVER");
	observe = strstr((const char *)buffer, "+MIPLOBSERVE");
	event = strstr((const char *)buffer, "+MIPLEVENT");
	
	if(read != NULL)
		code = COAP_READ;
	else if(write != NULL)
		code = COAP_WRITE;
	else if(excute != NULL)
		code = COAP_EXECUTE;
	else if(discover != NULL)
		code = COAP_DISCOVER;
	else if(observe != NULL)
		code = COAP_OBSERVE;
	else if(event != NULL)
		code = COAP_EVENT;

	if(COAP_READ <= code && code <= COAP_EXECUTE)//下行命令
	{
		if(dev->state == STATE_REGISTERED ||
		   dev->state == STATE_REG_UPDATE_PENDING ||
		   dev->state ==STATE_REG_UPDATE_NEEDED ||
		   dev->state ==STATE_DEREG_PENDING)
			handle_request(dev,
			               code,
			               buffer,
			               buffer_len,
			               max_buffer_len);
	}
	else //observer或者注册状态回复
	{
		handle_transaction(dev,
		                   code,
		                   buffer,
		                   buffer_len,
		                   max_buffer_len);
	}
}

int nbiot_device_connect(nbiot_device_t *dev,
                         int            timeout)
{
	int ret;
	time_t last;
	time_t curr;
	uint8_t buffer[NBIOT_SOCK_BUF_SIZE] = {0};

	ret = nbiot_register_start(dev,buffer,sizeof(buffer));

	if (ret)
	{
		return NBIOT_ERR_REG_FAILED;
	}

	last = nbiot_time();

	do
	{
		int buffer_len = nbiot_recv_buffer(buffer,sizeof(buffer));
		if (buffer_len > 0)
		{
			nbiot_handle_buffer(dev,
			                    buffer,
			                    buffer_len,
			                    sizeof(buffer));
		}

		curr = nbiot_time();

		nbiot_transaction_step(dev,
		                       curr,
		                       buffer,
		                       sizeof(buffer));
		/* ok */
		if(Registered_Flag == 1)
		{
			dev->state = STATE_REGISTERED;

			return NBIOT_ERR_OK;
		}

		/* failed */
		if (dev->state == STATE_REG_FAILED )
		{
			return NBIOT_ERR_REG_FAILED;
		}

		/* continue */
		nbiot_sleep(100);
	}
	while(curr <= last + timeout);

	return NBIOT_ERR_TIMEOUT;
}

void nbiot_device_close(nbiot_device_t *dev,
                        int             timeout)
{
	int ret;
	uint8_t buffer[NBIOT_SOCK_BUF_SIZE] = {0};

	ret = nbiot_deregister(dev,buffer,sizeof(buffer));

	if (ret == COAP_NO_ERROR)
	{
		time_t last;
		time_t curr;

		last = nbiot_time();
		do
		{
			int buffer_len = nbiot_recv_buffer(buffer,sizeof(buffer));

			if(buffer_len > 0)
			{
				nbiot_handle_buffer(dev,
				                    buffer,
				                    buffer_len,
				                    sizeof(buffer));
			}

			curr = nbiot_time();

			nbiot_transaction_step(dev,
			                       curr,
			                       buffer,
			                       sizeof(buffer));

			if(dev->state == STATE_DEREGISTERED)
			{
				break;
			}

			/* continue */
			nbiot_sleep( 100 );
		}
		while(curr <= last + timeout);
	}
}

int nbiot_device_step( nbiot_device_t *dev,
                       int             timeout )
{
	time_t last;
	time_t curr;
	uint8_t buffer[NBIOT_SOCK_BUF_SIZE] = {0};

	last = nbiot_time();

	do
	{
		int buffer_len = nbiot_recv_buffer(buffer,sizeof(buffer));

		if(buffer_len > 0)
		{
			nbiot_handle_buffer(dev,
			                    buffer,
			                    buffer_len,
			                    sizeof(buffer));
		}

		memset(buffer,0,sizeof(buffer));

		curr = nbiot_time();

		nbiot_register_step(dev,
		                    curr,
		                    buffer,
		                    sizeof(buffer));

		if (dev->state == STATE_REGISTERED ||
		    dev->state == STATE_REG_UPDATE_NEEDED ||
		    dev->state == STATE_REG_UPDATE_PENDING||
		    dev->state == STATE_DEREG_PENDING||
		    dev->state == STATE_REG_PENDING||
		    dev->state == STATE_REG_FAILED)
		{
			nbiot_transaction_step(dev,
			                       curr,
			                       buffer,
			                       sizeof(buffer));
		}
		if(dev->state == STATE_REGISTERED ||
		   dev->state == STATE_REG_UPDATE_PENDING ||
		   dev->state == STATE_REG_UPDATE_NEEDED)
		{
			nbiot_observe_step(dev,
			                   curr,
			                   buffer,
			                   sizeof(buffer));
		}

		nbiot_sleep(10);
	}
	while(curr <= last + timeout);

	return STATE_ERROR(dev);
}

int nbiot_resource_add(nbiot_device_t  *dev,
                       uint16_t        objid,
                       uint16_t        instid,
                       uint16_t        resid,
                       uint16_t        isrw,
                       uint16_t		   isexec,
                       nbiot_value_t   *data,
                       uint8_t         index,
                       uint8_t         flag)
{
	nbiot_node_t *obj;
	nbiot_node_t *res;
	nbiot_node_t *inst;
	uint8_t i;
//	static uint8_t Attrcount = 0;
//	static uint8_t Actcount = 0;
//	static char insBitmap[10];
	bool obj_new = false;
	bool inst_new = false;
	nbiot_uri_t uri;

	uri.objid = objid;
	uri.instid = instid;
	uri.resid = resid;
	uri.flag |= NBIOT_SET_OBJID;
	uri.flag |= NBIOT_SET_INSTID;
	uri.flag |= NBIOT_SET_RESID;

	obj = (nbiot_node_t*)NBIOT_LIST_GET(dev->nodes, objid);

	if (!obj)
	{
		obj = (nbiot_node_t*)nbiot_malloc(sizeof(nbiot_node_t));

		if (!obj)
		{
			return NBIOT_ERR_NO_MEMORY;
		}

		obj_new = true;

		nbiot_memzero(obj, sizeof(nbiot_node_t));

		obj->id = objid;

		dev->nodes = (nbiot_node_t*)NBIOT_LIST_ADD(dev->nodes, obj);
	}

	inst = (nbiot_node_t*)NBIOT_LIST_GET(obj->data, instid);

	if (!inst)
	{
		inst = (nbiot_node_t*)nbiot_malloc(sizeof(nbiot_node_t));

		if (!inst)
		{
			if (obj_new)
			{
				dev->nodes = (nbiot_node_t*)NBIOT_LIST_DEL(dev->nodes, objid, NULL);

				nbiot_free(obj);
			}

			return NBIOT_ERR_NO_MEMORY;
		}
		
		inst_new = true;
		nbiot_memzero(inst, sizeof(nbiot_node_t));
		inst->id = instid;
		obj->data = NBIOT_LIST_ADD(obj->data, inst);
		
		strcat(insBitmap,"1");
	}

	

	if(flag == 0)
	{
		Attrcount += isrw;
		Actcount += isexec;

		if(index == 1)
		{
			for(i = 0; i < instid; i ++)
				strcat(insBitmap,"0");
		}

//		strcat(insBitmap,"1");
	}
	else if(flag == 1)
	{
		Attrcount += isrw;
		Actcount += isexec;

//		strcat(insBitmap,"1");

		bcxx_addobj(objid,instid + 1,insBitmap,Attrcount,Actcount);

		memset(insBitmap,'\0',strlen(insBitmap));

		Attrcount=0;
		Actcount=0;
	}

	nbiot_observe_add(dev,(const nbiot_uri_t *)&uri);
	
	res = (nbiot_node_t*)NBIOT_LIST_GET(inst->data, resid);
	if (!res)
	{
		res = (nbiot_node_t*)nbiot_malloc( sizeof(nbiot_node_t));
		if (!res )
		{
			if (inst_new)
			{
				obj->data = NBIOT_LIST_DEL(obj->data, instid, NULL);

				nbiot_free(inst);
			}

			if ( obj_new )
			{
				dev->nodes = (nbiot_node_t*)NBIOT_LIST_DEL(dev->nodes, objid, NULL);

				nbiot_free(obj);
			}

			return NBIOT_ERR_NO_MEMORY;
		}

		nbiot_memzero(res, sizeof(nbiot_node_t));

		res->id = resid;
		inst->data = NBIOT_LIST_ADD(inst->data, res);
		
		
//		data->flag |= NBIOT_UPDATED;						//登陆后会马上发数据
		
//		if(data->type == NBIOT_STRING)						//初始化字符串类型的数据
//        {
//            nbiot_free(data->value.as_buf.val);
//            data->value.as_buf.val = nbiot_strdup("NULL", 4);
//            data->value.as_buf.len = 4;
//        }
	}

	/* not free */
	res->data = data;

//    if (!STATE_ERROR(dev))
//    {
//        dev->state = STATE_REG_UPDATE_NEEDED;
//    }

    return NBIOT_ERR_OK;
}

int nbiot_resource_del(nbiot_device_t  *dev,
                       uint16_t        objid,
                       uint16_t        instid,
                       uint16_t        resid)
{
    nbiot_node_t *obj;
    nbiot_node_t *res;
    nbiot_node_t *inst;

    obj = (nbiot_node_t*)NBIOT_LIST_GET(dev->nodes, objid);

    if (obj)
    {
        inst = (nbiot_node_t*)NBIOT_LIST_GET(obj->data, instid);

        if (inst)
        {
            inst->data = NBIOT_LIST_DEL(inst->data, resid, &res);

            if (res)
            {
                nbiot_free(res);

                if (inst->data)
                {
                    obj->data = NBIOT_LIST_DEL(obj->data, instid, NULL);

                    nbiot_free(inst);
                }

                if (obj->data)
                {
                    dev->nodes = (nbiot_node_t*)NBIOT_LIST_DEL( dev->nodes, objid, NULL );

                    nbiot_free(obj);
                }

                if (!STATE_ERROR(dev))
                {
                    dev->state = STATE_REG_UPDATE_NEEDED;
                }

				bcxx_delobj(objid);

                return NBIOT_ERR_OK;
            }
        }
    }

    return NBIOT_ERR_NOT_FOUND;
}



int nbiot_send_buffer(const nbiot_uri_t   *uri,
	                  uint8_t             *buffer,
                      size_t              buffer_len,
                      bool                updated )
{
	int err = NBIOT_ERR_OK;
	char tmp[8];
	char buf[1024];
	size_t  len = 0;
	uint8_t type = 0;
	uint8_t trigger = 0;
	uint8_t *msg = NULL;
	nbiot_uri_t uri1;

	uri1.msgid = uri->msgid;
	uri1.objid = uri->objid;

	msg = buffer;

	while(1)
	{
		if(msg != NULL)
		{
			while(*msg != ',')
			tmp[len ++] = *msg ++;
			tmp[len] = '\0';
			msg = msg + 1;
			len = 0;
			uri1.instid = nbiot_atoi(tmp,strlen(tmp));

			while(*msg != ',')
			tmp[len ++] = *msg ++;
			tmp[len] = '\0';
			msg = msg + 1;
			len = 0;
			uri1.resid = nbiot_atoi(tmp,strlen(tmp));

			while(*msg != ',')
			tmp[len ++] = *msg ++;
			tmp[len] = '\0';
			msg = msg + 1;
			len = 0;
			type = nbiot_atoi(tmp,strlen(tmp));

			while(*msg != ';')
			buf[len ++] = *msg ++;
			buf[len] = '\0';
			msg = msg + 1;
			len = 0;
			if(*msg == '\0')
			trigger = 1;

			if(updated == true)
				err = bcxx_notify_upload(uri1,type,buf);
			else
				err = bcxx_read_upload(uri1,type,buf);

			if(1 == trigger)
				break;
		}
		else
		{
			break;
		}
	}
	
	if(err == 2)
	{
		err = NBIOT_ERR_ERROR;
	}

	return err;
}

int nbiot_recv_buffer(uint8_t *buffer,
                      size_t   buffer_len)
{
    int ret;
    size_t recv = 0;

    ret = nbiot_udp_recv(buffer,
                         buffer_len,
                         &recv);
    if (ret)
    {
        return ret;
    }

    return recv;
}

