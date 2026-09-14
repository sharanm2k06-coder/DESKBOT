#pragma once
#include <Arduino.h>
#include "config.h"
#include "DeviceState.h"

// A small, fixed-capacity, RAM-only queue. No SPIFFS/NVS persistence by
// design: message bodies (esp. WhatsApp-sourced ones, arriving in Phase 4)
// must not be stored permanently on the device. On reboot the queue is
// simply empty — this is intentional, not a bug.
class MessageManager {
 public:
  void begin();

  // Returns the assigned id, or 0 if the queue is full and the message
  // could not be enqueued (caller should treat 0 as failure).
  uint32_t enqueue(MessageSource source,
                    MessagePriority priority,
                    const char* sender,
                    const char* title,
                    const char* body,
                    uint32_t ttlMs = MESSAGE_DEFAULT_TTL_MS);

  // Call once per loop() — expires stale messages. Non-blocking.
  void update();

  int count() const;
  bool empty() const { return count() == 0; }

  // Index is 0-based across currently VALID messages only (i.e. it's stable
  // for UI paging even though the underlying slot indices may have gaps).
  const Message* getByDisplayIndex(int displayIndex) const;
  void markReadByDisplayIndex(int displayIndex);
  void acknowledgeAndRemove(uint32_t messageId);

  bool wasRecentlySeen(uint32_t externalId) const; // duplicate-prevention hook

 private:
  Message _slots[MAX_QUEUED_MESSAGES];
  uint32_t _nextId = 1;

  // small ring of recently-seen external ids (e.g. from BLE/API) to prevent
  // double-enqueueing the same source message if it's retried.
  static const uint8_t SEEN_RING_SIZE = 16;
  uint32_t _seenRing[SEEN_RING_SIZE] = {0};
  uint8_t _seenRingPos = 0;

  int findFreeSlot() const;
  void rememberSeen(uint32_t externalId);
};
