package com.deskbot.companion.ui.screens

import android.content.Intent
import android.provider.Settings
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.navigation.NavHostController
import com.deskbot.companion.data.local.AppPreferences
import com.deskbot.companion.notifications.DeskBotNotificationListenerService
import com.deskbot.companion.ui.Routes
import com.deskbot.companion.ui.components.Panel
import com.deskbot.companion.ui.theme.Ink
import com.deskbot.companion.ui.theme.InkFaint
import com.deskbot.companion.ui.components.ScreenScaffold
import kotlinx.coroutines.launch

@Composable
fun WhatsAppScreen(navController: NavHostController) {
    val context = LocalContext.current
    val prefs = remember { AppPreferences(context) }
    val scope = rememberCoroutineScope()
    val forwardingEnabled by prefs.whatsappForwardingEnabled.collectAsState(initial = true)
    var listenerConnected by remember { mutableStateOf(DeskBotNotificationListenerService.isConnected()) }

    ScreenScaffold(navController, title = "WhatsApp", currentRoute = Routes.HOME) { modifier ->
        Column(modifier.padding(16.dp).fillMaxSize(), verticalArrangement = Arrangement.spacedBy(16.dp)) {
            Panel(title = "Notification access") {
                Text(
                    if (listenerConnected)
                        "Granted — DeskBot Companion can read WhatsApp's notifications on this phone."
                    else
                        "Not granted yet. Android requires this permission to be turned on from system Settings, not from inside the app.",
                    color = if (listenerConnected) Ink else InkFaint,
                )
                Spacer(Modifier.height(10.dp))
                if (!listenerConnected) {
                    Button(onClick = {
                        context.startActivity(Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
                    }) {
                        Text("Open notification access settings")
                    }
                    Spacer(Modifier.height(8.dp))
                    OutlinedButton(onClick = { listenerConnected = DeskBotNotificationListenerService.isConnected() }) {
                        Text("I granted it — re-check")
                    }
                }
            }

            Panel(title = "Forwarding") {
                Row(horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.fillMaxWidth()) {
                    Column(Modifier.weight(1f)) {
                        Text("Forward WhatsApp messages to DeskBot", color = Ink)
                        Text(
                            "When on, new WhatsApp notifications are sent to your connected DeskBot over Bluetooth. Message text is never uploaded or stored — see docs/BLE_PROTOCOL.md.",
                            color = InkFaint,
                            style = MaterialTheme.typography.bodySmall,
                        )
                    }
                    Switch(
                        checked = forwardingEnabled,
                        onCheckedChange = { checked -> scope.launch { prefs.setWhatsappForwardingEnabled(checked) } },
                    )
                }
            }
        }
    }
}
