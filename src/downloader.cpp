#include "downloader.h"
#include <Arduino.h>

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "esp_http_client.h"
#include "esp_tls.h"

#include "fsHelper.h"
#include "SPIFFS.h"

#define MAX_HTTP_RECV_BUFFER 2048

typedef struct {
    String fileName; 
} userData;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static bool receivingFile = false;
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            Serial.println("HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            Serial.println("HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            Serial.println("HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            //Serial.println("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            Serial.println("HTTP_EVENT_ON_HEADER");
            break;
        case HTTP_EVENT_ON_DATA:
        {
            userData *usrData = (userData *) evt->user_data;
            if(!receivingFile)
            {
                deleteIfExists(SPIFFS, usrData->fileName.c_str());
            }

            receivingFile = true;

            if (!esp_http_client_is_chunked_response(evt->client)) {
               // Serial.print("HTTP_EVENT_ON_DATA CHUNK: ");
               // Serial.println(evt->data_len);
                appendFile(SPIFFS, usrData->fileName.c_str(), (char *) evt->data, evt->data_len);
            }
            else
            {
                Serial.print("HTTP_EVENT_ON_DATA NOCHUNK: ");
                Serial.println(evt->data_len);
                writeFile(SPIFFS, usrData->fileName.c_str(), (char *) evt->data, evt->data_len);
            }          

            break;
        }
        case HTTP_EVENT_ON_FINISH:
            Serial.println("HTTP_EVENT_ON_FINISH");
            Serial.print("Data: ");
            receivingFile = false;
            break;
        case HTTP_EVENT_DISCONNECTED:
            Serial.println("HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            receivingFile = false;
         //   esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
           // if (err != 0) {
             //   if (output_buffer != NULL) {
               //     free(output_buffer);
                 //   output_buffer = NULL;
                //}
                //output_len = 0;
                //ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                //ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            //}
            break;
    }
    return ESP_OK;
}

void downloadFile(const char *link, String fileName)
{
    // wait for WiFi connection

    userData *user = new userData();
    user->fileName = fileName;

    esp_http_client_config_t *config = new esp_http_client_config_t();
    //config->url = "http://httpbin.org/stream/1";
    //   config->url = "http://www.sci.utah.edu/~macleod/docs/txt2html/sample.txt";
    config->url = link;
    config->event_handler = _http_event_handler;
    config->method        = HTTP_METHOD_GET;
    config->path          = "/";
    config->username      = "";
    config->password      = "";
    config->query         = "";
    config->max_redirection_count = 2;
    config->disable_auto_redirect = false;
    config->user_data = user;
    config->is_async = false;
    config->buffer_size = 1024;


    esp_http_client_handle_t client = esp_http_client_init(config);
    esp_err_t err = esp_http_client_perform(client);
    Serial.println("Created esp_http_client_perform");

    if (err == ESP_OK) {
        Serial.print("HTTP chunk encoding OK" );
        Serial.println(esp_http_client_get_status_code(client));
        Serial.println(esp_http_client_get_content_length(client));           
    } else {
        Serial.print("Error perform http request" );
        Serial.println(esp_err_to_name(err));
    }

    Serial.println("http client cleanup");
    esp_http_client_cleanup(client);
    delete user;
    delete config;
    listDir(SPIFFS, "/", 0);     

}