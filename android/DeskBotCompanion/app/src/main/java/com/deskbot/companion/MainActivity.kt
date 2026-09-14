package com.deskbot.companion

import android.Manifest
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.ui.Modifier
import com.deskbot.companion.ui.AppNavHost
import com.deskbot.companion.ui.theme.DeskBotTheme
import com.deskbot.companion.ui.theme.Void

/**
 * Single-Activity app (per master spec §12/§48's screen list, implemented
 * as Compose destinations under ui/screens/ rather than separate
 * Activities). This class's only real job beyond hosting Compose is
 * requesting the runtime permissions BLE/notifications need — see
 * requestRuntimePermissions() — everything else lives in ui/.
 */
class MainActivity : ComponentActivity() {

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { /* individual screens re-check their own required permissions on resume */ }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestRuntimePermissions()

        setContent {
            DeskBotTheme {
                androidx.compose.material3.Surface(
                    modifier = Modifier.fillMaxSize().background(Void),
                ) {
                    AppNavHost()
                }
            }
        }
    }

    /**
     * Requests only what's needed for the current OS version (master spec
     * §44: "Do not request permissions unnecessarily"). Notification-listener
     * access and exact-alarm-style settings aren't in this list because
     * they're not requestable via the runtime-permission dialog — those are
     * handled with explicit "open settings" buttons in ui/screens/WhatsAppScreen.kt
     * and ui/screens/DevicesScreen.kt instead.
     */
    private fun requestRuntimePermissions() {
        val permissions = buildList {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                add(Manifest.permission.BLUETOOTH_SCAN)
                add(Manifest.permission.BLUETOOTH_CONNECT)
            } else {
                add(Manifest.permission.ACCESS_FINE_LOCATION)
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                add(Manifest.permission.POST_NOTIFICATIONS)
            }
        }
        permissionLauncher.launch(permissions.toTypedArray())
    }
}
