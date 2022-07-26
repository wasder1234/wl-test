/* wl_test project
	see README.md
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

//commit 3, pt1


//#include "ledc.h"
//#include "homie.h"
//#include "driver/gpio.h"

// GitHubTest. Branch2
// GitHubTest1. Branch2

//--------------- user defined --------------
#define WL_ESP_WIFI_SSID      	"WL_WIFI_SSID"			//your wifi access point
#define WL_ESP_WIFI_PASS      	"WL_WIFI_PASSWORD"		//your wifi password

#define WL_MQTT_URI			  	"m13.cloudmqtt.com"
#define WL_MQTT_USERNAME	  	"WL_MQTT_USER"			//User @ MQTT server
#define WL_MQTT_PASSWORD	  	"WL_MQTT_PASSWORD"		//Password @ MQTT server

#define WL_MQTT_BASE_TOPIC		"homie"					//device id homie/wl-test-01
#define WL_MQTT_CLIENT_ID		"wl-test-01"

#define WL_INPUT_PIN_SUBTOPIC	"input-pin/on-state"	//topic get: homie/wl-test-01/input-pin/on-state -> true|false
#define WL_LED_SUBTOPIC			"led/on-state"			//topic set: homie/wl-test-01/led/on-state <- true|false


//adjust blinking led parameters during WjFi and MQTT connection establishing
#define LEDC_OUTPUT_IO          	(2)				//gpio pin on ESP32 dev board. change if to desired led gpio if needed
#define LEDC_FREQUENCY          	(2) 			// Frequency in Hertz. Set frequency at 2 Hz

#define WL_LED_OUTPUT_IO    		5				//led pin number
#define WL_LED_OUTPUT_PIN_SEL  	(1ULL<<WL_LED_OUTPUT_IO)

#define WL_GPIO_INPUT_IO    		6				//gpio input pin number
#define WL_GPIO_INPUT_PIN_SEL  	(1ULL<<WL_GPIO_INPUT_IO)

#define RECONNECT_WAY	1		//conditional compilation
//-----------------------------

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095


//#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

//static int s_retry_num = 0;

//----------------------------------------------
static void wl_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        //start led blinking by changing duty
        ledc_set_duty_and_update(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY, ledc_get_hpoint(LEDC_MODE, LEDC_CHANNEL));
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        //if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
        	//start led blinking by changing duty
        	ledc_set_duty_and_update(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY, ledc_get_hpoint(LEDC_MODE, LEDC_CHANNEL));
            esp_wifi_connect();
            //s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        //} else {
            //xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        //}
        //ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        //s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        //stop led blinking by changing duty to 0
        ledc_set_duty_and_update(LEDC_MODE, LEDC_CHANNEL, 0, ledc_get_hpoint(LEDC_MODE, LEDC_CHANNEL));
    }
}

//commit 3, pt4

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //esp_event_handler_instance_t instance_any_id;
    //esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));	//&instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));	//&instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WL_ESP_WIFI_SSID,
            .password = WL_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdTRUE, //pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WL_ESP_WIFI_SSID, WL_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WL_ESP_WIFI_SSID, WL_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    //vEventGroupDelete(s_wifi_event_group);
}

//commit 3, pt3

void wl_homie_connect_handler()
{
	// turn blinking off
	ledc_set_duty_and_update(LEDC_MODE, LEDC_CHANNEL, 0, ledc_get_hpoint(LEDC_MODE, LEDC_CHANNEL));
}


void wl_homie_disconnect_handler()
{
	// turn blinking on
	ledc_set_duty_and_update(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY, ledc_get_hpoint(LEDC_MODE, LEDC_CHANNEL));
	//FIXME: not sure what should work
#if RECONNECT_WAY == 1
	esp_mqtt_client_reconnect(client);	//1. just reconnect
#elif RECONNECT_WAY == 2
	esp_mqtt_client_start(client);		//
//#elif RECONNECT_WAY == 3
//	esp_mqtt_client_stop(client);		// can't be called from mqtt handler
//	esp_mqtt_client_start(client);
#endif
}


void wl_homie_msg_handler(char *subtopic, char *payload)
{
	//parse topic
    if(strncmp(subtopic, WL_LED_SUBTOPIC, strlen(WL_LED_SUBTOPIC)) == 0){
        if(strncmp(payload, "true", strlen("true")) == 0){
        	gpio_set_level(WL_LED_OUTPUT_IO, 1);	//turn led on
		}
        else {
        	gpio_set_level(WL_LED_OUTPUT_IO, 0);	//turn led off
        }
    }
}


//commit 3, pt2


void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Set the LEDC peripheral configuration
    wl_ledc_init();

    //init led control pin
    gpio_config_t led_conf = {
    	.pin_bit_mask = WL_LED_OUTPUT_PIN_SEL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_conf);

    //init input control pin
    gpio_config_t input_conf = {
    	.pin_bit_mask = WL_GPIO_INPUT_PIN_SEL,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,			//pull-up enable. connect button normal open, close to gnd.
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&input_conf);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    //turn blinking on while MQTT connects
    ledc_set_duty_and_update(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY, ledc_get_hpoint(LEDC_MODE, LEDC_CHANNEL));

    //init Homie
    homie_config_t homie_conf = {
        .mqtt_uri = WL_MQTT_URI,
        .mqtt_username = WL_MQTT_USERNAME,
        .mqtt_password = WL_MQTT_PASSWORD,
		.client_id = WL_MQTT_CLIENT_ID,
        .device_name = "WebbyLab Device",
        .base_topic = WL_MQTT_BASE_TOPIC,
        .firmware_name = "Example",
        .firmware_version = "0.0.1",
        .ota_enabled = false,
		.msg_handler = wl_homie_msg_handler,
		.connected_handler = wl_homie_connect_handler,
		.disconnected_handler = wl_homie_disconnect_handler
    };

    homie_init(&homie_conf);
    homie_subscribe(WL_LED_SUBTOPIC);

    // Keep the main task around
    while (1) {

    	homie_publish_bool(WL_INPUT_PIN_SUBTOPIC, 1, 1, gpio_get_level(WL_GPIO_INPUT_IO)^1 );

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

}
