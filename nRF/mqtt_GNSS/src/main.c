/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <dk_buttons_and_leds.h>

#include <nrf_modem_gnss.h>

#include <string.h>

#define STACK_SIZE					1024
#define GNSS_THREAD_PRIORITY		5
#define	KEEP_ALIVE_THREAD_PRIORITY	7
#define SEND_MQTT_THREAD_PRIORITY	6


LOG_MODULE_REGISTER(MQTT_GNSS, LOG_LEVEL_INF);

K_SEM_DEFINE(GNSS_SEM, 0, 1);
K_SEM_DEFINE(SEND_MQTT_SEM, 0, 1);

K_THREAD_STACK_DEFINE(threadStackGNSS, STACK_SIZE);
K_THREAD_STACK_DEFINE(threadStackKEEPALIVE, STACK_SIZE);
K_THREAD_STACK_DEFINE(threadStackMQTT, STACK_SIZE);

K_MSGQ_DEFINE(gnssMessage, sizeof(bool), 2, 1);

struct k_thread gnssThreadData, keepAliveThreadData, sendMQTT_ThreadData;

static struct nrf_modem_gnss_pvt_data_frame	pvtData;

static int64_t gnssStartTime;
static bool firstFix = false;
static bool fixFind = false;

static int modem_configure(void)
{
	int err;

	LOG_INF("Initializing modme library");

	err = nrf_modem_lib_init();
	if(err)
	{
		LOG_ERR("Failed to initialize the modem library: error: %d", err);
		return err;
	}

	return 0;
}

static void printFixData(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	LOG_INF("Latitude:			%.06f", pvt_data->latitude);
	LOG_INF("Longitude:			%.06f", pvt_data->longitude);
	LOG_INF("Altitude:			%.01f m", (double)pvt_data->altitude);
	LOG_INF("Time (UTC): 		%02u:%02u:%02u.%03u", pvt_data->datetime.hour,
								pvt_data->datetime.minute,
								pvt_data->datetime.seconds,
								pvt_data->datetime.ms);
}

static void gnss_event_handler(int event)
{
	int err;
	static bool toggle = false;
	switch(event)
	{
		case NRF_MODEM_GNSS_EVT_PVT:
			err = nrf_modem_gnss_read(&pvtData, sizeof(pvtData), NRF_MODEM_GNSS_DATA_PVT);
			fixFind = false;
			if(toggle)
			{
				dk_set_led_on(DK_LED3);
			}
			else
			{
				dk_set_led_off(DK_LED3);
			}

			toggle = !toggle;
			
			if(err)
			{
				dk_set_led_off(DK_LED3);
				LOG_ERR("nrf_modem_gnss_read failed, err %d", err);
				return;
			}

			if(pvtData.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID)
			{
				dk_set_led_on(DK_LED1);
				dk_set_led_off(DK_LED3);
				fixFind = true;
				k_msgq_put(&gnssMessage, &fixFind, K_FOREVER);
			}

			break;

		case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
			LOG_INF("GNSS has woken up");
			break;
		
		case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
			LOG_INF("GNSS enters sleep after fix");
			break;

		default:
			break;
	}
}


void gnssThread(void *arg1, void *arg2, void *arg3)
{
	bool firstStart = true;
	bool wakeUpThread = true;
	bool gnnsFixGet = false;

	uint64_t startTime = 0;
	int err;

	while(1)
	{
		k_sem_take(&GNSS_SEM, K_FOREVER);

		//dk_set_led_on(DK_LED2);
		printk("Starting GNSS");
		if(nrf_modem_gnss_start() != 0)
		{
			
			LOG_ERR("Failed to start GNSS");
			return;
		}

		startTime = k_uptime_get();
		
		printk("GNSS search\r\n");
		

		while(!gnnsFixGet)
		{
			k_msgq_get(&gnssMessage, &gnnsFixGet, K_NO_WAIT);
			/*if(!firstStart)
			{
				if(k_uptime_get() - startTime >= 60000)
				{
					firstStart = false;
					LOG_ERR("GNSS not find in time");
					LOG_INF("Stop GNSS");
					err = nrf_modem_gnss_stop();
					if(err)
					{
						LOG_ERR("nrf_modem_gnss_stop_error, err %d", err);
					}
					break;
				}
			}*/
		}
		gnnsFixGet = false;
		firstStart = false;
		nrf_modem_gnss_stop();
		k_sem_give(&SEND_MQTT_SEM);
		k_yield();
		
	}
}

void keepAliveThread(void *arg1, void *arg2, void *arg3)
{
	uint32_t gnssTimeoutCounter = 0;
	while(1)
	{
		LOG_INF("Keep alive\r\n");
		gnssTimeoutCounter++;

		if(gnssTimeoutCounter == 3)
		{
			gnssTimeoutCounter = 0;
			k_sem_give(&GNSS_SEM);
		}
		
		k_sleep(K_SECONDS(60));
	}
}

void sendMQTT_Thread(void *arg1, void *arg2, void *arg3)
{
	while(1)
	{
		k_sem_take(&SEND_MQTT_SEM, K_FOREVER);
		LOG_INF("SEND data to server!\r\n");
		printFixData(&pvtData);
		k_yield();
	}
}


int main(void)
{
	int err;
	k_tid_t threadID0, threadID1, threadID2;

	if(dk_leds_init() != 0)
	{
		LOG_ERR("Failed to initialize the LEDS Library");
	}

	err = modem_configure();
	if(err)
	{
		LOG_ERR("Failed to configure the modem");
		return 0;
	}

	if(lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS) != 0)
	{
		LOG_ERR("Failed to activate GNSS functional mode");
	}

	if(nrf_modem_gnss_event_handler_set(gnss_event_handler) != 0)
	{
		LOG_ERR("Failed to set GNSS event handler");
		return 0;
	}


	printk("Init threads\r\n");

	k_sem_give(&GNSS_SEM);

	threadID0 = k_thread_create(&gnssThreadData, threadStackGNSS, K_THREAD_STACK_SIZEOF(threadStackGNSS), gnssThread, (void *)fixFind, NULL, NULL, GNSS_THREAD_PRIORITY, 0, K_NO_WAIT);
	threadID1 = k_thread_create(&keepAliveThreadData, threadStackKEEPALIVE, K_THREAD_STACK_SIZEOF(threadStackKEEPALIVE), keepAliveThread, NULL, NULL, NULL, KEEP_ALIVE_THREAD_PRIORITY, 0, K_NO_WAIT);
	threadID2 = k_thread_create(&sendMQTT_ThreadData, threadStackMQTT, K_THREAD_STACK_SIZEOF(threadStackMQTT), sendMQTT_Thread, NULL, NULL, NULL, SEND_MQTT_THREAD_PRIORITY, 0, K_NO_WAIT);
	
}
