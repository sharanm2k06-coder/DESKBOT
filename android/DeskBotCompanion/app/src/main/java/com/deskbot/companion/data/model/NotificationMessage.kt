package com.deskbot.companion.data.model

import org.json.JSONObject

/** Priority levels, mirrored from the backend/firmware spec (§10). */
enum class MessagePriority { NORMAL, IMPORTANT, ALERT }

/** Where a message originated — matches Message.source across the whole system. */
enum class MessageSource { APP, WHATSAPP, SYSTEM, SCHEDULED }

/**
 * The normalized shape every notification (WhatsApp or otherwise) is
 * converted into before it's sent over BLE. This is intentionally the same
 * shape documented in docs/BLE_PROTOCOL.md's MESSAGE payload and the
 * master spec's §10 Message object, so the ESP32 doesn't need per-source
 * parsing logic.
 */
data class NotificationMessage(
    val source: MessageSource,
    val sender: String?,
    val title: String?,
    val body: String,
    val timestamp: Long,
) {
    fun toJson(): String = JSONObject().apply {
        put("source", source.name)
        put("sender", sender ?: JSONObject.NULL)
        put("title", title ?: JSONObject.NULL)
        put("body", body)
        put("timestamp", timestamp)
    }.toString()

    companion object {
        /**
         * Parses the JSON payload carried inside a BLE MESSAGE packet.
         * Never throws on missing/malformed fields — a message missing
         * `body` is not worth crashing over (see master spec §45: "do not
         * crash if fields are missing").
         */
        fun fromJson(json: String): NotificationMessage? {
            return try {
                val obj = JSONObject(json)
                val body = obj.optString("body", "").ifBlank { return null }
                NotificationMessage(
                    source = runCatching { MessageSource.valueOf(obj.optString("source", "SYSTEM")) }
                        .getOrDefault(MessageSource.SYSTEM),
                    sender = obj.optString("sender", null).takeUnless { it.isNullOrBlank() },
                    title = obj.optString("title", null).takeUnless { it.isNullOrBlank() },
                    body = body,
                    timestamp = obj.optLong("timestamp", System.currentTimeMillis()),
                )
            } catch (e: Exception) {
                null
            }
        }
    }
}
