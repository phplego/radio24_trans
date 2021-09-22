#include <SPI.h>
#include <RH_NRF24.h>
#include <Wire.h>
#include <EEPROM.h>

//#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>


// Singleton instance of the radio driver
RH_NRF24 nrf24(D1, D2); // use this for NodeMCU Amica/AdaFruit Huzzah ESP8266 Feather
// RH_NRF24 nrf24(8, 7); // use this to be electrically compatible with Mirf
// RH_NRF24 nrf24(8, 10);// For Leonardo, need explicit SS pin
// RH_NRF24 nrf24(8, 7); // For RFM73 on Anarduino Mini
int LED = D4; //(D4 - built in)


void sendRadioMessage()
{
    randomSeed(micros());
    long randomId = random(1000, 9999);


    String pld = String() + "myid " + randomId;
    nrf24.send((uint8_t *)pld.c_str(), pld.length());
    nrf24.waitPacketSent();
    Serial.print("> ");
    Serial.print(pld);
    Serial.print(" ");
    
    long sendTime = millis();
    // Now wait for a reply
    uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (nrf24.waitAvailableTimeout(20))
    {
        digitalWrite(LED, LOW);

        // Should be a reply message for us now
        while (nrf24.recv(buf, &len))
        {

            Serial.print("got reply: ");
            for(int i=0; i < len; i++)
                Serial.print((char)buf[i]);
            Serial.print(" in ");
            Serial.print(millis()-sendTime);
            Serial.print(" ms");
            Serial.println();
            delay(50);
        }
    }
    else
    {
        Serial.println("NO REPLAY!");
    }
    digitalWrite(LED, HIGH);

}


void setupWifiMode()
{
    WiFiManager wifiManager;

    String deviceName = "esp-nrf24transmitter";
    WiFi.mode(WIFI_STA); // no access point after connect

    wifi_set_sleep_type(NONE_SLEEP_T); // prevent wifi sleep (stronger connection)

    // On Access Point started (not called if wifi is configured)
    wifiManager.setAPCallback([](WiFiManager* mgr){
        Serial.println( String("Please connect to Wi-Fi"));
        Serial.println( String("Network: ") + mgr->getConfigPortalSSID());
        Serial.println( String("Password: 12341234"));
        Serial.println( String("Then go to ip: 10.0.1.1"));
    });

    wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    wifiManager.setConfigPortalTimeout(60);
    wifiManager.autoConnect(deviceName.c_str(), "12341234"); // IMPORTANT! Blocks execution. Waits until connected

    // Restart if not connected
    if (WiFi.status() != WL_CONNECTED) 
    {
        ESP.restart();
    }

    // Host name should be set AFTER the wifi connect
    WiFi.hostname(deviceName);

    // Initialize OTA (firmware updates via WiFi)
    ArduinoOTA.begin();
}

void setup()
{
    Serial.begin(74880);
    Serial.println();
    Serial.print("Transmitter Started");
    Serial.println();

    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH); // turn led off 

    pinMode(0, INPUT_PULLUP); // flash button


    nrf24.init();

    if (!nrf24.setChannel(4))
    {
        Serial.println("setChannel failed");
    }

    if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::RFM73TransmitPowerm0dBm))
    {
        Serial.println("setRF failed");
    }

    sendRadioMessage();

    setupWifiMode();
}





void radioLoop()
{
    int buttonPressed = digitalRead(0) == LOW;
    
    if(buttonPressed)
    {
        sendRadioMessage();
    }
    

    while(digitalRead(0) == LOW) // while button pressed, sleep
        delay(1);
}

// TRANSMITTER
void loop()
{
    ArduinoOTA.handle();
    radioLoop();
}