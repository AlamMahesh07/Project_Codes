#include <WiFi.h>
#include <HTTPClient.h>
#include <DFRobotDFPlayerMini.h>

const char* ssid = "OnePlus Nord CE4";
const char* password = "cheppanuraa";

// Firebase URL
const char* firebaseHost = "https://alexa-27be5-default-rtdb.asia-southeast1.firebasedatabase.app";

// DFPlayer Serial
HardwareSerial mySerial(2);
DFRobotDFPlayerMini dfPlayer;

// Config
unsigned long fetchInterval = 5000;
unsigned long lastFetchTime = 0;
int lastSong = -1;
bool isPaused = false;
const int maxSongs = 255;

void setup() {
  Serial.begin(115200);

  mySerial.begin(9600, SERIAL_8N1, 16, 17);

  if (!dfPlayer.begin(mySerial)) {
    Serial.println("? DFPlayer not responding.");
    while (true);
  }

  dfPlayer.volume(25);
  dfPlayer.play(1);  // Play root song
  lastSong = 1;
  Serial.println("?? Startup: Playing 001.mp3 from root");

  WiFi.begin(ssid, password);
  Serial.print("?? Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n? WiFi connected.");
}

void loop() {
  if (millis() - lastFetchTime > fetchInterval) {
    lastFetchTime = millis();
    fetchAndPlaySong();
  }
}

void fetchAndPlaySong() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("? WiFi not connected.");
    return;
  }

  HTTPClient http;
  String url = String(firebaseHost) + "/gesture/song.json";
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    int command = payload.toInt();

    if (command <= 0 || command == lastSong) return;

    Serial.print("?? Firebase song command: ");
    Serial.println(command);

    switch (command) {
      case 9:  // Pause
        if (!isPaused) {
          dfPlayer.pause();
          isPaused = true;
          Serial.println("? Paused");
        }
        break;

      case 7:  // Resume
        if (isPaused) {
          dfPlayer.start();
          isPaused = false;
          Serial.println("? Resumed");
        }
        break;

      case 2:  // Next song
        lastSong++;
        if (lastSong > maxSongs) lastSong = 1;
        dfPlayer.playMp3Folder(lastSong);
        isPaused = false;
        Serial.print("? Next: Playing MP3/");
        Serial.println(lastSong);
        break;

      case 14:  // Previous song
        lastSong--;
        if (lastSong < 1) lastSong = maxSongs;
        dfPlayer.playMp3Folder(lastSong);
        isPaused = false;
        Serial.print("? Previous: Playing MP3/");
        Serial.println(lastSong);
        break;

      default:
        if (command >= 1 && command <= maxSongs) {
          lastSong = command;
          dfPlayer.playMp3Folder(lastSong);
          isPaused = false;
          Serial.print("?? Playing specific MP3/");
          Serial.println(lastSong);
        } else {
          Serial.println("? Invalid song number.");
        }
        break;
    }
  } else {
    Serial.print("? Firebase fetch error: ");
    Serial.println(httpCode);
  }

  http.end();
}