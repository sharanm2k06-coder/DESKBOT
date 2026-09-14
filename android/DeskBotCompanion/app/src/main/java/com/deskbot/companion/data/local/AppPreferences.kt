package com.deskbot.companion.data.local

import android.content.Context
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

private val Context.dataStore by preferencesDataStore(name = "deskbot_prefs")

/**
 * Everyday app settings — none of this is sensitive (see SecureStorage for
 * what is). Notably includes the WhatsApp-forwarding toggle from master
 * spec §43: "Allow the user to disable WhatsApp forwarding."
 */
class AppPreferences(private val context: Context) {

    private object Keys {
        val WHATSAPP_FORWARDING_ENABLED = booleanPreferencesKey("whatsapp_forwarding_enabled")
        val KEEP_CONNECTED = booleanPreferencesKey("keep_deskbot_connected")
        val LAST_DEVICE_ADDRESS = stringPreferencesKey("last_device_address")
        val LAST_DEVICE_NAME = stringPreferencesKey("last_device_name")
    }

    val whatsappForwardingEnabled: Flow<Boolean> =
        context.dataStore.data.map { it[Keys.WHATSAPP_FORWARDING_ENABLED] ?: true }

    suspend fun setWhatsappForwardingEnabled(enabled: Boolean) {
        context.dataStore.edit { it[Keys.WHATSAPP_FORWARDING_ENABLED] = enabled }
    }

    val keepConnected: Flow<Boolean> =
        context.dataStore.data.map { it[Keys.KEEP_CONNECTED] ?: true }

    suspend fun setKeepConnected(enabled: Boolean) {
        context.dataStore.edit { it[Keys.KEEP_CONNECTED] = enabled }
    }

    val lastDeviceAddress: Flow<String?> =
        context.dataStore.data.map { it[Keys.LAST_DEVICE_ADDRESS] }

    val lastDeviceName: Flow<String?> =
        context.dataStore.data.map { it[Keys.LAST_DEVICE_NAME] }

    suspend fun setLastDevice(address: String, name: String) {
        context.dataStore.edit {
            it[Keys.LAST_DEVICE_ADDRESS] = address
            it[Keys.LAST_DEVICE_NAME] = name
        }
    }
}
