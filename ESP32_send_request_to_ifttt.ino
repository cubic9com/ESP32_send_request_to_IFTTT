#include <WiFiClientSecure.h>
#include "esp_deep_sleep.h"
#include "wifi_config.h"
#include "ifttt_config.h"

// hostname of Maker of IFTTT Platform
const char*  HOSTNAME = "maker.ifttt.com";

// the certificate of root certificate authority for maker.ifttt.com.
const char* ROOT_CA_CERT = \
     "-----BEGIN CERTIFICATE-----\n" \
     "MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx\n" \
     "EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT\n" \
     "EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp\n" \
     "ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz\n" \
     "NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH\n" \
     "EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE\n" \
     "AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw\n" \
     "DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD\n" \
     "E6bFIEMBO4Tx5oVJnyfq9oQbTqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH\n" \
     "/PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy\n" \
     "DfMAZiLIjWltNowRGLfTshxgtDj6AozO091GB94KPutdfMh8+7ArU6SSYmlRJQVh\n" \
     "GkSBjCypQ5Yj36w6gZoOKcUcqeldHraenjAKOc7xiID7S13MMuyFYkMlNAJWJwGR\n" \
     "tDtwKj9useiciAF9n9T521NtYJ2/LOdYq7hfRvzOxBsDPAnrSTFcaUaz4EcCAwEA\n" \
     "AaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE\n" \
     "FDqahQcQZyi27/a9BUFuIMGU2g/eMA0GCSqGSIb3DQEBCwUAA4IBAQCZ21151fmX\n" \
     "WWcDYfF+OwYxdS2hII5PZYe096acvNjpL9DbWu7PdIxztDhC2gV7+AJ1uP2lsdeu\n" \
     "9tfeE8tTEH6KRtGX+rcuKxGrkLAngPnon1rpN5+r5N9ss4UXnT3ZJE95kTXWXwTr\n" \
     "gIOrmgIttRD02JDHBHNA7XIloKmf7J6raBKZV8aPEjoJpL1E/QYVN8Gb5DKj7Tjo\n" \
     "2GTzLH4U/ALqn83/B2gX2yKQOC16jdFU8WnjXzPKej17CuPKf1855eJ1usV2GDPO\n" \
     "LPAvTK33sefOT6jEm0pUBsV/fdUID+Ic/n4XuKxe9tQWskMJDE32p2u0mYRlynqI\n" \
     "4uJEvlz36hz1\n" \
     "-----END CERTIFICATE-----\n" \
;

WiFiClientSecure client;

void setup() {
  Serial.begin(115200);

  delay(100);

  if (digitalRead(GPIO_NUM_0) == 0) {
    if (connect_to_wifi()) {
      trigger_event();
    }
  }

  start_deep_sleep();
}

void loop() {
  // do nothing
}

// connect to the WiFi network of home
bool connect_to_wifi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);
  // attempt to connect to Wifi network:
  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
    retry_count++;
    if (retry_count > 30) {
      Serial.println("WiFi connection failed!");      
      return false;
    }
  }

  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  return true;
}

// trigger an event of Maker
void trigger_event() {
  client.setCACert(ROOT_CA_CERT);

  Serial.println("\nStarting connection to server...");
  if (!client.connect(HOSTNAME, 443)) {
    Serial.println("Server connection failed!");
  } else {
    Serial.println("Connected to server!");

    // Make a HTTP request:
    client.print("GET ");
    client.print("/trigger/");
    client.print(EVENT_NAME);
    client.print("/with/key/");
    client.print(KEY);
    client.println(" HTTP/1.0");
    client.print("Host: ");
    client.println(HOSTNAME);
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }
    Serial.println();

    client.stop();
  }
}

// enable wakeup using GPIO 0 and start deep sleep mode
void start_deep_sleep() {
  esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);

  gpio_pullup_en(GPIO_NUM_0);
  gpio_pulldown_dis(GPIO_NUM_0);

  esp_deep_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); //1 = High, 0 = Low

  delay(1000);
  Serial.println("Going to sleep now");

  esp_deep_sleep_start();
}

