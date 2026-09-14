package com.deskbot.companion.service

import android.content.Context
import com.deskbot.companion.ble.BleManager

/**
 * There must be exactly one BluetoothGatt connection to a given device at
 * a time — a second BleManager instance would just fight the first one for
 * the radio. Compose screens and DeskBotForegroundService both go through
 * this holder rather than constructing their own BleManager.
 */
object BleManagerHolder {
    @Volatile
    private var instance: BleManager? = null

    fun getOrCreate(context: Context): BleManager =
        instance ?: synchronized(this) {
            instance ?: BleManager(context.applicationContext).also { instance = it }
        }
}
