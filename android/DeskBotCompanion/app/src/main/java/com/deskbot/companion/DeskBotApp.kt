package com.deskbot.companion

import android.app.Application

class DeskBotApp : Application() {
    override fun onCreate() {
        super.onCreate()
        // Intentionally minimal: BleManager/ApiClient/SecureStorage are all
        // constructed lazily where first needed (see service/BleManagerHolder.kt
        // and ui/AppContainer.kt) rather than eagerly here.
    }
}
