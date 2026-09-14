#pragma once

#include <Arduino.h>

#include "config.h"
#include "DeviceState.h"
#include "MessageManager.h"
#include "NotificationManager.h"

// ============================================================================
// DESKBOT BLE MANAGER
// ============================================================================
//
// BLE transport layer for DeskBot.
//
// Packet format:
//   Offset  Size  Field
//   0       1     version
//   1       1     type
//   2       4     message_id (uint32 LE)
//   6       1     sequence
//   7       1     total
//   8       2     payload_length (uint16 LE)
//   10      N     payload
//   10+N    2     CRC16 (CCITT-FALSE, LE)
//
// Supported packet types:
//   1 = MESSAGE
//   2 = ACK
//   3 = COMMAND
//   4 = PING
//   5 = STATUS_UPDATE
//
// MESSAGE and COMMAND packets support fragmentation.
//
// IMPORTANT:
// This implementation is compatible with ESP32 Arduino Core 3.x,
// including Core 3.3.11, where BLECharacteristic::getValue()
// returns Arduino String.
//

class BLEManager {
 public:
  // Initialize BLE and connect the manager to the other DeskBot managers.
  void begin(MessageManager* messageManager,
             NotificationManager* notifier);

  // Call once per loop().
  // Updates BLE connection state and sends periodic status updates.
  void update(DeviceState &state);

  // Returns true when a phone/device is currently connected.
  bool isConnected() const;

 private:
  MessageManager* _messages = nullptr;
  NotificationManager* _notifier = nullptr;

  bool _wasConnected = false;
};