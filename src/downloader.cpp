#include "downloader.h"
#include <Arduino.h>

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "esp_http_client.h"
#include "esp_tls.h"
#include <Update.h>
#include "fsHelper.h"
#include "SPIFFS.h"

#define MAX_HTTP_RECV_BUFFER 2048

bool isOta = false;

typedef struct {
    String fileName; 
    fs::FS *activeFS;
} userData;

static bool receivingFile = false;
static bool writeOk = true;
static unsigned int totalSize = 0;
static unsigned int contentLength = 0;

bool success;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
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
            if(strcmp(evt->header_key, "Content-Length") == 0)
            {
                contentLength = atoi(evt->header_value);
                Serial.print("Content lenth: ");
                Serial.println(contentLength);
            }

            /*Serial.print("HTTP_EVENT_ON_HEADER: ");
            Serial.print(evt->header_key);
            Serial.print(" - ");
            Serial.print(evt->header_value);
            Serial.println();*/
            break;
        case HTTP_EVENT_ON_DATA:
        {
            if(isOta)
            {
                if(!receivingFile)
                {
                    Serial.println("Update OTA [");
                    if (!Update.begin(contentLength)) { //start with max available size
                        Serial.print("Update begin error: ");
                        Update.printError(Serial);
                    }
                    receivingFile = true;
                }

                if (Update.write((uint8_t *)evt->data, evt->data_len) != evt->data_len) {
                    Update.printError(Serial);
                }
            }
            else
            {
                if(!writeOk)
                {
                    return ESP_FAIL;
                }

                userData *usrData = (userData *) evt->user_data;
                if(!receivingFile)
                {
                    if(!deleteIfExists(*usrData->activeFS, usrData->fileName.c_str()))
                    {
                        writeOk = false;
                        return ESP_FAIL;
                    }

                    Serial.print("Downloading: ");
                    Serial.println(usrData->fileName);
                }

                receivingFile = true;

                if (!esp_http_client_is_chunked_response(evt->client)) {
                // Serial.print("HTTP_EVENT_ON_DATA CHUNK: ");
                // Serial.println(evt->data_len);
                    writeOk = appendFile(*usrData->activeFS, usrData->fileName.c_str(), (char *) evt->data, evt->data_len);
                }
                else
                {
                    Serial.print("HTTP_EVENT_ON_DATA NOCHUNK: ");
                    Serial.println(evt->data_len);
                    writeOk = writeFile(*usrData->activeFS, usrData->fileName.c_str(), (char *) evt->data, evt->data_len);
                }          
            }

            Serial.print(".");

            totalSize += evt->data_len;

            break;
        }
        case HTTP_EVENT_ON_FINISH:
            Serial.println("HTTP_EVENT_ON_FINISH");
            
            if(isOta)
            {
                if (Update.end(true)) { //true to set the size to the current progress
                    Serial.printf("] Update Success: %u\nRebooting...\n", totalSize);
                    success = true;
                } else {
                    Update.printError(Serial);
                    success = false;
                }
            }
            else
            {
                if(writeOk)
                {
                    success = true;
                }
            }

            Serial.printf("Data received: %u", totalSize);
            Serial.println();
            receivingFile = false;
            totalSize = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            // this is always called, also on success
            Serial.println("HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            receivingFile = false;
            totalSize = 0;
            if(!success)
            {
                if(isOta)
                {
                    Update.abort();
                }
            }
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

bool downloadFile(const char *link, String fileName, fs::FS &fs, bool isOtaUpdate)
{
    success = false;
    writeOk = true;
    Serial.print("downloadFile: ");
    Serial.println(fileName);
    Serial.print("isOtaUpdate: ");
    Serial.println(isOtaUpdate);
    
    isOta = isOtaUpdate;
    // wait for WiFi connection
    receivingFile = false;
    totalSize = 0;

    userData *user = new userData();
    user->fileName = fileName;
    user->activeFS = &fs;

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
        Serial.print("HTTP chunk encoding OK: " );
        Serial.println(esp_http_client_get_status_code(client));
        Serial.println(esp_http_client_get_content_length(client));           
    } else {
        Serial.print("Error perform http request: " );
        Serial.println(esp_err_to_name(err));
    }

    Serial.println("http client cleanup");
    esp_http_client_cleanup(client);
    delete user;
    delete config;

    return success;
}