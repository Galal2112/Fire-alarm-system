#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
// Library for interacting with the Telegram API
// Search for "Telegegram" in the Library manager and install
// The universal Telegram library
// https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot


#include <ArduinoJson.h>
#include <iostream>
#include <sstream>
// Library used for parsing Json from the API responses
// NOTE: There is a breaking change in the 6.x.x version,
// install the 5.x.x version instead
// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

//------- Replace the following! ------

char ssid[] = "Home";         // your network SSID (name)
char password[] = "G87783F8946MYUL3"; // your network password

#define TELEGRAM_BOT_TOKEN "1892711791:AAGU-inz6WXzT_SLhw7uiFuvUV_mMbA7p5w"

// This is the Wifi client that supports HTTPS
WiFiClientSecure client;
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, client);


//#define ONE_WIRE_BUS D7

OneWire oneWire(D7);

DallasTemperature sensors(&oneWire);

float Celsius = 0;
String chat_id = "";
float Fahrenheit = 0;
int delayBetweenChecks = 1000;
unsigned long lastTimeChecked;   //last time messages' scan has been done

void setup() {
    sensors.begin();
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    // attempt to connect to Wifi network:
    Serial.print("Connecting Wifi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Only required on 2.5 Beta
    client.setInsecure();


    // longPoll keeps the request to Telegram open for the given amount of seconds if there are no messages
    // This hugely improves response time of the bot, but is only really suitable for projects
    // where the the initial interaction comes from Telegram as the requests will block the loop for
    // the length of the long poll
    bot.longPoll = 60;
}

void handleNewMessages(int numNewMessages) {

    for (int i = 0; i < numNewMessages; i++) {

        // If the type is a "callback_query", a inline keyboard button was pressed
        if (bot.messages[i].type ==  F("callback_query")) {
            String text = bot.messages[i].text;
            Serial.print("Call back button pressed with text: ");
            Serial.println(text);

            if (text == F("WEATHER")) {
                //Send weather
            } else if (text == F("HOME_TEMPERATURE")) {
                std::ostringstream message;
                message << "Current temperature is " << Celsius << "\n" << Celsius;
                std::string var = message.str();
                bot.sendMessage(chat_id, reinterpret_cast<String &&>(message), "Markdown");

            }
        } else {
            chat_id = String(bot.messages[i].chat_id);
            String text = bot.messages[i].text;

            if (text == F("/options")) {

                // Keyboard Json is an array of arrays.
                // The size of the main array is how many row options the keyboard has
                // The size of the sub arrays is how many coloums that row has

                // "The Text" property is what shows up in the keyboard
                // The "callback_data" property is the text that gets sent when pressed

                String keyboardJson = F("[[{ \"text\" : \"Weather\", \"callback_data\" : \"WEATHER\" },{ \"text\" : \"Home Temperature\", \"callback_data\" : \"HOME_TEMPERATURE\" }]]");
                bot.sendMessageWithInlineKeyboard(chat_id, "Your ", "", keyboardJson);
            }

            // When a user first uses a bot they will send a "/start" command
            // So this is a good place to let the users know what commands are available
            if (text == F("/start")) {

                bot.sendMessage(chat_id, "/options : returns the inline keyboard\n", "Markdown");
            }
        }
    }
}

void loop() {
    sensors.requestTemperatures();

    Celsius = sensors.getTempCByIndex(0);
    Fahrenheit = sensors.toFahrenheit(Celsius);

    Serial.print(Celsius);
    Serial.print(" C  ");
    Serial.print(Fahrenheit);
    Serial.println(" F");

    if (millis() > lastTimeChecked + delayBetweenChecks)  {

        // getUpdates returns 1 if there is a new message from Telegram
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        Serial.println(numNewMessages);

        if (numNewMessages) {
            Serial.println("got response");
            handleNewMessages(numNewMessages);
        }

        lastTimeChecked = millis();
    }
    delay(1000);
}
