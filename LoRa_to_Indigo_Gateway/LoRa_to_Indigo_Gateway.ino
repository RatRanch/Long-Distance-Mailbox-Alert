/*
    This sketch was adapted from work by
    Parham Alvani and Sajjad Rahnama, 2018-01-07.

    Changed the call to http.begin to use a nonstandard port
    and added a delay between first and second connection
    attempts to avoid a race condition.

    See https://wiki.indigodomo.com/doku.php?id=indigo_s_restful_urls
*/

/*
 * This sketch waits for serial input from the LoRa receiver and
 * updates the Indigo variables mailboxOpen and mailboxBattVoltage 
 */

#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>


// Replace with your WiFi ID and password
#ifndef STASSID
#define STASSID "YOUR WIFI SSID"
#define STAPSK  "WIFI PASSWORD"
#endif

const char* ssid = STASSID;
const char* ssidPassword = STAPSK;

// Replace with your Indigo credentials and server IP
const char *username = "username";
const char *password = "password";
const char *server = "192.168.0.10";

String uri = "";


String exractParam(String& authReq, const String& param, const char delimit) {
  int _begin = authReq.indexOf(param);
  if (_begin == -1) {
    return "";
  }
  return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}

String getCNonce(const int len) {
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  String s = "";

  for (int i = 0; i < len; ++i) {
    s += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return s;
}

String getDigestAuth(String& authReq, const String& username, const String& password, const String& uri, unsigned int counter) {
  // extracting required parameters for RFC 2069 simpler Digest
  String realm = exractParam(authReq, "realm=\"", '"');
  String nonce = exractParam(authReq, "nonce=\"", '"');
  String cNonce = getCNonce(8);

  char nc[9];
  snprintf(nc, sizeof(nc), "%08x", counter);

  // parameters for the RFC 2617 newer Digest
  MD5Builder md5;
  md5.begin();
  md5.add(username + ":" + realm + ":" + password);  // md5 of the user:realm:user
  md5.calculate();
  String h1 = md5.toString();

  md5.begin();
  md5.add(String("GET:") + uri);
  md5.calculate();
  String h2 = md5.toString();

  md5.begin();
  md5.add(h1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":" + "auth" + ":" + h2);
  md5.calculate();
  String response = md5.toString();

  String authorization = "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce +
                         "\", uri=\"" + uri + "\", algorithm=\"MD5\", qop=auth, nc=" + String(nc) + ", cnonce=\"" + cNonce + "\", response=\"" + response + "\"";
  Serial.println(authorization);

  return authorization;
}
  String inputString = "";         // a String to hold incoming data
  bool stringComplete = false;  // whether the string is complete
  String mailboxOpen = "false";

  
  void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, ssidPassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // reserve 200 bytes for the inputString and uri:
  inputString.reserve(200);

  uri.reserve(200);
}

void loop() {
  checkSerial();
  // print the string when a newline arrives:
  if (stringComplete) {
    if (inputString.substring(0, 1) == "1"){ 
      mailboxOpen = "true";
      uri = "/variables/mailboxOpen?_method=put&value=true";
    }
    else{
      mailboxOpen = "false";
      uri = "/variables/mailboxOpen?_method=put&value=false";
    }

    // Send update to Indigo
    updateIndigo();
    
    String battVoltage = inputString.substring(2, 6); //from position 2 to end of string
    uri = "/variables/mailboxBattVoltage?_method=put&value=" + battVoltage;
    // Send update to Indigo
    updateIndigo();

    Serial.print("Received: "); Serial.print(inputString);
    Serial.print("Open: "); Serial.print(mailboxOpen); Serial.print("   Battery Voltage: "); Serial.println(battVoltage);
    Serial.println();


    // clear the string:
    inputString = "";
    stringComplete = false; 
  } 
}

/*
  Check the serial port to see if the LoRa module sent anything
*/
void checkSerial() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    //Serial.print(inChar);
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void updateIndigo() {
  WiFiClient client;
  HTTPClient http; //must be declared after WiFiClient for correct destruction order, because used by http.begin(client,...)

  Serial.print("[HTTP] begin...\n");

  const int httpPort = 8176;

  http.begin(String(server), httpPort, String(uri)); //RAT

  const char *keys[] = {"WWW-Authenticate"};
  http.collectHeaders(keys, 1);

  Serial.print("[HTTP] GET...\n");
  int httpCode = http.GET();
  String payload = http.getString();
  //Serial.println(payload);
  
  if (httpCode > 0) {
    Serial.println("The first get request succeeded");
    String authReq = http.header("WWW-Authenticate");
    Serial.print("Auth Req:  ");Serial.println(authReq);

    String authorization = getDigestAuth(authReq, String(username), String(password), String(uri), 1);

    http.end();
    
    // This was an issue with the sample code if we connected to a local server
    // Need to add a delay or it doesn't have enough time to close the first connection
    delay(100);
    
    http.begin(String(server), httpPort, String(uri)); //RAT
    http.addHeader("Authorization", authorization);

    Serial.println("We added the Authorization header");

    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  delay(100);
}
