#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
#define BUZZER_PIN     26

Adafruit_MPU6050 mpu;

// Global variables for sensor data
float accelerationMagnitude = 0.0;
float gyroMagnitude = 0.0;
float temperature = 0.0;
bool locked      = false; 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// BLE settings
BLEServer* pServer;
BLECharacteristic *pCharacteristicSensor;
BLECharacteristic *pCharacteristicCommand;
BLEService* pService;
bool bleConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        bleConnected = true;
    }
    void onDisconnect(BLEServer* pServer) {
        bleConnected = false;
    }
};

void setupOLED() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;) {
            // Loop indefinitely if OLED display initialization fails
        }
    }
    display.clearDisplay(); // Clear the display
    display.display();
    delay(500); // Pause for 2 seconds
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setRotation(0);
}

void setupBLE() {
    BLEDevice::init("Secure_Guard_BLE");
    pServer = BLEDevice::createServer();
    pService = pServer->createService(BLEUUID("6e400001-b5a3-f393-e0a9-e50e224dcca9"));
    pCharacteristicSensor = pService->createCharacteristic(
        BLEUUID("180D"),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    pCharacteristicCommand = pService->createCharacteristic(
        BLEUUID("180C"),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );

    pCharacteristicSensor->addDescriptor(new BLE2902());
    pService->start();
    BLEAdvertising* pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
    pServer->setCallbacks(new MyServerCallbacks());

    pCharacteristicCommand->setValue("Unlock"); // Set an initial command
}

void setupMPU6050() {
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1);
    }
}

void displayUI() {
    // Display data on OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(F("BLE: "));
    display.println(bleConnected ? "Connected" : "Disconnected");

    // Display MPU6050 data
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    temperature = temp.temperature;
    // Calculate the total acceleration (vector magnitude)
    accelerationMagnitude = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));

    // Calculate the total gyro (vector magnitude)
    gyroMagnitude = sqrt(pow(g.gyro.x, 2) + pow(g.gyro.y, 2) + pow(g.gyro.z, 2));

    display.setCursor(0, 10);
    display.print(F("Accel: "));
    display.print(accelerationMagnitude);
    display.setCursor(0, 20);
    display.print(F("Gyro: "));
    display.print(gyroMagnitude);
    display.setCursor(0, 30);
    display.print(F("Temp: "));
    display.print(temperature);
    display.setCursor(0, 40);
    display.print(F("Lock Status: "));
    display.println(locked ? "Locked" : "Unlocked");
    display.display();
}

void myTone( int pin)
{
  ledcAttachPin(pin, 0);             // pin, channel
  ledcWriteNote(0, NOTE_F, 4);    // channel, frequency, octave
}

void myNoTone( int pin)
{
  ledcDetachPin(pin);
}


void loop() {
    displayUI();
    
    // Update BLE characteristics if connected
    if (bleConnected) {
        uint8_t data[12];
        int16_t accelInt = static_cast<int16_t>(accelerationMagnitude);
        int16_t gyroInt = static_cast<int16_t>(gyroMagnitude);
        int16_t tempInt = static_cast<int16_t>(temperature);
        memcpy(data, &accelInt, sizeof(accelInt));
        memcpy(data + sizeof(accelInt), &gyroInt, sizeof(gyroInt));
        memcpy(data + sizeof(accelInt) + sizeof(gyroInt), &tempInt, sizeof(tempInt));
        pCharacteristicSensor->setValue(data, sizeof(data)); // Set BLE characteristic value
        pCharacteristicSensor->notify();

        // Handle incoming commands
        std::string receivedCommand = pCharacteristicCommand->getValue();
        // Check the received command
        if (receivedCommand == "Lock") {
        locked = true;
        myTone(BUZZER_PIN); // Sound the buzzer
        delay(1000); // Wait for 1 second
        myNoTone(BUZZER_PIN); // Turn off the buzzer
        } else if (receivedCommand == "Unlock") {
        locked = false;
        myTone(BUZZER_PIN); // Sound the buzzer
        delay(1000); // Wait for 1 second
        myNoTone(BUZZER_PIN); // Turn off the buzzer
        }

    }

    if (locked) {
        if (fabs(accelerationMagnitude) > 2 || fabs(gyroMagnitude) > 1) {

            myTone(BUZZER_PIN); // Sound the buzzer
            delay(1000); // Wait for 1 second
            myNoTone(BUZZER_PIN); // Turn off the buzzer
        }
    }

    if (temperature > 35.0) { // Adjust the threshold as needed
        myTone(BUZZER_PIN); // Sound the buzzer
        delay(1000); // Wait for 1 second
        myNoTone(BUZZER_PIN); // Turn off the buzzer
    }

}

void setup() {
    Serial.begin(115200);

    pinMode(BUZZER_PIN, OUTPUT);
    myNoTone(BUZZER_PIN); // Ensure the buzzer is off initially
    
    setupOLED();
    setupBLE();
    setupMPU6050();
    Serial.println("MPU6050 found!");
}
