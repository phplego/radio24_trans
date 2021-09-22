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
int PIN_LED = D4; //(D4 - built in)
int PIN_BTN = 0;
int PIN_WIFI_ENABLE = 16;


void sendRadioMessage(String action, long id)
{
    //randomSeed(micros());

    String pld = action + " " + id;
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
        digitalWrite(PIN_LED, LOW);

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
            delay(50); // make led flashing visible
        }
    }
    else
    {
        Serial.println("NO REPLAY!");
    }
    digitalWrite(PIN_LED, HIGH);

}

void fpm_wakup_cb_func(void) {
    Serial.println("Light sleep is over, either because timeout or external interrupt");
    Serial.flush();
}


void goToLightSleep()
{
    // for timer-based light sleep to work, the os timers need to be disconnected
    extern os_timer_t *timer_list;
    timer_list = nullptr;

    // enable light sleep
    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
    wifi_fpm_open();


    wifi_fpm_set_wakeup_cb(fpm_wakup_cb_func);

    // optional: register one or more wake-up interrupts. the chip
    // will wake from whichever (timeout or interrupt) occurs earlier
    //gpio_pin_wakeup_enable(D2, GPIO_PIN_INTR_HILEVEL);

    // sleep for 10 seconds
    long sleepTimeMilliSeconds = 10e3;
    // light sleep function requires microseconds
    wifi_fpm_do_sleep(sleepTimeMilliSeconds * 1000);

    // timed light sleep is only entered when the sleep command is
    // followed by a delay() that is at least 1ms longer than the sleep
    delay(sleepTimeMilliSeconds + 1);    
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

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH); // turn led off 

    pinMode(PIN_BTN, INPUT_PULLUP); // flash button
    pinMode(PIN_WIFI_ENABLE, INPUT_PULLDOWN_16);


    nrf24.init();

    if (!nrf24.setChannel(4))
    {
        Serial.println("setChannel failed");
    }

    if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::RFM73TransmitPowerm0dBm))
    {
        Serial.println("setRF failed");
    }

    sendRadioMessage("start", random(1000, 9999));


    if(digitalRead(PIN_WIFI_ENABLE) == HIGH){
        sendRadioMessage("WIFI_MODE", random(1000, 9999));
        setupWifiMode();
    }
    else {
        sendRadioMessage("RADIO_MODE", random(1000, 9999));
        //goToLightSleep();
    }

}





void radioLoop()
{
    int buttonPressed = digitalRead(PIN_BTN) == LOW;
    
    if(buttonPressed)
    {
        sendRadioMessage("btn1", random(1000, 9999));
    }
    

    while(digitalRead(PIN_BTN) == LOW) // while button pressed, sleep
        delay(1);
}

// TRANSMITTER
void loop()
{
    ArduinoOTA.handle();
    radioLoop();
}