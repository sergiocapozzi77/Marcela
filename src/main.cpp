/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include "FS.h"
#include "SPIFFS.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "esp_http_client.h"
#include "esp_tls.h"

#define USE_SERIAL Serial

WiFiMulti wifiMulti;
const char* ssid = "99BB Hyperoptic 1Gbps Broadband";
const char* password = "hszdtubp";
#define FORMAT_SPIFFS_IF_FAILED true
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048


typedef struct {
    String fileName; 
} userData;

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void writeFile(fs::FS &fs, const char * path, const char * message, const int len){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.write((uint8_t *) message, len)){
        Serial.println("- file written");
    } else {
        Serial.println("- frite failed");
    }

    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message, const int len){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.write((uint8_t *) message, len)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }

    file.close();
}

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
            Serial.println("HTTP_EVENT_ON_DATA");
            userData *usrData = (userData *) evt->user_data;
            if(!receivingFile)
            {
                if(SPIFFS.exists(usrData->fileName))
                {
                    SPIFFS.remove(usrData->fileName);
                }
            }

            receivingFile = true;

            if (!esp_http_client_is_chunked_response(evt->client)) {
                Serial.print("HTTP_EVENT_ON_DATA CHUNK: ");
                Serial.println(evt->data_len);
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

/*
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
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
            
            //Serial.println("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        
            if (!esp_http_client_is_chunked_response(evt->client)) {
                Serial.print("HTTP_EVENT_ON_DATA CHUNK: ");
                Serial.println(evt->data_len);

                if (output_buffer == NULL) {
                    output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL) {
                        Serial.println("Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);

                output_len += evt->data_len;
            }
            else
            {
                Serial.print("HTTP_EVENT_ON_DATA NOCHUNK: ");
                Serial.println(evt->data_len);

                output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                output_len = 0;
                if (output_buffer == NULL) {
                    Serial.println("Failed to allocate memory for output buffer");
                    return ESP_FAIL;
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
                
                output_len = evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            Serial.println("HTTP_EVENT_ON_FINISH");
            Serial.print("Data: ");
          //  Serial.println(output_buffer);
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            Serial.println("HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
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
*/

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){        
        Serial.print(file.readString());
        Serial.print("line")
    }
}

void setup() {

    USE_SERIAL.begin(115200);

    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    listDir(SPIFFS, "/", 0);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();
    readFile(SPIFFS, "/index.txt");

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    wifiMulti.addAP(ssid, password);
}

void downloadFile(const char *link, String fileName)
{
      // wait for WiFi connection
   if((wifiMulti.run() == WL_CONNECTED)) {
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
        config->user_data = &user;

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
}

void loop() {
   downloadFile("https://raw.githubusercontent.com/sergiocapozzi77/Marcela/master/content/index", "/index.txt");
   readFile(SPIFFS, "/index.txt");
   delay(50000);
}