#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLE2902.h>

BLEScan* pBLEScan;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.println();
        // Extract and print device name
        std::string deviceName = advertisedDevice.getName();
        if (!deviceName.empty()) {
            Serial.print("Device Name: ");
            Serial.println(deviceName.c_str());
        } else {
            Serial.println("Device Name: N/A");
        }

        Serial.print("Device found: ");
        Serial.println(advertisedDevice.getAddress().toString().c_str());

        // Extract and print manufacturer data
        if (advertisedDevice.haveManufacturerData()) {
            uint16_t manufacturerId = advertisedDevice.getManufacturerData()[0] << 8 | advertisedDevice.getManufacturerData()[1];
            const char* manufacturerData = &advertisedDevice.getManufacturerData()[2];
            int manufacturerDataSize = advertisedDevice.getManufacturerData().size() - 2;

            Serial.print("Manufacturer ID: 0x");
            Serial.println(manufacturerId, HEX);

            Serial.print("Manufacturer Data: ");
            for (int i = 0; i < manufacturerDataSize; i++) {
                Serial.print((uint8_t)manufacturerData[i], HEX); // Cast to uint8_t to print as byte
                Serial.print(" ");
            }
            Serial.println();
        } else {
            Serial.println("Manufacturer Data: N/A");
        }

        Serial.print("Raw Payload: ");
        for (int i = 0; i < advertisedDevice.getPayloadLength(); i++) {
            Serial.print(advertisedDevice.getPayload()[i], HEX);
            Serial.print(" ");
        }
        Serial.println();

        // Print insights about the device
        printDeviceInsights(advertisedDevice);
    }

    void printDeviceInsights(BLEAdvertisedDevice advertisedDevice) {
        Serial.println("Device Insights:");
        for (int i = 0; i < advertisedDevice.getPayloadLength() - 2; i++) {
            if (advertisedDevice.getPayload()[i] == 0x02 || advertisedDevice.getPayload()[i] == 0x03) {
                uint16_t serviceUUID = (advertisedDevice.getPayload()[i + 1] << 8) | advertisedDevice.getPayload()[i];
                Serial.print("Service UUID: 0x");
                Serial.println(serviceUUID, HEX);
                printServiceDetails(serviceUUID);
            }
        }
        Serial.println("End of insights");
    }

    void printServiceDetails(uint16_t serviceUUID) {
        // You can add more service UUIDs and details here
        if (serviceUUID == 0x180F) {
            Serial.println("Heart Rate Service");
        } else if (serviceUUID == 0x180A) {
            Serial.println("Device Information Service");
        }
        // Add more conditions for other service UUIDs...
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE scan...");

    BLEDevice::init("BLE_SNIFFER");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

    pBLEScan->setActiveScan(true);
    pBLEScan->start(0, nullptr, false);
}

void loop() {
    // Scanning is handled by the BLE stack
}
