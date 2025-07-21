#include "src/OV2640.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <SD_MMC.h> // For SD card functionality
#include "EEPROM.h"

// Select camera model
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Wi-Fi credentials
#define SSID1 "Islington College"
#define PWD1 "I$LiNGT0N2025"

// Pin definitions
#define MOTION_TRIGGER_PIN 3 // Pin receiving signal from Arduino
#define STATUS_LED 4 // Built-in LED
#define DEBOUNCE_DELAY 50 // Debounce time in milliseconds

// EEPROM configuration
#define EEPROM_SIZE 1
#define COUNTER_ADDRESS 0

OV2640 cam;
WebServer server(80);

// Global variables
int imageCounter = 0;
bool lastPinState = HIGH;
unsigned long lastDebounceTime = 0;
bool motionCaptured = false;

// HTTP Headers for MJPEG streaming
const char HEADER[] = "HTTP/1.1 200 OK\r\n" \
                      "Access-Control-Allow-Origin: *\r\n" \
                      "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);

//
// Function to initialize SD card
void initSDCard() {
    if (!SD_MMC.begin()) {
        Serial.println("SD Card Mount Failed");
        // Indicate error with LED
        for (int i = 0; i < 5; i++) {
            digitalWrite(STATUS_LED, HIGH);
            delay(100);
            digitalWrite(STATUS_LED, LOW);
            delay(100);
        }
    return;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD Card attached");
        return;
    }

    Serial.println("SD Card Mounted Successfully");
}

//
// Function to capture and save image
void captureAndSaveImage() {
    digitalWrite(STATUS_LED, HIGH); // Turn on LED
    camera_fb_t* fb = esp_camera_fb_get();

    if (!fb) {
        Serial.println("Camera capture failed");
        digitalWrite(STATUS_LED, LOW);
        return;
    }

    char filename[32];
    sprintf(filename, "/MD_%d.jpg", imageCounter);

    fs::FS &fs = SD_MMC;
    File file = fs.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        esp_camera_fb_return(fb);
        digitalWrite(STATUS_LED, LOW);
        return;
    }

    size_t written = file.write(fb->buf, fb->len);

    if (written != fb->len) {
        Serial.println("Write failed");
    } else {
        Serial.printf("Saved image: %s, Size: %u bytes\n", filename, fb->len);
        imageCounter++;
        EEPROM.write(COUNTER_ADDRESS, imageCounter);
        EEPROM.commit();
    }
    file.close();
    esp_camera_fb_return(fb);
    digitalWrite(STATUS_LED, LOW); // Turn off LED
}

//
//handle MJPEG streaming
void handle_jpg_stream() {
    WiFiClient client = server.client();
    client.write(HEADER, hdrLen);
    client.write(BOUNDARY, bdrLen);

    unsigned long lastFrameTime = 0;
    const int frameDelay = 100; // Delay between frames in milliseconds

    while (client.connected()) {
        unsigned long currentTime = millis();
        if (currentTime - lastFrameTime >= frameDelay) {
            lastFrameTime = currentTime;

            cam.run();
            int s = cam.getSize();

            client.write(CTNTTYPE, cntLen);
            char buf[32];
            sprintf(buf, "%d\r\n\r\n", s);
            client.write(buf, strlen(buf));
            client.write((char *)cam.getfb(), s);
            client.write(BOUNDARY, bdrLen);
        }

        // Process other server requests
        server.handleClient();
    }
}

// Handle button actions
void handleAction() {
    String command = server.arg("cmd");

    if (command == "action1") {
        Serial.println("up");
    } else if (command == "action2") {
        Serial.println("down");
    } else if (command == "action3") {
        Serial.println("left");
    } else if (command == "action4") {
        Serial.println("right");
    } else {
        Serial.println("Unknown command");
    }

    server.send(200, "text/plain", "Command executed: " + command);
}


// Handel web page
void handle_mjpeg_page() {
    String html = R"rawliteral(
        <html>
        <head>
            <title>SMART EYE SYSTEM</title>
            <!-- Retro Pixel Font -->
            <link href="https://fonts.googleapis.com/css2?family=Press+Start+2P&display=swap" rel="stylesheet">
            <style>
                /* Retro pixel background animation */
                body {
                margin: 0;
                padding: 0;
                font-family: 'Press Start 2P', cursive;
                background: #141414 url('https://i.imgur.com/3Z4FZLT.png') repeat;
                overflow-x: hidden;
                color: #e0e0e0;
                text-align: center;
                }

                h1 {
                margin: 20px 0;
                font-size: 1.5rem;
                color: #ffcc00;
                text-shadow: 2px 2px #000;
                }

                .video-container {
                margin: 20px auto;
                width: 90%;
                max-width: 640px;
                background: #000;
                border: 4px solid #ffcc00;
                box-shadow: 0 0 20px #ffcc00;
                }

                .video-container img {
                display: block;
                width: 100%;
                height: auto;
                }

                .controls {
                display: grid;
                grid-template-areas:
                    ". up ."
                    "left . right"
                    ". down .";
                grid-gap: 12px;
                width: 300px;
                margin: 30px auto;
                }

                .controls button {
                grid-area: unset;
                width: 100px;
                height: 40px;
                font-size: 0.7rem;
                border: 3px solid #ffcc00;
                background: #222;
                color: #ffcc00;
                cursor: pointer;
                transition: background 0.2s, transform 0.2s;
                }

                button.up    { grid-area: up; }
                button.down  { grid-area: down; }
                button.left  { grid-area: left; }
                button.right { grid-area: right; }

                .controls button:hover {
                background: #ffcc00;
                color: #000;
                transform: scale(1.1);
                box-shadow: 0 0 10px #ffcc00;
                }

                /* Footer pixel bar */
                footer {
                margin-top: 40px;
                padding: 10px 0;
                background: #000;
                border-top: 2px solid #ffcc00;
                font-size: 0.6rem;
                }
            </style>
        </head>
        <body>
            <h1>SMART EYE SYSTEM</h1>
            <div class="video-container">
                <img src="/mjpeg/1" alt="Live Stream">
            </div>
            <div class="controls">
                <button class="up"    onclick="sendCommand('action1')">UP</button>
                <button class="left"  onclick="sendCommand('action3')">LEFT</button>
                <button class="right" onclick="sendCommand('action4')">RIGHT</button>
                <button class="down"  onclick="sendCommand('action2')">DOWN</button>
            </div>

            <script>
                function sendCommand(cmd) {
                fetch('/action?cmd=' + cmd)
                    .then(res => res.text())
                    .then(console.log)
                    .catch(console.error);
                }

                document.addEventListener('keydown', e => {
                switch(e.key) {
                    case 'ArrowUp': sendCommand('action1'); break;
                    case 'ArrowDown': sendCommand('action2'); break;
                    case 'ArrowLeft': sendCommand('action3'); break;
                    case 'ArrowRight': sendCommand('action4'); break;
                    default: return;
                }
                e.preventDefault();
                });
            </script>

            <footer>Powered by SMART EYE • Retro Mode Engaged</footer>
        </body>
        </html>
    )rawliteral";

    server.send(200, "text/html", html);
}

void setup() {
    Serial.begin(115200);

    // Initialize LED and motion trigger pin
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);
    pinMode(MOTION_TRIGGER_PIN, INPUT_PULLUP);

    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    imageCounter = EEPROM.read(COUNTER_ADDRESS);
    initSDCard();

    // Camera configuration
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;

    cam.init(config);

    // SD card initialization
    if (!SD_MMC.begin()) {
        Serial.println("SD Card Mount Failed");
        return;
    }

    // Wi-Fi connection
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID1, PWD1);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    IPAddress ip = WiFi.localIP();
    Serial.println(ip);
    Serial.print("Stream Link: http://");
    Serial.print(ip);
    Serial.println("/");

    // Server routes
    server.on("/", HTTP_GET, handle_mjpeg_page);
    server.on("/mjpeg/1", HTTP_GET, handle_jpg_stream);
    server.on("/action", HTTP_GET, handleAction);

    server.begin();

    // Take test image
    Serial.println("Taking test image...");
    captureAndSaveImage();
}

void loop() {
    bool currentPinState = digitalRead(MOTION_TRIGGER_PIN);
    unsigned long currentMillis = millis();

    // Check for debounce
    if (currentPinState == LOW && (currentMillis - lastDebounceTime) > DEBOUNCE_DELAY) {
        lastDebounceTime = currentMillis;

        if (!motionCaptured) {
            Serial.println("Motion trigger received - Capturing image");
            captureAndSaveImage();
            motionCaptured = true; // Set flag to prevent repeated captures
        }
    }

    if (currentPinState == HIGH) {
        motionCaptured = false; // Reset flag when motion stops
    }

    server.handleClient();

}
