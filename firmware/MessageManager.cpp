#include "MessageManager.h"
#include "Logger.h"
#include <string.h>

void MessageManager::begin() {
  for (auto &m : _slots) m.valid = false;
  _nextId = 1;
  LOGLN("[MessageManager] ready");
}

int MessageManager::findFreeSlot() const {
  for (int i = 0; i < MAX_QUEUED_MESSAGES; i++) {
    if (!_slots[i].valid) return i;
  }
  return -1;
}

uint32_t MessageManager::enqueue(MessageSource source,
                                  MessagePriority priority,
                                  const char* sender,
                                  const char* title,
                                  const char* body,
                                  uint32_t ttlMs) {
  int slot = findFreeSlot();
  if (slot < 0) {
    // Queue full: drop the oldest NORMAL-priority message to make room for
    // anything ALERT/IMPORTANT; otherwise refuse (never silently drop an
    // ALERT to make room for a NORMAL one).
    int oldestNormal = -1;
    uint32_t oldestCreated = UINT32_MAX;
    for (int i = 0; i < MAX_QUEUED_MESSAGES; i++) {
      if (_slots[i].valid && _slots[i].priority == MessagePriority::NORMAL &&
          _slots[i].createdAtMs < oldestCreated) {
        oldestCreated = _slots[i].createdAtMs;
        oldestNormal = i;
      }
    }
    if (oldestNormal >= 0 && priority != MessagePriority::NORMAL) {
      slot = oldestNormal;
      LOGLN("[MessageManager] queue full, evicted oldest NORMAL message");
    } else {
      LOGLN("[MessageManager] queue full, message rejected");
      return 0;
    }
  }

  Message &m = _slots[slot];
  m.id = _nextId++;
  m.source = source;
  m.priority = priority;
  strncpy(m.sender, sender ? sender : "", MESSAGE_SENDER_MAX_LEN - 1);
  m.sender[MESSAGE_SENDER_MAX_LEN - 1] = '\0';
  strncpy(m.title, title ? title : "", MESSAGE_TITLE_MAX_LEN - 1);
  m.title[MESSAGE_TITLE_MAX_LEN - 1] = '\0';
  strncpy(m.body, body ? body : "", MESSAGE_BODY_MAX_LEN - 1);
  m.body[MESSAGE_BODY_MAX_LEN - 1] = '\0';
  m.createdAtMs = millis();
  m.ttlMs = ttlMs;
  m.read = false;
  m.valid = true;

  LOGF("[MessageManager] enqueued id=%lu source=%d priority=%d\n",
       (unsigned long)m.id, (int)source, (int)priority);
  return m.id;
}

void MessageManager::update() {
  uint32_t now = millis();
  for (auto &m : _slots) {
    if (m.valid && (now - m.createdAtMs) > m.ttlMs) {
      LOGF("[MessageManager] expiring id=%lu\n", (unsigned long)m.id);
      m.valid = false;
    }
  }
}

int MessageManager::count() const {
  int c = 0;
  for (const auto &m : _slots) if (m.valid) c++;
  return c;
}

const Message* MessageManager::getByDisplayIndex(int displayIndex) const {
  int c = 0;
  for (const auto &m : _slots) {
    if (m.valid) {
      if (c == displayIndex) return &m;
      c++;
    }
  }
  return nullptr;
}

void MessageManager::markReadByDisplayIndex(int displayIndex) {
  int c = 0;
  for (auto &m : _slots) {
    if (m.valid) {
      if (c == displayIndex) { m.read = true; return; }
      c++;
    }
  }
}

void MessageManager::acknowledgeAndRemove(uint32_t messageId) {
  for (auto &m : _slots) {
    if (m.valid && m.id == messageId) {
      m.valid = false;
      LOGF("[MessageManager] acknowledged/removed id=%lu\n", (unsigned long)messageId);
      return;
    }
  }
}

bool MessageManager::wasRecentlySeen(uint32_t externalId) const {
  if (externalId == 0) return false;
  for (uint8_t i = 0; i < SEEN_RING_SIZE; i++) {
    if (_seenRing[i] == externalId) return true;
  }
  return false;
}

void MessageManager::rememberSeen(uint32_t externalId) {
  _seenRing[_seenRingPos] = externalId;
  _seenRingPos = (_seenRingPos + 1) % SEEN_RING_SIZE;
}
