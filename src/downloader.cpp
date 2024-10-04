#include "downloader.h"
#include <Arduino.h>
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "esp_http_client.h"
#include "esp_tls.h"
#include <Update.h>
#include "fsHelper.h"
#include "SPIFFS.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#define MAX_HTTP_RECV_BUFFER 2048
bool isOta = false;

typedef struct
{
    String fileName;
    fs::FS *activeFS;
} userData;

static bool receivingFile = false;
static bool writeOk = true;
static unsigned int totalSize = 0;
static unsigned int contentLength = 0;
File fileToDownload;

bool success;

// This is GandiStandardSSLCA2.pem, the root Certificate Authority that signed
// the server certifcate for the demo server https://jigsaw.w3.org in this
// example. This certificate is valid until Sep 11 23:59:59 2024 GMT
const char *rootCACertificate =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIHOTCCBiGgAwIBAgIQBj1JF0BNOeUTyz/uzRsuGzANBgkqhkiG9w0BAQsFADBZ\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMTMwMQYDVQQDEypE\n"
    "aWdpQ2VydCBHbG9iYWwgRzIgVExTIFJTQSBTSEEyNTYgMjAyMCBDQTEwHhcNMjQw\n"
    "MzE1MDAwMDAwWhcNMjUwMzE0MjM1OTU5WjBnMQswCQYDVQQGEwJVUzETMBEGA1UE\n"
    "CBMKQ2FsaWZvcm5pYTEWMBQGA1UEBxMNU2FuIEZyYW5jaXNjbzEVMBMGA1UEChMM\n"
    "R2l0SHViLCBJbmMuMRQwEgYDVQQDDAsqLmdpdGh1Yi5pbzCCASIwDQYJKoZIhvcN\n"
    "AQEBBQADggEPADCCAQoCggEBAK0rFKU6TEGvuLCY3ZOuXlG+3jerD6EP1gc1qe35\n"
    "g68FqyGuVPOUddYNZiymjYMZxywoNp3qxlbFFBTf9etsayavT+uW+2UMjqCotAdK\n"
    "KicBEspuExoACFuNgTi7sSUT7A55+k4/+5O+VtpaxQ5dmQk7HxcqvMYx5owBU+fB\n"
    "wYDD+hXeg3YvxLZNeIlN8OlqWL8w9HbG+3ccegVEjOJQbkrcrW7IQMq2Uk92XjxI\n"
    "PmMVIvaefqcC1poGYvS4VvEh3x64vJK1hEM4YLMKBaE/hqFtcMozi+H/8JqTCfzP\n"
    "Qhnu21HIop9rSucxxnZbe9AeHz2LERpUTf3rjgOMg9PB1RUCAwEAAaOCA+0wggPp\n"
    "MB8GA1UdIwQYMBaAFHSFgMBmx9833s+9KTeqAx2+7c0XMB0GA1UdDgQWBBTob1fr\n"
    "hlGY65+lvlPa25SsKC777TB7BgNVHREEdDByggsqLmdpdGh1Yi5pb4IJZ2l0aHVi\n"
    "LmlvghVnaXRodWJ1c2VyY29udGVudC5jb22CDnd3dy5naXRodWIuY29tggwqLmdp\n"
    "dGh1Yi5jb22CFyouZ2l0aHVidXNlcmNvbnRlbnQuY29tggpnaXRodWIuY29tMD4G\n"
    "A1UdIAQ3MDUwMwYGZ4EMAQICMCkwJwYIKwYBBQUHAgEWG2h0dHA6Ly93d3cuZGln\n"
    "aWNlcnQuY29tL0NQUzAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYBBQUH\n"
    "AwEGCCsGAQUFBwMCMIGfBgNVHR8EgZcwgZQwSKBGoESGQmh0dHA6Ly9jcmwzLmRp\n"
    "Z2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbEcyVExTUlNBU0hBMjU2MjAyMENBMS0x\n"
    "LmNybDBIoEagRIZCaHR0cDovL2NybDQuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xv\n"
    "YmFsRzJUTFNSU0FTSEEyNTYyMDIwQ0ExLTEuY3JsMIGHBggrBgEFBQcBAQR7MHkw\n"
    "JAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBRBggrBgEFBQcw\n"
    "AoZFaHR0cDovL2NhY2VydHMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsRzJU\n"
    "TFNSU0FTSEEyNTYyMDIwQ0ExLTEuY3J0MAwGA1UdEwEB/wQCMAAwggF/BgorBgEE\n"
    "AdZ5AgQCBIIBbwSCAWsBaQB2AE51oydcmhDDOFts1N8/Uusd8OCOG41pwLH6ZLFi\n"
    "mjnfAAABjkN89oAAAAQDAEcwRQIgU/M527Wcx0KQ3II7kCuG5WMuOHRSxKkf1xAj\n"
    "JuSkyPACIQCVX0uurcIA2Ug7ipNN2S1ZygukWqJCh7hjIH0XsrXh8QB2AH1ZHhLh\n"
    "eCp7HGFnfF79+NCHXBSgTpWeuQMv2Q6MLnm4AAABjkN89oEAAAQDAEcwRQIgCxpL\n"
    "BDak+TWKarrCHlZn4DlqwEfAN3lvlgSo21HQuU8CIQDicrb72c0lA2suMWPWT92P\n"
    "FLaRvFrFn9HVzI6Vh50YZgB3AObSMWNAd4zBEEEG13G5zsHSQPaWhIb7uocyHf0e\n"
    "N45QAAABjkN89pQAAAQDAEgwRgIhAPJQX4QArFCjM0sKKzsWLmqmmU8lMhKEYR2T\n"
    "ges1AQyQAiEA2Y3VhP5RG+dapcbwYgVbrTlgWzO7KE/lg1x11CVcz3QwDQYJKoZI\n"
    "hvcNAQELBQADggEBAHKlvzObJBxxgyLaUNCEFf37mNFsUtXmaWvkmcfIt9V+TZ7Q\n"
    "mtvjx5bsd5lqAflp/eqk4+JYpnYcKWrZfM/vMdxPQTeh/VQWewY/hYn6X/V1s2JI\n"
    "MtjqEkW4aotVdWjHVvsx4rAjz5vtub/wVYgtrU8jusH3TVpT9/0AoFhKE5m2IS7M\n"
    "Ig7wKR+DDxoNj4fFFluxteVNgbtwuJcb23NkBQqfHXCvQWqxXZZA4Nwl/WoGPoGG\n"
    "dW5qVOc3BlhtITW53ASyhvKC7HArhj7LwQH8C/dRgn1agIHP9vVJ1NaZnPXhK98T\n"
    "ohv++OO0E/F/bVGNWVnLBQ4v5PjQzRQUTGvM2mU=\n"
    "-----END CERTIFICATE-----\n";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
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
        // Serial.println("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        if (strcmp(evt->header_key, "Content-Length") == 0)
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
        if (isOta)
        {
            if (!receivingFile)
            {
                Serial.println("Update OTA [");
                if (!Update.begin(contentLength))
                { // start with max available size
                    Serial.print("Update begin error: ");
                    Update.printError(Serial);
                }
                receivingFile = true;
            }

            if (Update.write((uint8_t *)evt->data, evt->data_len) != evt->data_len)
            {
                Update.printError(Serial);
            }
        }
        else
        {
            if (!writeOk)
            {
                return ESP_FAIL;
            }

            userData *usrData = (userData *)evt->user_data;
            if (!receivingFile)
            {
                if (!deleteIfExists(*usrData->activeFS, usrData->fileName.c_str()))
                {
                    writeOk = false;
                    return ESP_FAIL;
                }

                Serial.print("Downloading: ");
                Serial.println(usrData->fileName);

                fileToDownload = openFile(*usrData->activeFS, FILE_APPEND, usrData->fileName.c_str());
                if (!fileToDownload)
                {
                    writeOk = false;
                    return ESP_FAIL;
                }
            }

            receivingFile = true;

            if (!esp_http_client_is_chunked_response(evt->client))
            {
                // Serial.print("HTTP_EVENT_ON_DATA CHUNK: ");
                // Serial.println(evt->data_len);
                writeOk = appendFile(fileToDownload, (char *)evt->data, evt->data_len);
            }
            else
            {
                Serial.print("HTTP_EVENT_ON_DATA NOCHUNK: ");
                Serial.println(evt->data_len);
                writeOk = writeFile(*usrData->activeFS, usrData->fileName.c_str(), (char *)evt->data, evt->data_len);
            }

            if (!writeOk)
            {
                Serial.println("Error whjile writing");
            }
        }

        Serial.print(".");

        totalSize += evt->data_len;

        break;
    }
    case HTTP_EVENT_ON_FINISH:
        Serial.println("HTTP_EVENT_ON_FINISH");
        closeFile(fileToDownload);

        if (isOta)
        {
            if (Update.end(true))
            { // true to set the size to the current progress
                Serial.printf("] Update Success: %u\nRebooting...\n", totalSize);
                success = true;
            }
            else
            {
                Update.printError(Serial);
                success = false;
            }
        }
        else
        {
            if (contentLength > 0 && contentLength == totalSize)
            {
                success = true;
            }
            else
            {
                Serial.printf("Write error. Planned to receive %u but received", contentLength, totalSize);
            }

            if (writeOk)
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
        if (!success)
        {
            if (isOta)
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
        // output_len = 0;
        // ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
        // ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        //}
        break;
    }
    return ESP_OK;
}

bool downloadFile2(const char *link, String fileName, fs::FS &fs, bool isOtaUpdate)
{
    WiFiClientSecure *client = new WiFiClientSecure;
    if (client)
    {
        // client->setCACert(rootCACertificate);
        client->setInsecure();

        {
            // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
            HTTPClient https;

            Serial.printf("[HTTPS] begin, connecting to %s\n", link);
            if (https.begin(*client, link))
            { // HTTPS
                Serial.print("[HTTPS] GET...\n");
                // start connection and send HTTP header
                int httpCode = https.GET();

                // httpCode will be negative on error
                if (httpCode > 0)
                {
                    Serial.printf("[HTTPS] GET httpCode: %d\n", httpCode);
                    // file found at server
                    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                    {
                        // get length of document (is -1 when Server sends no Content-Length header)
                        int len = https.getSize();
                        if (len > 0)
                        {
                            if (!deleteIfExists(fs, fileName.c_str()))
                            {
                                writeOk = false;
                                return ESP_FAIL;
                            }

                            Serial.print("Downloading: ");
                            Serial.println(fileName);

                            fileToDownload = openFile(fs, FILE_APPEND, fileName.c_str());
                            if (!fileToDownload)
                            {
                                writeOk = false;
                                return ESP_FAIL;
                            }

                            // get tcp stream
                            WiFiClient *stream = https.getStreamPtr();
                            uint8_t *buff = new uint8_t[10000];

                            File file = openFile(fs, FILE_WRITE, fileName.c_str());
                            // read all data from server
                            while (https.connected() && (len > 0 || len == -1))
                            {
                                // get available data size
                                size_t size = stream->available();

                                if (size)
                                {
                                    // read up to 128 byte
                                    int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                                    file.write(buff, c);
                                    if (len > 0)
                                    {
                                        len -= c;
                                    }

                                    Serial.print('.');
                                }

                                delay(1);
                            }

                            delete buff;
                            file.close();
                            Serial.println("\nDownload complete");
                        }
                    }
                    else
                    {
                        Serial.printf("[HTTP] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
                        return false;
                    }
                }
                else
                {
                    Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
                    return false;
                }

                Serial.println("Http End");
                https.end();
            }
            else
            {
                Serial.printf("[HTTPS] Unable to connect\n");
                return false;
            }

            // End extra scoping block
        }

        delete client;
    }
    else
    {
        Serial.println("Unable to create client");
        return false;
    }

    Serial.println("Returning true");
    return true;
}

bool downloadFile(const char *link, String fileName, fs::FS &fs, bool isOtaUpdate)
{
    success = false;
    fileToDownload = (File)NULL;
    writeOk = true;
    contentLength = -1;
    Serial.print("downloadingFile: ");
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
    // config->url = "http://httpbin.org/stream/1";
    //    config->url = "http://www.sci.utah.edu/~macleod/docs/txt2html/sample.txt";
    config->url = link;
    config->event_handler = _http_event_handler;
    config->method = HTTP_METHOD_GET;
    config->path = "/";
    config->username = "";
    config->password = "";
    config->query = "";
    config->max_redirection_count = 2;
    config->disable_auto_redirect = false;
    config->user_data = user;
    config->is_async = false;
    config->buffer_size = 1024;
    config->transport_type = HTTP_TRANSPORT_OVER_SSL;          // Specify transport type
    config->crt_bundle_attach = arduino_esp_crt_bundle_attach; // Attach the certificate bundle

    esp_http_client_handle_t client = esp_http_client_init(config);
    esp_err_t err = esp_http_client_perform(client);
    Serial.println("Created esp_http_client_perform");

    if (err == ESP_OK)
    {
        Serial.print("HTTP chunk encoding OK: ");
        Serial.println(esp_http_client_get_status_code(client));
        Serial.println(esp_http_client_get_content_length(client));
    }
    else
    {
        Serial.print("Error perform http request: ");
        Serial.println(esp_err_to_name(err));
    }

    Serial.println("http client cleanup");
    esp_http_client_cleanup(client);
    delete user;
    delete config;

    return success;
}