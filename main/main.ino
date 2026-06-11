#include <WiFi.h>
#include <ESPmDNS.h>
#include <FastLED.h>

#define NUM_LEDS   25
#define DATA_PIN   4
#define BRIGHTNESS 128

const char *ssid = "Net4me";
const char *password = "brousovaubych";
NetworkServer server(80);
CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connecting to "); Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  MDNS.begin("lantern");
}

void loop()
{
  NetworkClient client = server.accept();

  if (client) //if new client
  {
    Serial.println("New Client.");
    String currentLine = "";
    unsigned long timeout = millis();

    while (client.connected() && millis() - timeout < 800) // loop if client's connected, 2s timeout
    {
      if (client.available())
      {
        timeout = millis(); // reset timeout on data
        char c = client.read();

        if (c == '\n')
        {
          if (currentLine.length() == 0)
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html"); client.println();

            //site contents
            client.print("<!DOCTYPE html>");
            client.print("<html><head><meta charset=\"UTF-8\"><style>");
            client.print("body{margin:0;height:100vh;display:flex;flex-direction:column;justify-content:center;align-items:center;gap:24px;background:#0a1a3a;font-family:sans-serif}");
            client.print("button{padding:14px 40px;font-size:18px;border:0;border-radius:8px;background:#fff;color:#0a1a3a;font-weight:700}");
            client.print("button:active{transform:scale(.95)}");
            client.print("input[type=range]{width:200px;accent-color:#fff}");
            client.print("</style></head><body>");
            client.print("<button onclick=\"fetch('/on')\">On</button><button onclick=\"fetch('/off')\">Off</button>");
            client.print("<input type=range min=0 max=255 value=128 oninput=\"fetch('/brightness?v='+this.value)\">");
            client.print("</body></html>");

            client.println(); break; //exits client connected loop
          }
          else currentLine = "";
        }
        else if (c != '\r') currentLine += c;

        //button functions
        if (currentLine.endsWith("/on")) { 
          fill_solid(leds, NUM_LEDS, CRGB::White); 
          FastLED.show(); 
          }
        if (currentLine.endsWith("/off")) { fill_solid(leds, NUM_LEDS, CRGB::Black); FastLED.show(); }

        //slider brightness
        if (currentLine.indexOf("/brightness?v=") >= 0 && currentLine.endsWith(" HTTP/1.1")) {
          int idx = currentLine.indexOf("?v=") + 3;
          int val = currentLine.substring(idx).toInt();
          FastLED.setBrightness(val);
          FastLED.show();
        }
      }
    }

    // close the connection if client not connected
    client.stop();
    Serial.println("Client Disconnected.");
  }
}