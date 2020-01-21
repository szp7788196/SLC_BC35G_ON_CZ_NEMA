/**
 * Copyright (c) 2017 China Mobile IOT.
 * All rights reserved.
**/

#include "internal.h"
#include "stdio.h"
#include "bcxx.h"

#define REGISTER_START  1
#define REGISTER_UPDATE 2
#define DEREGISTER      3



static void registraction_reply(nbiot_device_t  *dev,
                                bool            suc,
                                bool            ack)
{
    if(dev->state == STATE_REG_PENDING)
    {
        dev->registraction = nbiot_time();

        if (suc == true)
        {
            dev->state = STATE_REGISTERED;
        }
        else
        {
            dev->state = STATE_REG_FAILED;
        }
    }
}

int nbiot_register_start(nbiot_device_t  *dev,
	                     uint8_t         *buffer,
                         size_t          buffer_len)
{
	size_t length = 0;
#ifdef NBIOT_BOOTSTRAP
	if(dev->state == STATE_BS_FINISHED)
#else
	if(dev->state == STATE_REG_FAILED || dev->state == STATE_DEREGISTERED)
#endif
	{
		length = bcxx_register_request(buffer,buffer_len,dev->life_time);

		if(length > 0)
		{
			dev->state = STATE_REG_PENDING;

			nbiot_transaction_add(dev,
			                      true,
			                      buffer,
			                      length,
			                      registraction_reply);
		}
		else
		{
			return COAP_INTERNAL_SERVER_ERROR_500;
		}
	}

	return COAP_NO_ERROR;
}

static void registraction_update_reply(nbiot_device_t  *dev,
                                       bool            suc,
                                       bool            ack )
{
    if (dev->state == STATE_REG_UPDATE_PENDING)
    {
        dev->registraction = nbiot_time();

        if(suc == true)
        {
            dev->state = STATE_REGISTERED;
        }
        else
        {
            dev->state = STATE_REG_FAILED;
        }
    }
}

int nbiot_register_update(nbiot_device_t  *dev,
	                      uint8_t         *buffer,
                          size_t          buffer_len)
{
	size_t length=0;

//	if(dev->state == STATE_REG_UPDATE_NEEDED)
	{
#ifdef DEBUG_LOG
		printf("update\r\n");
#endif
		length = bcxx_register_update(dev->life_time,buffer,buffer_len);

		if(length > 0)
		{
			dev->state = STATE_REG_UPDATE_PENDING;

			nbiot_transaction_add(dev,
			                      true,
			                      buffer,
			                      length,
			                      registraction_update_reply);

//			SendCmd("AT+MIPLCLOSE=0\r\n","OK",300,0,TIMEOUT_5S);//������
//			dev->state = STATE_REG_FAILED;
		}
		else
		{
			return COAP_INTERNAL_SERVER_ERROR_500;
		}
	}

	return COAP_NO_ERROR;
}

static void deregister_reply(nbiot_device_t     *dev,
                             bool               suc,
                             bool               ack)
{
    if(dev->state == STATE_DEREG_PENDING)
    {
        dev->state = STATE_DEREGISTERED;
    }
}

int nbiot_deregister(nbiot_device_t  *dev,
                     uint8_t         *buffer,
                     size_t          buffer_len)

{
	size_t length=0;

	if ( dev->state == STATE_REGISTERED ||
         dev->state == STATE_REG_UPDATE_NEEDED ||
         dev->state == STATE_REG_UPDATE_PENDING ||
         dev->state == STATE_REG_FAILED)
	{
		length = bcxx_close_request(buffer,buffer_len);

		if(length>0)
		{
			dev->state = STATE_DEREG_PENDING;

			nbiot_transaction_add(dev,
			                      true,
			                      buffer,
			                      length,
			                      deregister_reply);
		}
		else
		{
			return COAP_INTERNAL_SERVER_ERROR_500;
		}
	}

	return COAP_NO_ERROR;
}

int nbiot_register_step(nbiot_device_t *dev,
                         time_t         now,
                         uint8_t        *buffer,
                         size_t         buffer_len)
{
	int err = COAP_NO_ERROR;

	int next_update = dev->life_time;

	if (dev->state == STATE_REGISTERED)
	{
		next_update = next_update;

		if(dev->registraction + next_update <= now)
		{
			nbiot_register_update(dev,buffer,buffer_len);
		}
	}

	if(dev->state == STATE_REG_UPDATE_NEEDED)
	{
		nbiot_register_update(dev,buffer,buffer_len);
	}

	if(dev->state == STATE_REG_FAILED)
	{
		nbiot_device_close(dev,10);
		err = nbiot_register_start(dev,buffer,buffer_len);
	}

	return err;
}
