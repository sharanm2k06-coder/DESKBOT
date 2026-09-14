package com.deskbot.companion.security

import android.content.Context
import android.content.SharedPreferences
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey

/**
 * Wraps EncryptedSharedPreferences (AES256-GCM, key in the Android
 * Keystore) for the two secrets this app ever holds:
 *   - the backend JWT (from /auth/login)
 *   - nothing from WhatsApp is ever stored here — see docs/BLE_PROTOCOL.md
 *     "Security considerations": message bodies are memory-only.
 *
 * The DeskBot device's own BLE address/name are NOT secrets and live in
 * regular (unencrypted) DataStore prefs instead — see AppPreferences.
 */
class SecureStorage(context: Context) {

    private val masterKey = MasterKey.Builder(context)
        .setKeyScheme(MasterKey.KeyScheme.AES256_GCM)
        .build()

    private val prefs: SharedPreferences = EncryptedSharedPreferences.create(
        context,
        "deskbot_secure_prefs",
        masterKey,
        EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
        EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM,
    )

    var backendToken: String?
        get() = prefs.getString(KEY_TOKEN, null)
        set(value) = prefs.edit().putString(KEY_TOKEN, value).apply()

    fun clear() = prefs.edit().clear().apply()

    companion object {
        private const val KEY_TOKEN = "backend_jwt"
    }
}
