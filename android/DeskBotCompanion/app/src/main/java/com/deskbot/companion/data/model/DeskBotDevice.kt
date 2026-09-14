package com.deskbot.companion.data.model

/** A DeskBot as registered on the backend AND/OR discovered over BLE. */
data class DeskBotDevice(
    val id: String,               // backend device id, empty if only BLE-known
    val deviceUid: String,        // e.g. "DESKBOT-01"
    val name: String,
    val bleAddress: String? = null,
    val backendStatus: String? = null, // "ONLINE" | "OFFLINE" | "UNKNOWN"
)

enum class ConnectionState { DISCONNECTED, SCANNING, CONNECTING, CONNECTED }

/** A message sent from this app, for local history display. */
data class SentMessage(
    val id: String,
    val body: String,
    val priority: MessagePriority,
    val sentAt: Long,
    val delivered: Boolean,
)
