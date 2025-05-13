/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/net/socket.h>

#include <nrf_modem_gnss.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>


#include "mqtt_connection.h"

#define MESSAGE_SIZE				256

#define STACK_SIZE					1024
#define GNSS_THREAD_PRIORITY		5
#define	KEEP_ALIVE_THREAD_PRIORITY	7
#define SEND_MQTT_THREAD_PRIORITY	6


LOG_MODULE_REGISTER(MQTT_GNSS, LOG_LEVEL_INF);

K_SEM_DEFINE(GNSS_SEM, 0, 1);
K_SEM_DEFINE(SEND_MQTT_SEM, 0, 1);

K_SEM_DEFINE(lte_connected, 0, 1);

K_THREAD_STACK_DEFINE(threadStackGNSS, STACK_SIZE);
K_THREAD_STACK_DEFINE(threadStackMQTT, STACK_SIZE);

K_MSGQ_DEFINE(gnssMessage, sizeof(bool), 2, 1);
K_MSGQ_DEFINE(LTE_Message, sizeof(bool), 2, 1);
K_MSGQ_DEFINE(LTE_error, sizeof(bool), 2, 1);
K_MSGQ_DEFINE(getGnss, sizeof(bool), 2, 1);

struct k_thread gnssThreadData, keepAliveThreadData, sendMQTT_ThreadData;

static struct nrf_modem_gnss_pvt_data_frame	pvtData;

static uint8_t GPS_data[MESSAGE_SIZE];

static struct mqtt_client client;

static struct pollfd fds;

static bool fixFind = false;
static bool lteConnected = false;
static bool lteError = false;
volatile bool connected = false;

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch(evt->type)
	{
		case LTE_LC_EVT_NW_REG_STATUS:
			if((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
			   (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING))
			{
				break;
			}

			if(evt->nw_reg_status == LTE_LC_NW_REG_REGISTRATION_DENIED)
			{
				LOG_ERR("LTE error");
				lteError = true;
				k_msgq_put(&LTE_error, &lteError, K_FOREVER);
				lteError = false;
				k_sem_give(&lte_connected);
				break;
			}

			LOG_INF("Networking registration status: %s", 
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "Connected - home network" : "Connected - Roaming");
			lteConnected = true;
			k_msgq_put(&LTE_Message, &lteConnected, K_FOREVER);
			lteConnected = false;
			k_sem_give(&lte_connected);
			break;

		case LTE_LC_EVT_RRC_UPDATE:
			LOG_INF("RRC mode: %s", evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ? "Connected" : "Idle");
			break;

		case LTE_LC_EVT_CELL_UPDATE:
			LOG_INF("LTE cell changed: Cell ID: %d, Tracking area: %d\n", evt->cell.id, evt->cell.tac);
			break;
		
		default:
			break;
	}
}

static int modem_configure(void)
{
	int err;

	printk("Initializing modme library");

	err = nrf_modem_lib_init();
	if(err)
	{
		LOG_ERR("Failed to initialize the modem library: error: %d", err);
		return err;
	}

	err = certificate_provision();
	if(err != 0)
	{
		LOG_ERR("Failed to provision certificates");
		return err;
	}

	LOG_INF("Connecting to LTE network");
	err = lte_lc_connect_async(lte_handler);
	if(err)
	{
		LOG_ERR("Error in lte_lc_connect_async, error: %d", err);
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

	int err = snprintf(GPS_data, MESSAGE_SIZE, "Time (UTC): %02u:%02u:%02u, Latitude: %.06f, Longitude: %.06f", pvt_data->datetime.hour,  pvt_data->datetime.minute, 
																										        pvt_data->datetime.seconds, pvt_data->latitude, pvt_data->longitude);
	if(err < 0)
	{
		LOG_ERR("Failed to print to buffer: %d", err);
	}
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
			dk_set_led_off(DK_LED1);
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
	bool gnnsFixGet = false;
	bool getNewGnss = false;
	

	uint64_t startTime = 0;
	int err;

	while(1)
	{
		k_sem_take(&GNSS_SEM, K_FOREVER);

		getNewGnss = false;

		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS);
		if(err)
		{
			LOG_ERR("Failed to deactivate LTE and enable GNSS functional mode");
		}

		//dk_set_led_on(DK_LED2);
		printk("Starting GNSS");
		if(nrf_modem_gnss_start() != 0)
		{
			LOG_ERR("Failed to start GNSS");
		}

		startTime = k_uptime_get();
		
		printk("GNSS search\r\n");
		

		while(!gnnsFixGet)
		{
			k_msgq_get(&gnssMessage, &gnnsFixGet, K_NO_WAIT);
			//if(!firstStart)
			//{
				if(k_uptime_get() - startTime >= CONFIG_GNSS_TIMEOUT)
				{
					firstStart = false;
					LOG_ERR("GNSS not find in time");
					LOG_INF("Stop GNSS");
					err = nrf_modem_gnss_stop();
					dk_set_led_off(DK_LED3);
					if(err)
					{
						LOG_ERR("nrf_modem_gnss_stop_error, err %d", err);
					}
					snprintf(GPS_data, MESSAGE_SIZE, "Time out GNSS");
					break;
				}
			//}
			
		}

		LOG_INF("Find time: %lld s\r\n", (k_uptime_get() - startTime)/1000);

		gnnsFixGet = false;
		firstStart = false;

		getNewGnss = true;

		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
		if(err)
		{
			LOG_ERR("Failed to deactivate LTE and enable GNSS functional mode");
		}

		k_msgq_put(&getGnss, &getNewGnss, K_FOREVER);

		

		k_yield();
		
	}
}

void sendMQTT_Thread(void *arg1, void *arg2, void *arg3)
{
	int err;
	bool lteConnectedGet = false;
	uint8_t counter = 0;
	bool getNewGnss = false;
	bool getLteError = false;
	
	while(1)
	{
		k_sleep(K_SECONDS(60));
		k_msgq_get(&getGnss, &getNewGnss, K_NO_WAIT);
		LOG_INF("get gnss: %d", getNewGnss);
		LOG_INF("Counter: %d", counter);
		
		if(getNewGnss)
		{
			getNewGnss = false;
			err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
			if (err != 0){
				LOG_ERR("Failed to activate LTE");
				//break;
			}

			dk_set_led_on(DK_LED1); //piros led
			k_sem_take(&lte_connected, K_FOREVER);
			dk_set_led_off(DK_LED1);

			dk_set_led_on(DK_LED2); //kék led
			while(!lteConnectedGet)
			{
				k_msgq_get(&LTE_Message, &lteConnectedGet, K_NO_WAIT);
			}
			dk_set_led_off(DK_LED2);
			lteConnectedGet = false;

			printFixData(&pvtData);

			k_msgq_get(&LTE_error, &getLteError, K_NO_WAIT);

			if(!getLteError)
			{

				err = mqtt_live(&client);

				if(err)
				{
					LOG_ERR("MQTT disconnected, err %d", err);

					err = client_init(&client);
					if (err) {
						LOG_ERR("Failed to initialize MQTT client: %d", err);
					}

					err = mqtt_connect(&client);
					if(err)
					{
						LOG_ERR("Error in mqtt_connect: %d", err);
						//return;
					}

					while (!connected) {
						LOG_INF("Wait for mqtt connect");
						mqtt_input(&client);  // feldolgozza az érkező MQTT csomagokat
						mqtt_live(&client);   // életjel küldés, ha kell
						k_sleep(K_MSEC(100)); // kis szünet, hogy ne pörögjön a ciklus
					}
					connected = false;
				}
			
				err = fds_init(&client, &fds);
				if(err)
				{
					LOG_ERR("Error in fds_init: %d", err);
					//return;
				}

				

				LOG_INF("SEND data to server!\r\n");
				
				data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, GPS_data, sizeof(GPS_data));
				if(err)
				{
					LOG_ERR("Error in data_publish, err %d", err);
				}

				/*err = mqtt_disconnect(&client);
				if(err)
				{
					LOG_ERR("error in mqtt_disconnect, err %d", err);
				}*/
			}

			err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
			if(err)
			{
				LOG_ERR("Failed to deactivate LTE and enable GNSS functional mode");
			}

		}
		else
		{
			counter++;

			if(counter == CONFIG_GNSS_INTERVAL)
			{
				counter = 0;
				k_sem_give(&GNSS_SEM);
				k_yield();
			}
		}
	}
}


int main(void)
{
	int err;
	uint32_t connect_attempt = 0;

	k_tid_t threadID0, threadID2;

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

	k_sem_take(&lte_connected, K_FOREVER);

	err = client_init(&client);
	if (err) {
		LOG_ERR("Failed to initialize MQTT client: %d", err);
		return 0;
	}

	if (connect_attempt++ > 0) {
		LOG_INF("Reconnecting in %d seconds...",
			CONFIG_MQTT_RECONNECT_DELAY_S);
		k_sleep(K_SECONDS(CONFIG_MQTT_RECONNECT_DELAY_S));
	}

	err = mqtt_connect(&client);
	if(err)
	{
		LOG_ERR("Error in mqtt_connect: %d", err);
	}

	while (!connected) {
		mqtt_input(&client);  // feldolgozza az érkező MQTT csomagokat
		mqtt_live(&client);   // életjel küldés, ha kell
		k_sleep(K_MSEC(100)); // kis szünet, hogy ne pörögjön a ciklus
	}
	connected = false;

	

	err = fds_init(&client,&fds);
	if (err) {
		LOG_ERR("Error in fds_init: %d", err);
		return 0;
	}

	err = data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, (uint8_t *)"Hello", sizeof("Hello"));
	if(err)
	{
		LOG_ERR("Error in data_publish, err %d", err);
	}

	/*err = mqtt_disconnect(&client);
	if(err)
	{
		LOG_ERR("error in mqtt_disconnect, err %d", err);
		return err;
	}*/


	
	if(lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS) != 0)
	{
		LOG_ERR("Failed to activate GNSS functional mode");
	}

	if(nrf_modem_gnss_event_handler_set(gnss_event_handler) != 0)
	{
		LOG_ERR("Failed to set GNSS event handler");
		return 0;
	}

	

	if(lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE) != 0)
	{
		LOG_ERR("Failed to activate GNSS functional mode");
	}



	printk("Init threads\r\n");

	k_sem_give(&GNSS_SEM);

	threadID0 = k_thread_create(&gnssThreadData, threadStackGNSS, K_THREAD_STACK_SIZEOF(threadStackGNSS), gnssThread, (void *)fixFind, NULL, NULL, GNSS_THREAD_PRIORITY, 0, K_NO_WAIT);
	threadID2 = k_thread_create(&sendMQTT_ThreadData, threadStackMQTT, K_THREAD_STACK_SIZEOF(threadStackMQTT), sendMQTT_Thread, NULL, NULL, NULL, SEND_MQTT_THREAD_PRIORITY, 0, K_NO_WAIT);
	
}
