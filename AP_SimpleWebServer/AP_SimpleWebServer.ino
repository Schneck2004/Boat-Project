/*
  WiFi Web Server Brushless Motor Control (ESC)

  A simple web server that controls a Brushless Motor via an ESC.
  This sketch creates an access point and launches a web server.
  
  Wiring Instructions:
  - Connect the ESC's signal wire (usually white or yellow) to pin 9.
  - Connect the ESC's BEC 5V wire (usually red) to the Arduino's 5V pin.
  - Connect the ESC's BEC GND wire (usually black or brown) to the Arduino's GND pin.
  - **WARNING**: If you are powering the Arduino via USB while testing, it is 
    recommended to disconnect the ESC's BEC 5V power wire to avoid conflicting power sources.
*/

#include "WiFiS3.h"
#include <Servo.h>
#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password
int keyIndex = 0;

int led = LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

void printWiFiStatus();

// ESC Setup
Servo esc;
#define escPin 9 // Pin name with macro to reduce file size and load
int currentThrottle = 1500; // 1500us is neutral (stopped) for most car/boat ESCs

// Rudder Setup
Servo rudder;
#define rudderPin 10 // Pin name with macro to reduce file size and load
int currentAngle = 0;
#define MAX_LEFT -45
#define MAX_RIGHT 45

// Simulated Telemetry (Can be replaced with actual sensor reads)
float batteryVoltage = 12.6; 
int engineRPM = 0;

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect
  }
  Serial.println("Access Point Web Server for ESC");

  pinMode(led, OUTPUT);

  // Attach ESC
  esc.attach(escPin);
  esc.writeMicroseconds(currentThrottle); // Send neutral signal to initialize/arm ESC

  // Attach Rudder
  rudder.attach(rudderPin);
  rudder.write(currentAngle); // Send neutral signal to center rudder

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  WiFi.config(IPAddress(192, 48, 56, 2));

  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();
}

// Modern Glassmorphism Dashboard UI
const char *index_html=
  #include "html.h"
;


// Parses the incoming HTTP request and responds accordingly
void processHttpRequest(WiFiClient& client, String req) {
  
  // Throttle API
  if (req.startsWith("GET /api/throttle")) {
    int vIndex = req.indexOf("v=");
    if (vIndex != -1) {
      int endIndex = req.indexOf(" ", vIndex);
      if (endIndex == -1) endIndex = req.length();
      
      String valStr = req.substring(vIndex + 2, endIndex);
      int val = valStr.toInt();
      // Serial.print(val);
      // Serial.print(" ");
      // Serial.println(valStr);
      
      // Strict constraint for servo microsecond range 1000-2000
      if (val >= 1000 && val <= 2000) {
        currentThrottle = val;
        esc.writeMicroseconds(currentThrottle);
        // Serial.println(currentThrottle);
      }
    }
  }

  //Rudder API
  if (req.startsWith("GET /api/rudangle")) {
    int vIndex = req.indexOf("v=");
    if (vIndex != -1) {
      int endIndex = req.indexOf(" ", vIndex);
      if (endIndex == -1) endIndex = req.length();

      String valStr = req.substring(vIndex + 2, endIndex);
      int val = valStr.toInt();
      // Serial.print(valStr);
      // Serial.print(" ");
      // Serial.println(val);

      // Strict constraint for rudder ange range -45 to 45
      if (val >= -45 && val <= 45) {
        currentAngle = val;
        rudder.write(currentAngle);
        // Serial.println(currentAngle);
      }
    }
  }

  // Calculate speed percentage for UI
  int speedPct = 0;
  if(currentThrottle >= 1500) {
    speedPct = map(currentThrottle, 1500, 2000, 0, 100);
  } else {
    speedPct = -map(currentThrottle, 1500, 1000, 0, 100);
  }

  // Send JSON response for both telemetry, throttle & rudder angle update
  if(req.startsWith("GET /api/")) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type: application/json");
      client.println("Connection: close");
      client.println("Access-Control-Allow-Origin: *"); // Useful for debugging from local file
      client.println();
      
      client.print("{\"voltage\":");
      client.print(batteryVoltage);
      client.print(",\"speed_pct\":");
      client.print(speedPct);
      client.print(",\"throttle\":");
      client.print(currentThrottle);
      client.print(",\"rudangle\":");
      client.print(currentAngle);
      client.println("}");
  } 
  else {
      // Default: Serve the HTML page
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type: text/html");
      client.println("Connection: close");
      client.println();
      client.print(index_html);
  }
}

void loop() {
  if (status != WiFi.status()) {
    status = WiFi.status();
    if (status == WL_AP_CONNECTED) {
      Serial.println("Device connected to AP");
    } else {
      Serial.println("Device disconnected from AP");
      // Safety First: Neutral throttle and full right rudder when device disconnects
      currentThrottle = 1500;
      esc.writeMicroseconds(currentThrottle);
      rudder.write(MAX_RIGHT);
    }
  }

  // Simulated Telemetry logic (e.g., voltage slightly sags as throttle increases)
  int absThrottle = abs(currentThrottle - 1500);
  batteryVoltage = 12.6 - (absThrottle * 0.001); 

  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    String requestLine = "";
    bool requestLineRead = false;

    while (client.connected()) {
      // 10 microsecond delay is required for AP stability on Arduino WiFi module
      delayMicroseconds(10); 
      if (client.available()) {
        char c = client.read();

        // Read the first line (HTTP Request Line)
        if (c == '\n' && currentLine.length() > 0) {
            if(!requestLineRead) {
                requestLine = currentLine;
                requestLineRead = true;
            }
            currentLine = "";
        } 
        else if (c == '\n' && currentLine.length() == 0) {
          // Empty line indicates end of HTTP headers
          processHttpRequest(client, requestLine);
          break;
        } 
        else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    // Close the connection
    client.stop();
  }
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
