#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>


/**
 * My Wifi access data and APIs
 * **/
char ssid[] = "network_SSID";
char password[] = "network _assword";

#define TELEGRAM_BOT_TOKEN "SET_ME"

// This is the Wifi client that supports HTTPS
WiFiClientSecure telegramClient;
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, telegramClient);

// Open Weather Map API
WiFiClient weatherClient;
String latitude = "52.5790";
String longitude = "13.2805";
String apiKey = "SET_ME";
DynamicJsonDocument jsonDocument(2048);

// My Arduino pin
OneWire oneWire(D7);
DallasTemperature sensors(&oneWire);

float Celsius = 0;
float currentLocationTemp;
String chat_id = "";
int delayBetweenChecks = 1000; //Telegram
unsigned long lastTimeChecked;   //last time messages' scan has been done
int delayBetweenWeatherChecks = 5 * 60 * 1000;
unsigned long lastTimeCheckedWeather = 0;

void getWeather() {
    Serial.println("\nStarting connection to server...");
    // if you get a connection, report back via serial:
    if (weatherClient.connect("api.openweathermap.org", 80)) {
        Serial.println("connected to server");
        // Make a HTTP request:
        String url = "";
        url = url + String("/data/2.5/onecall?lat=") + String(latitude + "&lon=") + String(longitude) +
              "&exclude=alerts,minutely,hourly,daily&appid=" + apiKey + String("&units=metric");

        // Make a HTTP request
        String http = "GET ";
        http = http + url + " HTTP/1.1\r\n";
        http = http + "Host: " + String("api.openweathermap.org") + "\r\n";
        http = http + "Connection: close\r\n";
        http = http + "Content-Type: application/json\r\n";
        http = http + "\r\n";
        weatherClient.print(http);
        delay(500);
    } else {
        Serial.println("unable to connect");
    }

    String response = weatherClient.readStringUntil('\r');
    Serial.println(response);

    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!weatherClient.find(endOfHeaders)) {
        Serial.println(F("Invalid response"));
        return;
    }

    DeserializationError error = deserializeJson(jsonDocument, weatherClient);
    if (error) {
        Serial.println("parseObject() failed");
        Serial.println(error.c_str());
        return;
    }
    //get the data from the json tree
    currentLocationTemp = jsonDocument["current"]["temp"];
    Serial.println(currentLocationTemp);
    // Disconnect from the server
    weatherClient.stop();
}

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

    telegramClient.setInsecure();

    getWeather();

    bot.longPoll = 20;
}
void handleNewMessages(int numNewMessages) {

    for (int i = 0; i < numNewMessages; i++) {
        if (bot.messages[i].chat_id.equals("1610750027")) {
            // If the type is a "callback_query", a inline keyboard button was pressed
            if (bot.messages[i].type == F("callback_query")) {
                String text = bot.messages[i].text;
                Serial.print("Call back button pressed with text: ");
                Serial.println(text);

                if (text == F("WEATHER")) {
                    getWeather();
                    String message = String("Weather temperature is ") + currentLocationTemp + "°C\n";
                    bot.sendMessage(chat_id, message, "Markdown");
                } else if (text == F("HOME_TEMPERATURE")) {
                    Celsius = sensors.getTempCByIndex(0);
                    String message = String("Current temperature is ") + Celsius + "°C\n";
                    bot.sendMessage(chat_id, message, "Markdown");

                }
            } else {
                chat_id = String(bot.messages[i].chat_id);
                String text = bot.messages[i].text;

                if (text == F("/options")) {
                    String keyboardJson = F(
                            "[[{ \"text\" : \"Weather\", \"callback_data\" : \"WEATHER\" },{ \"text\" : \"Home Temperature\", \"callback_data\" : \"HOME_TEMPERATURE\" }]]");
                    bot.sendMessageWithInlineKeyboard(chat_id, "Your Home Temperature Alarm", "", keyboardJson);
                }

                // When a user first uses a bot they will send a "/start" command
                // So this is a good place to let the users know what commands are available
                if (text == F("/start")) {

                    bot.sendMessage(chat_id, "/options : returns the available commands\n", "Markdown");
                }
            }
        } else {
            bot.sendMessage(bot.messages[i].chat_id, "Unauthorized\n", "Markdown");
        }
    }
}

void loop() {
    sensors.requestTemperatures();
    Celsius = sensors.getTempCByIndex(0);
    Serial.print(Celsius);
    Serial.println(" C  ");

    if (millis() > lastTimeCheckedWeather + delayBetweenWeatherChecks) {
        getWeather();
        lastTimeCheckedWeather = millis();
    }

    if (Celsius > (currentLocationTemp + 15)) {
        String message = "🔥🔥🔥Fire🔥🔥🔥";
        bot.sendMessage(chat_id, message, "Markdown");
    }

    if (millis() > lastTimeChecked + delayBetweenChecks) {

        // getUpdates returns 1 if there is a new message from Telegram
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

        if (numNewMessages) {
            Serial.println("got response");
            handleNewMessages(numNewMessages);
        }

        lastTimeChecked = millis();
    }
    delay(100);
}
