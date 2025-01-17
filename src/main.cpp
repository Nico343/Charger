#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

#include "FS.h"
#include <LITTLEFS.h>

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

DNSServer dnsServer;
AsyncWebServer server(80);

String user_name;
String proficiency;
bool name_received = false;
bool proficiency_received = false;

class CaptiveRequestHandler : public AsyncWebHandler
{
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request)
  {
    // request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request)
  {
    request->send(LittleFS, "/index.html", "text/html", false);
  }
};

void setupServer()
{
  server.serveStatic("/", LittleFS, "/");
  server.onNotFound([](AsyncWebServerRequest *request)
                    {
      if (request->url() == "/") {
        request->send(LittleFS, "/index.html", "text/html");
      } else {
        request->send(404, "text/plain", "Not Found");
      }
      Serial.println("Client Connected"); });
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //     request->send(LittleFS, "/index.html","text/html", false);
  //     request->send(LittleFS, "/style.css", "text/css", false);
  //     request->send(LittleFS, "/app.js", "text/js", false);
  //     Serial.println("Client Connected"); });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String inputMessage;
              String inputParam;

              if (request->hasParam("Ton"))
              {
                inputMessage = request->getParam("Ton")->value();
                inputParam = "Ton";
                user_name = inputMessage;
                Serial.println(inputMessage);
                name_received = true;
              }
              else
              {
                Serial.println("Doesn't have Ton");
              }

              if (request->hasParam("Toff"))
              {
                inputMessage = request->getParam("Toff")->value();
                inputParam = "Toff";
                proficiency = inputMessage;
                Serial.println(inputMessage);
                proficiency_received = true;
              }
              else
              {
                Serial.println("Doesn't have Toff");
              }

              if (request->url() == "/get")
              {
                request->send(LittleFS, "/Scrap.html", "text/html");
                Serial.println("/get");
              }

              // request->send(200, "text/html", "The values entered by you have been successfully sent to the device <br><a href=\"/\">Return to Home Page</a>");
            });
}

void setup()
{
  // your other setup stuff...
  Serial.begin(115200);
  if (!LittleFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting LITTLEFS");
    return;
  }

  // List contents of SPIFFS
  listDir(LittleFS, "/", 0);

  Serial.println();
  Serial.println("Setting up AP Mode");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("VCU");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Setting up Async WebServer");
  setupServer();
  Serial.println("Starting DNS Server");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); // only when requested from AP
  // more handlers...
  server.begin();
  Serial.println("All Done!");
}

void loop()
{
  dnsServer.processNextRequest();
  if (name_received && proficiency_received)
  {
    Serial.print("Hello ");
    Serial.println(user_name);
    Serial.print("You have stated your proficiency to be ");
    Serial.println(proficiency);
    name_received = false;
    proficiency_received = false;
    Serial.println("We'll wait for the next client now");
  }
}