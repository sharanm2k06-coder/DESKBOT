#include "BLEManager.h"
#include "Logger.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// ============================================================================
// BLE PROTOCOL CONSTANTS
// ============================================================================

namespace {

constexpr uint8_t HEADER_SIZE = 10;
constexpr uint8_t CRC_SIZE = 2;

// BLE_MAX_SINGLE_PACKET_LEN is 180 in config.h.
//
// 180 total bytes
// - 10 byte header
// - 2 byte CRC
// = 168 byte payload
//
constexpr uint16_t MAX_FRAGMENT_PAYLOAD =
    BLE_MAX_SINGLE_PACKET_LEN - HEADER_SIZE - CRC_SIZE;

// Maximum number of fragments in one message.
constexpr uint8_t MAX_FRAGMENTS = 8;

// Maximum reassembled message size.
//
// 168 * 8 = 1344 bytes
//
constexpr size_t MAX_MESSAGE_BYTES =
    (size_t)MAX_FRAGMENT_PAYLOAD * MAX_FRAGMENTS;

// ============================================================================
// PACKET TYPES
// ============================================================================

enum class PacketType : uint8_t {
  MESSAGE = 1,
  ACK = 2,
  COMMAND = 3,
  PING = 4,
  STATUS_UPDATE = 5
};

// ============================================================================
// CRC-16/CCITT-FALSE
// ============================================================================
//
// Polynomial : 0x1021
// Initial    : 0xFFFF
// Reflection : none
// Final XOR  : none
//
// Must match the Android BleProtocol.kt implementation.
//

uint16_t crc16(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < length; i++) {
    crc ^= (uint16_t)data[i] << 8;

    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x8000) {
        crc = (uint16_t)((crc << 1) ^ 0x1021);
      } else {
        crc = (uint16_t)(crc << 1);
      }
    }
  }

  return crc;
}

// ============================================================================
// PARSED PACKET HEADER
// ============================================================================

struct ParsedHeader {
  uint8_t version;
  PacketType type;

  uint32_t messageId;

  uint8_t sequence;
  uint8_t total;

  uint16_t payloadLength;

  const uint8_t* payload;
};

// ============================================================================
// PARSE BLE FRAGMENT
// ============================================================================
//
// Returns:
//   true  = valid packet
//   false = malformed / CRC failure / unsupported packet
//
// The supplied buffer is NOT copied here.
// payload points into the original BLE buffer.
//

bool parseFragment(const uint8_t* buf,
                   size_t len,
                   ParsedHeader& out) {

  // Minimum packet:
  //
  // 10 byte header + 2 byte CRC
  //
  if (buf == nullptr) {
    return false;
  }

  if (len < HEADER_SIZE + CRC_SIZE) {
    return false;
  }

  // --------------------------------------------------------------------------
  // Version
  // --------------------------------------------------------------------------

  if (buf[0] != 1) {
    LOGF(
        "[BLEManager] unsupported packet version %u, dropping\n",
        (unsigned int)buf[0]);

    return false;
  }

  // --------------------------------------------------------------------------
  // Payload length
  // --------------------------------------------------------------------------

  uint16_t payloadLength =
      (uint16_t)buf[8] |
      ((uint16_t)buf[9] << 8);

  size_t expectedLen =
      (size_t)HEADER_SIZE +
      payloadLength +
      CRC_SIZE;

  if (len != expectedLen) {
    LOGF(
        "[BLEManager] invalid packet length: got=%u expected=%u\n",
        (unsigned int)len,
        (unsigned int)expectedLen);

    return false;
  }

  // --------------------------------------------------------------------------
  // Maximum fragment payload
  // --------------------------------------------------------------------------

  if (payloadLength > MAX_FRAGMENT_PAYLOAD) {
    LOGLN(
        "[BLEManager] fragment payload too large, dropping");

    return false;
  }

  // --------------------------------------------------------------------------
  // CRC
  // --------------------------------------------------------------------------

  uint16_t receivedCrc =
      (uint16_t)buf[HEADER_SIZE + payloadLength] |
      ((uint16_t)buf[HEADER_SIZE + payloadLength + 1] << 8);

  uint16_t computedCrc =
      crc16(buf, HEADER_SIZE + payloadLength);

  if (receivedCrc != computedCrc) {
    LOGLN(
        "[BLEManager] fragment CRC mismatch, dropping");

    return false;
  }

  // --------------------------------------------------------------------------
  // Packet type
  // --------------------------------------------------------------------------

  uint8_t rawType = buf[1];

  if (rawType < 1 || rawType > 5) {
    LOGF(
        "[BLEManager] invalid packet type %u, dropping\n",
        (unsigned int)rawType);

    return false;
  }

  // --------------------------------------------------------------------------
  // Fill parsed structure
  // --------------------------------------------------------------------------

  out.version = buf[0];

  out.type = (PacketType)rawType;

  out.messageId =
      (uint32_t)buf[2] |
      ((uint32_t)buf[3] << 8) |
      ((uint32_t)buf[4] << 16) |
      ((uint32_t)buf[5] << 24);

  out.sequence = buf[6];

  out.total = buf[7];

  out.payloadLength = payloadLength;

  out.payload = buf + HEADER_SIZE;

  return true;
}

// ============================================================================
// BUILD PACKET
// ============================================================================
//
// Creates a single outgoing packet.
//
// Used for:
//   ACK
//   STATUS_UPDATE
//
// Both are small enough to fit inside one BLE packet.
//

size_t buildPacket(PacketType type,
                   uint32_t messageId,
                   const uint8_t* payload,
                   uint16_t payloadLength,
                   uint8_t* out,
                   size_t outCapacity) {

  if (out == nullptr) {
    return 0;
  }

  if (payloadLength > 0 && payload == nullptr) {
    return 0;
  }

  size_t totalLength =
      (size_t)HEADER_SIZE +
      payloadLength +
      CRC_SIZE;

  if (totalLength > outCapacity) {
    return 0;
  }

  // --------------------------------------------------------------------------
  // Header
  // --------------------------------------------------------------------------

  out[0] = 1; // protocol version

  out[1] = (uint8_t)type;

  // message ID - little endian
  out[2] = (uint8_t)(messageId & 0xFF);
  out[3] = (uint8_t)((messageId >> 8) & 0xFF);
  out[4] = (uint8_t)((messageId >> 16) & 0xFF);
  out[5] = (uint8_t)((messageId >> 24) & 0xFF);

  // Single packet
  out[6] = 0; // sequence
  out[7] = 1; // total fragments

  // payload length - little endian
  out[8] = (uint8_t)(payloadLength & 0xFF);
  out[9] = (uint8_t)((payloadLength >> 8) & 0xFF);

  // --------------------------------------------------------------------------
  // Payload
  // --------------------------------------------------------------------------

  if (payloadLength > 0) {
    memcpy(
        out + HEADER_SIZE,
        payload,
        payloadLength);
  }

  // --------------------------------------------------------------------------
  // CRC
  // --------------------------------------------------------------------------

  uint16_t crc =
      crc16(
          out,
          HEADER_SIZE + payloadLength);

  out[HEADER_SIZE + payloadLength] =
      (uint8_t)(crc & 0xFF);

  out[HEADER_SIZE + payloadLength + 1] =
      (uint8_t)((crc >> 8) & 0xFF);

  return totalLength;
}

// ============================================================================
// FRAGMENT REASSEMBLER
// ============================================================================
//
// DeskBot receives fragments of a single MESSAGE or COMMAND at a time.
//
// Maximum:
//   8 fragments
//   168 bytes each
//   1344 bytes total
//

struct Reassembler {

  bool active = false;

  uint32_t messageId = 0;

  uint8_t totalFragments = 0;

  // One bit per fragment.
  //
  // Supports up to 8 fragments.
  //
  uint8_t receivedMask = 0;

  uint8_t buffer[MAX_MESSAGE_BYTES];

  size_t length = 0;

  // --------------------------------------------------------------------------
  // RESET
  // --------------------------------------------------------------------------

  void reset() {
    active = false;

    messageId = 0;

    totalFragments = 0;

    receivedMask = 0;

    length = 0;
  }

  // --------------------------------------------------------------------------
  // ADD FRAGMENT
  // --------------------------------------------------------------------------
  //
  // Returns true when the entire message has been received.
  //

  bool addFragment(const ParsedHeader& h) {

    // Validate fragment count.

    if (h.total == 0 ||
        h.total > MAX_FRAGMENTS ||
        h.sequence >= h.total) {

      LOGLN(
          "[BLEManager] fragment header out of range, dropping");

      return false;
    }

    // ------------------------------------------------------------------------
    // New message
    // ------------------------------------------------------------------------

    if (!active ||
        h.messageId != messageId) {

      // Discard previous incomplete message.

      reset();

      active = true;

      messageId = h.messageId;

      totalFragments = h.total;
    }

    // ------------------------------------------------------------------------
    // Calculate destination offset
    // ------------------------------------------------------------------------

    size_t offset =
        (size_t)h.sequence *
        MAX_FRAGMENT_PAYLOAD;

    if (offset + h.payloadLength >
        MAX_MESSAGE_BYTES) {

      LOGLN(
          "[BLEManager] reassembled message too large, dropping");

      reset();

      return false;
    }

    // ------------------------------------------------------------------------
    // Copy fragment
    // ------------------------------------------------------------------------

    memcpy(
        buffer + offset,
        h.payload,
        h.payloadLength);

    // ------------------------------------------------------------------------
    // Determine final message length
    // ------------------------------------------------------------------------

    if (h.sequence == h.total - 1) {

      length =
          offset +
          h.payloadLength;
    }

    // Mark fragment received.

    receivedMask |=
        (uint8_t)(1U << h.sequence);

    // ------------------------------------------------------------------------
    // Check completion
    // ------------------------------------------------------------------------

    uint8_t completeMask;

    if (h.total >= 8) {

      completeMask = 0xFF;

    } else {

      completeMask =
          (uint8_t)((1U << h.total) - 1U);
    }

    return
        (receivedMask & completeMask) ==
        completeMask;
  }
};

} // namespace

// ============================================================================
// GLOBAL BLE OBJECTS
// ============================================================================

static BLEServer* g_server = nullptr;

static BLECharacteristic* g_charDeviceInfo = nullptr;

static BLECharacteristic* g_charCommand = nullptr;

static BLECharacteristic* g_charMessage = nullptr;

static BLECharacteristic* g_charStatus = nullptr;

// ============================================================================
// GLOBAL STATE
// ============================================================================

static bool g_connected = false;

static MessageManager* g_messages = nullptr;

static NotificationManager* g_notifier = nullptr;

// Set from BLE callback.
// Consumed in BLEManager::update().
//
// volatile because BLE callbacks can run from another task/context.
//

static volatile bool g_pendingAlert = false;

// ============================================================================
// REASSEMBLERS
// ============================================================================

static Reassembler g_messageReassembler;

static Reassembler g_commandReassembler;

// ============================================================================
// SEND ACK
// ============================================================================
//
// ACK is sent using Status characteristic.
//
// Payload:
//   ASCII representation of message_id
//
// Example:
//   "12345"
//

static void sendAck(uint32_t messageId) {

  if (g_charStatus == nullptr) {
    return;
  }

  char idStr[11];

  snprintf(
      idStr,
      sizeof(idStr),
      "%lu",
      (unsigned long)messageId);

  uint8_t out[
      HEADER_SIZE +
      10 +
      CRC_SIZE];

  size_t len =
      buildPacket(
          PacketType::ACK,
          messageId,
          (const uint8_t*)idStr,
          (uint16_t)strlen(idStr),
          out,
          sizeof(out));

  if (len == 0) {
    return;
  }

  g_charStatus->setValue(
      out,
      len);

  g_charStatus->notify();

  LOGF(
      "[BLEManager] ACK sent for message_id=%lu\n",
      (unsigned long)messageId);
}

// ============================================================================
// HANDLE COMPLETE MESSAGE
// ============================================================================
//
// Expected JSON:
//
// {
//   "source": "APP",
//   "sender": "John",
//   "title": "Test",
//   "body": "Hello",
//   "timestamp": 123456
// }
//

static void handleCompleteMessage(
    uint32_t messageId,
    const uint8_t* data,
    size_t length) {

  if (data == nullptr ||
      length == 0) {

    LOGLN(
        "[BLEManager] empty MESSAGE payload");

    return;
  }

  StaticJsonDocument<512> doc;

  DeserializationError err =
      deserializeJson(
          doc,
          data,
          length);

  if (err) {

    LOGF(
        "[BLEManager] MESSAGE JSON parse failed: %s\n",
        err.c_str());

    return;
  }

  // --------------------------------------------------------------------------
  // Read JSON fields
  // --------------------------------------------------------------------------

  const char* sourceStr =
      doc["source"] | "APP";

  const char* sender =
      doc["sender"] | "Unknown";

  const char* title =
      doc["title"] | "Message";

  const char* body =
      doc["body"] | "";

  // --------------------------------------------------------------------------
  // Determine message source
  // --------------------------------------------------------------------------

  MessageSource source =
      MessageSource::APP;

  if (strcmp(sourceStr, "WHATSAPP") == 0) {

    source =
        MessageSource::WHATSAPP;

  } else if (strcmp(sourceStr, "SYSTEM") == 0) {

    source =
        MessageSource::SYSTEM;

  } else if (strcmp(sourceStr, "SCHEDULED") == 0) {

    source =
        MessageSource::SCHEDULED;
  }

  // --------------------------------------------------------------------------
  // Enqueue message
  // --------------------------------------------------------------------------

  if (g_messages != nullptr) {

    uint32_t id =
        g_messages->enqueue(
            source,
            MessagePriority::NORMAL,
            sender,
            title,
            body);

    if (id != 0) {

      g_pendingAlert = true;
    }
  }

  // --------------------------------------------------------------------------
  // ACK
  // --------------------------------------------------------------------------
  //
  // ACK means:
  //
  // "The packet was received correctly."
  //
  // It does NOT necessarily mean that the local message queue accepted it.
  //

  sendAck(messageId);
}

// ============================================================================
// HANDLE COMPLETE COMMAND
// ============================================================================
//
// Expected JSON:
//
// {
//   "cmd": "clear_queue"
// }
//
// Currently commands are logged only.
// Add actual command dispatch here when command behaviour is defined.
//

static void handleCompleteCommand(
    const uint8_t* data,
    size_t length) {

  if (data == nullptr ||
      length == 0) {

    LOGLN(
        "[BLEManager] empty COMMAND payload");

    return;
  }

  StaticJsonDocument<128> doc;

  DeserializationError err =
      deserializeJson(
          doc,
          data,
          length);

  if (err) {

    LOGF(
        "[BLEManager] COMMAND JSON parse failed: %s\n",
        err.c_str());

    return;
  }

  const char* cmd =
      doc["cmd"] | "";

  LOGF(
      "[BLEManager] command received: %s\n",
      cmd);

  // --------------------------------------------------------------------------
  // Command handling can be added here.
  //
  // Examples:
  //
  // clear_queue
  // mute
  // navigate
  // status
  //
  // --------------------------------------------------------------------------
}

// ============================================================================
// BLE SERVER CALLBACKS
// ============================================================================

class ServerCallbacks : public BLEServerCallbacks {

  void onConnect(BLEServer* server) override {

    g_connected = true;

    LOGLN(
        "[BLEManager] phone connected");
  }

  void onDisconnect(BLEServer* server) override {

    g_connected = false;

    // Clear incomplete packets.

    g_messageReassembler.reset();

    g_commandReassembler.reset();

    LOGLN(
        "[BLEManager] phone disconnected, resuming advertising");

    // Restart advertising.

    BLEDevice::startAdvertising();
  }
};

// ============================================================================
// MESSAGE CHARACTERISTIC CALLBACK
// ============================================================================

class MessageCharCallbacks :
    public BLECharacteristicCallbacks {

  void onWrite(
      BLECharacteristic* c) override {

    // IMPORTANT:
    //
    // ESP32 Arduino Core 3.3.11:
    //
    // BLECharacteristic::getValue()
    //
    // returns Arduino String.
    //
    // Do NOT use:
    //
    // std::string value = c->getValue();
    //
    // because that causes:
    //
    // conversion from 'String' to non-scalar type 'std::string'
    //

    String value =
        c->getValue();

    if (value.length() == 0) {
      return;
    }

    // ------------------------------------------------------------------------
    // Convert Arduino String to byte buffer
    // ------------------------------------------------------------------------

    const uint8_t* data =
        (const uint8_t*)value.c_str();

    size_t dataLength =
        value.length();

    // ------------------------------------------------------------------------
    // Parse packet
    // ------------------------------------------------------------------------

    ParsedHeader header;

    if (!parseFragment(
            data,
            dataLength,
            header)) {

      return;
    }

    // ------------------------------------------------------------------------
    // PING
    // ------------------------------------------------------------------------

    if (header.type ==
        PacketType::PING) {

      return;
    }

    // ------------------------------------------------------------------------
    // Make sure this characteristic receives MESSAGE packets.
    // ------------------------------------------------------------------------

    if (header.type !=
        PacketType::MESSAGE) {

      LOGLN(
          "[BLEManager] unexpected type on Message characteristic, ignoring");

      return;
    }

    // ------------------------------------------------------------------------
    // Reassemble
    // ------------------------------------------------------------------------

    if (g_messageReassembler.addFragment(header)) {

      handleCompleteMessage(
          g_messageReassembler.messageId,
          g_messageReassembler.buffer,
          g_messageReassembler.length);

      g_messageReassembler.reset();
    }
  }
};

// ============================================================================
// COMMAND CHARACTERISTIC CALLBACK
// ============================================================================

class CommandCharCallbacks :
    public BLECharacteristicCallbacks {

  void onWrite(
      BLECharacteristic* c) override {

    // ESP32 Arduino Core 3.3.11 returns Arduino String.

    String value =
        c->getValue();

    if (value.length() == 0) {
      return;
    }

    // ------------------------------------------------------------------------
    // Convert to raw bytes
    // ------------------------------------------------------------------------

    const uint8_t* data =
        (const uint8_t*)value.c_str();

    size_t dataLength =
        value.length();

    // ------------------------------------------------------------------------
    // Parse packet
    // ------------------------------------------------------------------------

    ParsedHeader header;

    if (!parseFragment(
            data,
            dataLength,
            header)) {

      return;
    }

    // ------------------------------------------------------------------------
    // PING
    // ------------------------------------------------------------------------

    if (header.type ==
        PacketType::PING) {

      return;
    }

    // ------------------------------------------------------------------------
    // Make sure this characteristic receives COMMAND packets.
    // ------------------------------------------------------------------------

    if (header.type !=
        PacketType::COMMAND) {

      LOGLN(
          "[BLEManager] unexpected type on Command characteristic, ignoring");

      return;
    }

    // ------------------------------------------------------------------------
    // Reassemble
    // ------------------------------------------------------------------------

    if (g_commandReassembler.addFragment(header)) {

      handleCompleteCommand(
          g_commandReassembler.buffer,
          g_commandReassembler.length);

      g_commandReassembler.reset();
    }
  }
};

// ============================================================================
// BLEManager::begin
// ============================================================================

void BLEManager::begin(
    MessageManager* messageManager,
    NotificationManager* notifier) {

  // --------------------------------------------------------------------------
  // Save manager references
  // --------------------------------------------------------------------------

  _messages =
      messageManager;

  _notifier =
      notifier;

  g_messages =
      messageManager;

  g_notifier =
      notifier;

  // --------------------------------------------------------------------------
  // Initialize BLE
  // --------------------------------------------------------------------------

  BLEDevice::init(
      BLE_DEVICE_NAME);

  // --------------------------------------------------------------------------
  // Create BLE server
  // --------------------------------------------------------------------------

  g_server =
      BLEDevice::createServer();

  if (g_server == nullptr) {

    LOGLN(
        "[BLEManager] ERROR: failed to create BLE server");

    return;
  }

  g_server->setCallbacks(
      new ServerCallbacks());

  // --------------------------------------------------------------------------
  // Create BLE service
  // --------------------------------------------------------------------------

  BLEService* service =
      g_server->createService(
          BLE_SERVICE_UUID);

  if (service == nullptr) {

    LOGLN(
        "[BLEManager] ERROR: failed to create BLE service");

    return;
  }

  // ==========================================================================
  // DEVICE INFO CHARACTERISTIC
  // ==========================================================================

  g_charDeviceInfo =
      service->createCharacteristic(
          BLE_CHAR_DEVICEINFO_UUID,
          BLECharacteristic::PROPERTY_READ);

  if (g_charDeviceInfo != nullptr) {

    String info =
        String(DEVICE_ID_DEFAULT) +
        "|" +
        FIRMWARE_VERSION;

    g_charDeviceInfo->setValue(
        info.c_str());
  }

  // ==========================================================================
  // STATUS CHARACTERISTIC
  // ==========================================================================

  g_charStatus =
      service->createCharacteristic(
          BLE_CHAR_STATUS_UUID,
          BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);

  if (g_charStatus != nullptr) {

    // Descriptor required for notification support.

    g_charStatus->addDescriptor(
        new BLE2902());

    g_charStatus->setValue(
        "READY");
  }

  // ==========================================================================
  // COMMAND CHARACTERISTIC
  // ==========================================================================

  g_charCommand =
      service->createCharacteristic(
          BLE_CHAR_COMMAND_UUID,
          BLECharacteristic::PROPERTY_WRITE);

  if (g_charCommand != nullptr) {

    g_charCommand->setCallbacks(
        new CommandCharCallbacks());
  }

  // ==========================================================================
  // MESSAGE CHARACTERISTIC
  // ==========================================================================

  g_charMessage =
      service->createCharacteristic(
          BLE_CHAR_MESSAGE_UUID,
          BLECharacteristic::PROPERTY_WRITE);

  if (g_charMessage != nullptr) {

    g_charMessage->setCallbacks(
        new MessageCharCallbacks());
  }

  // --------------------------------------------------------------------------
  // Start service
  // --------------------------------------------------------------------------

  service->start();

  // --------------------------------------------------------------------------
  // Configure advertising
  // --------------------------------------------------------------------------

  BLEAdvertising* advertising =
      BLEDevice::getAdvertising();

  if (advertising != nullptr) {

    advertising->addServiceUUID(
        BLE_SERVICE_UUID);

    advertising->setScanResponse(
        true);
  }

  // --------------------------------------------------------------------------
  // Start advertising
  // --------------------------------------------------------------------------

  BLEDevice::startAdvertising();

  LOGLN(
      "[BLEManager] advertising started (BLE_PROTOCOL.md v1 packet format)");
}

// ============================================================================
// BLEManager::update
// ============================================================================

void BLEManager::update(
    DeviceState &state) {

  // --------------------------------------------------------------------------
  // Handle pending message alert
  // --------------------------------------------------------------------------

  if (g_pendingAlert) {

    g_pendingAlert = false;

    if (_notifier != nullptr) {

      _notifier->onNewMessage(
          state,
          MessagePriority::NORMAL);
    }
  }

  // --------------------------------------------------------------------------
  // Update connection state
  // --------------------------------------------------------------------------

  bool nowConnected =
      g_connected;

  if (nowConnected != _wasConnected) {

    _wasConnected =
        nowConnected;

    if (nowConnected) {

      state.bleState =
          BleState::CONNECTED;

    } else {

      state.bleState =
          BleState::ADVERTISING;
    }

    // ------------------------------------------------------------------------
    // Connection / disconnection notification
    // ------------------------------------------------------------------------

    if (_notifier != nullptr) {

      if (nowConnected) {

        _notifier->playBleConnected();

      } else {

        _notifier->playBleDisconnected();
      }
    }
  }

  // --------------------------------------------------------------------------
  // If BLE isn't connected, make sure state says advertising.
  // --------------------------------------------------------------------------

  else if (!nowConnected &&
           state.bleState == BleState::IDLE) {

    state.bleState =
        BleState::ADVERTISING;
  }

  // ==========================================================================
  // PERIODIC STATUS UPDATE
  // ==========================================================================
  //
  // Sends a STATUS_UPDATE every 5 seconds while connected.
  //
  // Example JSON:
  //
  // {
  //   "wifi": true,
  //   "ble": true,
  //   "battery": -1
  // }
  //

  static uint32_t lastStatusPush = 0;

  if (g_charStatus != nullptr &&
      nowConnected &&
      millis() - lastStatusPush > 5000) {

    lastStatusPush =
        millis();

    // ------------------------------------------------------------------------
    // Build JSON
    // ------------------------------------------------------------------------

    StaticJsonDocument<128> doc;

    doc["wifi"] =
        (state.wifiState ==
         WifiState::CONNECTED);

    doc["ble"] =
        true;

    // No fuel gauge on current board.

    doc["battery"] =
        -1;

    char json[128];

    size_t jsonLen =
        serializeJson(
            doc,
            json,
            sizeof(json));

    // ------------------------------------------------------------------------
    // Build BLE packet
    // ------------------------------------------------------------------------

    uint8_t out[
        HEADER_SIZE +
        128 +
        CRC_SIZE];

    size_t len =
        buildPacket(
            PacketType::STATUS_UPDATE,
            0,
            (const uint8_t*)json,
            (uint16_t)jsonLen,
            out,
            sizeof(out));

    // ------------------------------------------------------------------------
    // Send notification
    // ------------------------------------------------------------------------

    if (len > 0) {

      g_charStatus->setValue(
          out,
          len);

      g_charStatus->notify();
    }
  }
}

// ============================================================================
// BLEManager::isConnected
// ============================================================================

bool BLEManager::isConnected() const {

  return g_connected;
}