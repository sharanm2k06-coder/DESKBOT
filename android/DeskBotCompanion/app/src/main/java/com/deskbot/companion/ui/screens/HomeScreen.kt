package com.deskbot.companion.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.navigation.NavHostController
import com.deskbot.companion.data.local.AppPreferences
import com.deskbot.companion.data.model.ConnectionState
import com.deskbot.companion.notifications.DeskBotNotificationListenerService
import com.deskbot.companion.service.BleManagerHolder
import com.deskbot.companion.ui.Routes
import com.deskbot.companion.ui.components.Panel
import com.deskbot.companion.ui.components.PillTone
import com.deskbot.companion.ui.components.ScreenScaffold
import com.deskbot.companion.ui.components.StatusPill
import com.deskbot.companion.ui.theme.Ink
import com.deskbot.companion.ui.theme.InkDim
import kotlinx.coroutines.launch

@Composable
fun HomeScreen(navController: NavHostController) {
    val context = LocalContext.current
    val bleManager = remember { BleManagerHolder.getOrCreate(context) }
    val prefs = remember { AppPreferences(context) }
    val scope = rememberCoroutineScope()

    val connectionState by bleManager.connectionState.collectAsState()
    val forwardingEnabled by prefs.whatsappForwardingEnabled.collectAsState(initial = true)
    val lastDeviceName by prefs.lastDeviceName.collectAsState(initial = null)
    val listenerConnected = remember { DeskBotNotificationListenerService.isConnected() }

    ScreenScaffold(navController, title = "DeskBot", currentRoute = Routes.HOME) { modifier ->
        Column(
            modifier
                .padding(16.dp)
                .fillMaxSize(),
            verticalArrangement = Arrangement.spacedBy(16.dp),
        ) {
            Panel(title = "Connection") {
                Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.fillMaxWidth()) {
                    Column {
                        Text(lastDeviceName ?: "No device paired", style = MaterialTheme.typography.titleMedium, color = Ink)
                        Spacer(Modifier.height(6.dp))
                        StatusPill(
                            text = connectionState.name.lowercase(),
                            tone = if (connectionState == ConnectionState.CONNECTED) PillTone.ONLINE else PillTone.OFFLINE,
                        )
                    }
                }
            }

            Panel(title = "WhatsApp forwarding") {
                Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.fillMaxWidth()) {
                    Column(Modifier.weight(1f)) {
                        Text(if (forwardingEnabled) "On" else "Off", color = Ink)
                        Text(
                            if (listenerConnected) "Notification access granted" else "Notification access not granted yet",
                            style = MaterialTheme.typography.bodySmall,
                            color = InkDim,
                        )
                    }
                    Switch(
                        checked = forwardingEnabled,
                        onCheckedChange = { checked -> scope.launch { prefs.setWhatsappForwardingEnabled(checked) } },
                    )
                }
            }

            Panel(title = "Quick actions") {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    OutlinedButton(onClick = { navController.navigate(Routes.DEVICES) }, modifier = Modifier.fillMaxWidth()) {
                        Text("Connect a DeskBot")
                    }
                    OutlinedButton(onClick = { navController.navigate(Routes.WHATSAPP) }, modifier = Modifier.fillMaxWidth()) {
                        Text("WhatsApp settings")
                    }
                }
            }
        }
    }
}
