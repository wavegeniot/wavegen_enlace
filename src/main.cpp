// DEVICE 1 - WAVEGEN-1
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <DigiPotX9Cxxx.h>

SoftwareSerial SerialPi(5, 4);
DigiPot pot(12, 13, 14);

int con_status = 0;
char *type = 0;
char *amplitude = 0;
char *frequency = 0;
const char *id = "e07dba31-e2b0-4351-b4d2-ebcf94ca590f";
const char *ssid = "-";             // Enter SSID
const char *password = "-"; // Enter Password

const char *websocketURL = "wss://zmrakffklsmaouajknzy.supabase.co/realtime/v1/websocket?apikey=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InptcmFrZmZrbHNtYW91YWprbnp5Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3MDY1NjcwMDgsImV4cCI6MjAyMjE0MzAwOH0.1NQvhDko1YI-441l2mK2CiTtXdo3WGxbu6Rfg7FYzVM&vsn=1.0.0";
const char *websocketUR = "wss://socketsbay.com/wss/v2/1/demo/";

unsigned long previousMillis = 0;
unsigned long previousMillis_Piserial = 0;
const long interval = 20000;
const long interval_Piserial = 5000;

using namespace websockets;
WebsocketsClient client;

int sentRef = 1;

JsonDocument wave;

float max_ampl_sine = 0.48;
float max_ampl_square = 0.89;

float offset_sine = 0.5;
float offset_square = 0;

String TerminationChar = "\r\n";

void setupSine();
void setupSquare();
void setupSaw();
void sendWave();

void sendPayload(const char *topic, const char *event, const char *payload)
{
    // Construct the JSON payload
    JsonDocument doc;
    JsonDocument payload_json;

    deserializeJson(payload_json, payload);

    doc["topic"] = topic;
    doc["event"] = event;
    doc["payload"] = payload_json;
    doc["ref"] = sentRef;
    doc["join_ref"] = "1";

    // Serialize the JSON payload
    char buffer[1024];
    size_t n = serializeJson(doc, buffer);

    // Send the payload over WebSocket
    client.send(buffer, n);
    sentRef = sentRef + 1;
}

void deleteChar(char *str, int index)
{
    int len = strlen(str);

    if (index >= 0 && index < len)
    {
        for (int i = index; i < len - 1; i++)
        {
            str[i] = str[i + 1];
        }
        str[len - 1] = '\0'; // Null terminate the string
    }
}

void onMessageCallback(WebsocketsMessage message)
{
    Serial.print("Got Message: ");
    char buffer[1024];
    JsonDocument json_message;

    deserializeJson(json_message, message.data());

    const char *event = json_message["payload"]["data"]["type"];

    if (event)
    {
        serializeJson(json_message["payload"]["data"]["record"]["id"], buffer);
        Serial.println(buffer);
        if (strcmp(buffer, id))
        {
            serializeJson(json_message["payload"]["data"]["record"]["data"], buffer);
            char *token = strtok(buffer, ";");
            int i = 0;
            while (token != NULL)
            {
                switch (i)
                {
                case 0:
                    type = token;
                    break;
                case 1:
                    amplitude = token;
                default:
                    frequency = token;
                    break;
                }
                token = strtok(NULL, ";");
                i = i + 1;
            }
        }

        deleteChar(type, 0);
        deleteChar(frequency, strlen(frequency) - 1);

        switch (atoi(type))
        {
        case 1:
            setupSquare();
            break;
        case 2:
            setupSine();
            break;
        case 3:
            setupSaw();
            break;
        default:
            break;
        }
        if (strcmp(type, "2") == 0)
        {
            setupSine();
        }
    }
}

void onEventsCallback(WebsocketsEvent event, String data)
{
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        sendPayload("realtime:test_1", "phx_join", "{\"config\":{\"broadcast\":{\"ack\":false,\"self\":false},\"presence\":{\"key\":\"\"},\"postgres_changes\":[{\"event\":\"*\",\"schema\":\"public\",\"table\":\"wave_data\"}]},\"access_token\":\"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InptcmFrZmZrbHNtYW91YWprbnp5Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3MDY1NjcwMDgsImV4cCI6MjAyMjE0MzAwOH0.1NQvhDko1YI-441l2mK2CiTtXdo3WGxbu6Rfg7FYzVM\"}");

        sendPayload("realtime:test_1", "access_token", "{\"access_token\":\"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InptcmFrZmZrbHNtYW91YWprbnp5Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3MDY1NjcwMDgsImV4cCI6MjAyMjE0MzAwOH0.1NQvhDko1YI-441l2mK2CiTtXdo3WGxbu6Rfg7FYzVM\"}");
    }
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        Serial.println("Connnection Closed");
    }
    else if (event == WebsocketsEvent::GotPing)
    {
        Serial.println("Got a Ping!");
    }
    else if (event == WebsocketsEvent::GotPong)
    {
        Serial.println("Got a Pong!");
    }
}

void setupWave(String function, int frequency, float amplitude, float offset, float phase, int replicate, String pars, int frequency_value, int frequency_range, String command, int maxsamp)
{
    wave["func"] = function;
    wave["frequency"] = frequency;
    wave["amplitude"] = amplitude;
    wave["offset"] = offset;
    wave["phase"] = phase;
    wave["replicate"] = replicate;
    wave["pars"] = pars;
    wave["frequency_value"] = frequency_value;
    wave["command"] = command;
    wave["maxsamp"] = maxsamp;
}

void setupSine()
{
    int freq = atoi(frequency);
    float ampl = atof(amplitude);
    float amp = 0;

    if (ampl > 3)
    {
        amp = 0.94;
        float pot_aux = (ampl / 3.0) - 1;
        int pot_value =  (10000 - (pot_aux * 3000)) / 100;
        pot.set(pot_value);
    }
    else
    {
        pot.set(100);
        amp = (0.94 * ampl) / 3;
    }
    //1 + Rf/R2
    //R2 = 3k
    //100/3k > 10k/3k
    //1.033 / 4.33
    //
    //Rf/3k = 0.33
    float offs = amp / 2;

    setupWave("sine", freq, offs, offs, 0, 1, "[0.2, 0.4, 0.2]", freq, 1, "setup", 512);
    sendWave();
}

void setupSquare()
{
    int freq = atoi(frequency);
    float ampl = atof(amplitude);
    float amp = 0;

    if (ampl > 3)
    {
        amp = 0.9;
        float pot_aux = (ampl / 3.0) - 1;
        int pot_value =  (10000 - (pot_aux * 3000)) / 100;
        pot.set(pot_value);
    }
    else
    {
        pot.set(100);
        amp = (0.9 * ampl) / 3;
    }

    float offs = amp / 1;
    setupWave("pulse1", freq, offs, 0, 0, 1, "[0.1, 0.5, 0.1]", freq, 1, "setup", 512);
    sendWave();
}

void setupSaw()
{
    int freq = atoi(frequency);
    float ampl = atof(amplitude);
    float amp = 0;

    if (ampl > 3)
    {
        amp = 0.9;
        float pot_aux = (ampl / 3.0) - 1;
        int pot_value =  (10000 - (pot_aux * 3000)) / 100;
        pot.set(pot_value);
    }
    else
    {
        pot.set(100);
        amp = (0.9 * ampl) / 3;
    }

    float offs = amp / 1;
    Serial.println(offs);
    setupWave("pulse2", freq, offs, 0, 0, 1, "[1, 0, 0]", freq, 1, "setup", 512);
    sendWave();
}

void sendWave()
{
    if (con_status == 1)
    {
        char buffer[1024];
        size_t n = serializeJson(wave, buffer);
        SerialPi.println(buffer);
        con_status = 2;
    }
}

void setup()
{
    Serial.begin(9600);
    SerialPi.begin(9600);
    // Connect to wifi
    WiFi.begin(ssid, password);

    // Wait some time to connect to wifi
    for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++)
    {
        Serial.print(".");
        delay(1000);
    }

    // run callback when messages are received
    client.onMessage(onMessageCallback);

    // run callback when events are occuring
    client.onEvent(onEventsCallback);

    // Before connecting, set the ssl fingerprint of the server
    // client.setFingerprint(echo_org_ssl_fingerprin);
    client.setInsecure();
    // Connect to server
    client.connect(websocketURL);
    setupWave("none", 2000, 0.48, 0.5, 0, 1, "[0.2, 0.4, 0.2]", 2000, 1, "stop", 512);
    pot.reset();
    // sendPayload("phoenix", "hearbeat", "{}");
}

int incres = 1;
int direction = 0;
void loop()
{

    client.poll();

    unsigned long currentMillis = millis();
    unsigned long currentMillisPiserial = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        sendPayload("phoenix", "heartbeat", "{}");
    }

    if (currentMillisPiserial - previousMillis_Piserial >= interval_Piserial)
    {
        Serial.println(pot.get());
        previousMillis_Piserial = currentMillisPiserial;
        if (con_status == 2)
        {
            con_status = 1;
        }
    }

    if (con_status == 0)
    {
        SerialPi.write(0x3D);
        SerialPi.write(0x0D);
        Serial.print("try...");
    }

    if (SerialPi.available())
    {
        String receiveString = SerialPi.readString();
        if (con_status == 0)
        {
            JsonDocument dataPi;
            deserializeJson(dataPi, receiveString);
            const char *connection_pi = dataPi["connection"];
            if (strcmp(connection_pi, "READY") == 0)
            {
                con_status = 1;
            }
        }

        Serial.println(receiveString);
    }
}
