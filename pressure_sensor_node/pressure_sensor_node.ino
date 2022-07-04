#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char *client_name = "fsr";
const char *ssid = "";                                 // The SSID (name) of the Wi-Fi network you want to connect to
const char *password = "";                             // The password of the Wi-Fi network
const char *websockets_adress = "192.168.178.53/mqtt"; // ws adress
const int websockets_port = 8084;                      // ws port
WebSocketsClient ws_client;

const int FSR_1_PIN = 16;
const int FSR_2_PIN = 5;

int fsr1Read;
int fsr2Read;
bool sensorsSetup;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("Disconnected!\n");
    break;
  case WStype_CONNECTED:
  {
    Serial.printf("Connected!\n");
  }
  break;
  case WStype_TEXT:
    Serial.printf("Get text: %s\n", payload);
    playNotification();
    break;
  case WStype_BIN:
    Serial.printf("Get binary length: %u\n", length);
    hexdump(payload, length);
    break;
  }
}

void setup()
{
  WiFi.hostname("posture-control");
  WiFi.begin(ssid, password); // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  Serial.println("Starting Websocket Server...");
  ws_client.begin(websockets_adress, websockets_port, strcat("/ws?client=", client_name));
  ws_client.onEvent(webSocketEvent);
  ws_client.enableHeartbeat(15000, 3000, 2);
  ws_client.setReconnectInterval(5000);

  Serial.println("Setting up sensors...");
  setupSensors();
}

void setupSensors()
{
  pinMode(FSR_1_PIN, OUTPUT);
  pinMode(FSR_2_PIN, OUTPUT);
  while (!Serial)
    delay(10);

  sensorsSetup = true;
  client.publish(topic, "FSR Sensors Setup Finished");
}

int analogReadOnDigital(int readPin)
{
  if (readPin == 1)
  {
    digitalWrite(FSR_1_PIN, HIGH);
    digitalWrite(FSR_2_PIN, LOW);
  }
  else
  {
    digitalWrite(FSR_2_PIN, HIGH);
    digitalWrite(FSR_1_PIN, LOW);
  }

  return analogRead(0);
}

void loop()
{
  ws_client.loop();
  if (sensorsSetup)
  {

    fsr1Read = analogReadOnDigital(1);
    delay(100);
    fsr2Read = analogReadOnDigital(2);
    delay(100);

    StaticJsonDocument<64> doc;

    doc["type"] = "fsr";
    doc["fsr1"] = fsr1Read;
    doc["fsr2"] = fsr2Read;

    String output;
    serializeJson(doc, output);
    ws_client.sendTXT(output);
    Serial.println(output);
    delay(100);
  }
}
