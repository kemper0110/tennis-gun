#include <Arduino.h>
#include <NimBLEDevice.h>
#include "ble-control.h"
#include "control-state.h"
#include "json-protocol.h"

namespace {
constexpr char DEVICE_NAME[] = "Tennis Gun";
constexpr char SERVICE_UUID[] = "f3641400-00b0-4240-ba50-05ca45bf8abc";
constexpr char CONTROL_UUID[] = "f3641401-00b0-4240-ba50-05ca45bf8abc";
constexpr char STATUS_UUID[] = "f3641402-00b0-4240-ba50-05ca45bf8abc";
constexpr uint16_t PREFERRED_MTU = 128;
constexpr uint16_t CONTROL_MAX_LENGTH = 128;
constexpr uint16_t STATUS_MAX_LENGTH = 256;
constexpr uint16_t SUPERVISION_TIMEOUT = 200;

NimBLECharacteristic* statusCharacteristic = nullptr;

void refreshStatus() {
  const String status = getStatusJson();
  statusCharacteristic->setValue(status.c_str());
  Serial.println(status);
}

class ControlCallbacks final : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo& connInfo) override {
    (void)connInfo;
    const NimBLEAttValue& value = characteristic->getValue();
    handleControlJson(value.data(), value.size());
    refreshStatus();
  }
};

class StatusCallbacks final : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic* characteristic, NimBLEConnInfo& connInfo) override {
    (void)characteristic;
    (void)connInfo;
    refreshStatus();
  }
};

class ServerCallbacks final : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
    server->updateConnParams(connInfo.getConnHandle(), 12, 24, 0, SUPERVISION_TIMEOUT);
    refreshStatus();
    Serial.println("BLE client connected");
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
    (void)server;
    (void)connInfo;
    stopForDisconnect();
    refreshStatus();
    Serial.print("BLE client disconnected, reason: ");
    Serial.println(reason);
  }
};

ControlCallbacks controlCallbacks;
StatusCallbacks statusCallbacks;
ServerCallbacks serverCallbacks;
}  // namespace

void setupBleControl() {
  NimBLEDevice::init(DEVICE_NAME);
  if (!NimBLEDevice::setMTU(PREFERRED_MTU)) {
    Serial.println("Failed to set preferred BLE MTU");
  }

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks, false);
  server->advertiseOnDisconnect(true);

  NimBLEService* service = server->createService(SERVICE_UUID);
  NimBLECharacteristic* controlCharacteristic = service->createCharacteristic(
      CONTROL_UUID, NIMBLE_PROPERTY::WRITE, CONTROL_MAX_LENGTH);
  statusCharacteristic = service->createCharacteristic(
      STATUS_UUID, NIMBLE_PROPERTY::READ, STATUS_MAX_LENGTH);
  controlCharacteristic->setCallbacks(&controlCallbacks);
  statusCharacteristic->setCallbacks(&statusCallbacks);
  refreshStatus();

  server->start();
  NimBLEAdvertising* advertising = server->getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->enableScanResponse(true);
  advertising->start();
  Serial.println("BLE control service started");
}
