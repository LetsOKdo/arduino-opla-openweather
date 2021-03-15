#include "arduino_secrets.h"
#include <Arduino_MKRIoTCarrier.h>
#include <WiFiNINA.h>
#include <Arduino_JSON.h>
#include "Images.h"

MKRIoTCarrier carrier;

unsigned int threshold = 4;
unsigned int threshold_btn_0 = 3;
unsigned int threshold_btn_1 = 5;
unsigned int threshold_btn_2 = 5;
unsigned int threshold_btn_3 = 5;
unsigned int threshold_btn_4 = 5;

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASSWD;
char appid[] = SECRET_APPID;
char weather_loc[] = SECRET_LOC;

uint32_t green = carrier.leds.Color(255, 0, 0);

const char server[] = "api.openweathermap.org";
const long interval = 10000; // milliseconds
const int calibrate = -2; // Temperature calibration

int button_state = 0;
int backlight_state = HIGH;
unsigned long previous_t = 0;
unsigned long current_t = 0;
String weather = "";
String town = "";
double temp_in = 0;
double temp_out = 0;

WiFiClient client;

void setup() {
  //Serial.begin(9600);
  //while (!Serial);

  //CARRIER_CASE = true;
  carrier.Buttons.updateConfig(threshold);
  carrier.Button0.updateConfig(threshold_btn_0);
  carrier.Button1.updateConfig(threshold_btn_1);
  carrier.Button2.updateConfig(threshold_btn_2);
  carrier.Button3.updateConfig(threshold_btn_3);  
  carrier.Button4.updateConfig(threshold_btn_4);
  carrier.begin();
  carrier.display.setRotation(0);
  
  uint16_t color = 0x04B3; // Arduino logo colour
  carrier.display.fillScreen(ST77XX_BLACK);  
  carrier.display.drawBitmap(44, 60, ArduinoLogo, 152, 72, color);
  carrier.display.drawBitmap(48, 145, ArduinoText, 144, 23, color);  
}

void loop() {
  switch (button_state) {
    case 0:
      current_t = millis();
      if (current_t - previous_t >= interval) {
        previous_t = current_t;
        //Turn off backlight to save energy
        if (backlight_state == HIGH) {
          backlight_state = LOW;
          digitalWrite(TFT_BACKLIGHT, backlight_state);
        }
      }
      
      pulseLoop(); // Pulse LEDs

      carrier.Buttons.update(); // Check touch pads
      
      if (carrier.Button0.onTouchDown()) {
        button_state = 3;
      }

      if (carrier.Button2.onTouchDown()) {
        button_state = 1;
      }

      if (carrier.Button4.onTouchDown()) {
        button_state = 2;
      }
      break;
      
    case 1:
      showButtonLed(2);
      showText("Updating");
      connectWiFi();
      requestData();
      parseData();
      showWeather(town, weather);
      disconnectWiFi();
      
      changeState(0);
      break;
      
    case 2:
      showButtonLed(4);
      showValue("Outdoor", temp_out, "C");
      
      changeState(0);      
      break;
      
    case 3:
      showButtonLed(0);
      updateSensors();
      showValue("Indoor", temp_in + calibrate, "C");
      
      changeState(0);      
      break;
  }
  delay(10); // Short delay
}

void changeState(const int state) {
  previous_t = millis();      
  button_state = state;
  delay(250); // Delay for stability 
}

void showButtonLed(const int button) {
  carrier.Buzzer.sound(30);
  carrier.leds.setPixelColor(button, 0);
  carrier.leds.show();
  delay(50);  // Buzz length
  carrier.Buzzer.noSound();
}

void showValue(const char* name, float temp, const char* unit) {
  previous_t = millis();  
  if (backlight_state == LOW) {
    backlight_state = HIGH;
    digitalWrite(TFT_BACKLIGHT, backlight_state);
  }

  carrier.display.fillScreen(ST77XX_BLUE);
  carrier.display.setCursor(60, 50);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(3);
  carrier.display.print(name);
  carrier.display.setTextSize(7);
  // Centre display
  if (temp > 9) {
    carrier.display.setCursor(50, 110);
  }
  else {
    carrier.display.setCursor(40, 110);
  }
  carrier.display.print(String(temp, 0));
  carrier.display.print(unit);
}

void showText(const char* text) {
  previous_t = millis();  
  if (backlight_state == LOW) {
    backlight_state = HIGH;
    digitalWrite(TFT_BACKLIGHT, backlight_state);
  }
  
  carrier.display.fillScreen(ST77XX_BLUE);    
  carrier.display.setCursor(20, 100);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(4);
  carrier.display.print(text);  
}

void connectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    // wait for connection:
    delay(1000);
  }
  Serial.println(WiFi.firmwareVersion());
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
}

void requestData() {
  if (client.connect(server, 80)) {
    // Make a HTTP request:
    client.print("GET ");
    client.print("/data/2.5/weather?q=");
    client.print(weather_loc);
    client.print("&units=metric&appid=");
    client.print(appid);
    client.println(" HTTP/1.0");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
  }
}

void parseData() {
  static String line = "";
  while (client.connected()) {
    line = client.readStringUntil('\n');
    Serial.println(line);
    JSONVar myObject = JSON.parse(line);
 
    weather = JSON.stringify(myObject["weather"][0]["main"]);
    weather.replace("\"", "");
    town = JSON.stringify(myObject["name"]);
    town.replace("\"", "");        
    temp_out = myObject["main"]["temp"];

    if (line.startsWith("{")) {
      break;
    }
  }  
}

void showWeather(String town, String weather) {
  carrier.display.fillScreen(ST77XX_BLUE);
  carrier.display.setCursor(60, 50);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(3);
  carrier.display.print(town);
  carrier.display.setCursor(50, 110);
  carrier.display.setTextSize(4);  
  carrier.display.print(weather);  
}

// Disconnect WiFi to save energy
void disconnectWiFi() {
  client.stop();
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
    Serial.println("Disconnected...");
    WiFi.end();
    Serial.println("Ended...");    
  }
}

void updateSensors() {
  temp_in = carrier.Env.readTemperature();
}

void pulseLoop() {
  static unsigned int i = 0;
  const int max_brightness = 10;
  // Convert mod i degrees to radians
  float b = (i++ % 180) * PI / 180; 
  b = sin(b) * max_brightness;
  carrier.leds.setPixelColor(0, green);
  carrier.leds.setPixelColor(2, green);
  carrier.leds.setPixelColor(4, green);          
  carrier.leds.setBrightness(b);
  carrier.leds.show();
}