package com.deskbot.companion.ble

import android.bluetooth.BluetoothDevice
import java.util.UUID

/** UUIDs from docs/BLE_PROTOCOL.md — keep these two files in sync. */
object DeskBotGatt {
    val SERVICE_UUID: UUID = UUID.fromString("7d4a0001-8c4b-4f5b-9e11-123456789abc")
    val CHAR_DEVICE_INFO: UUID = UUID.fromString("7d4a0002-8c4b-4f5b-9e11-123456789abc")
    val CHAR_COMMAND: UUID = UUID.fromString("7d4a0003-8c4b-4f5b-9e11-123456789abc")
    val CHAR_MESSAGE: UUID = UUID.fromString("7d4a0004-8c4b-4f5b-9e11-123456789abc")
    val CHAR_STATUS: UUID = UUID.fromString("7d4a0005-8c4b-4f5b-9e11-123456789abc")
    val CCCD_UUID: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
}

data class BleScanResult(
    val device: BluetoothDevice,
    val name: String?,
    val address: String,
    val rssi: Int,
)
