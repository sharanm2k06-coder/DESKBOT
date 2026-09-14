package com.deskbot.companion.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.navigation.NavHostController
import com.deskbot.companion.ui.Routes
import com.deskbot.companion.ui.components.Panel
import com.deskbot.companion.ui.components.ScreenScaffold
import com.deskbot.companion.ui.theme.InkFaint

@Composable
fun AboutScreen(navController: NavHostController) {
    ScreenScaffold(navController, title = "About", currentRoute = Routes.SETTINGS) { modifier ->
        Column(modifier.padding(16.dp).fillMaxSize(), verticalArrangement = Arrangement.spacedBy(16.dp)) {
            Panel(title = "DeskBot Companion") {
                Text(
                    "Connects your phone to a DeskBot desktop companion over Bluetooth Low " +
                        "Energy, and forwards WhatsApp notifications to it via Android's " +
                        "notification listener — WhatsApp itself is never accessed directly.",
                    style = MaterialTheme.typography.bodyMedium,
                    color = InkFaint,
                )
            }
            Panel(title = "Privacy") {
                Text(
                    "Message text is held in memory only long enough to send it over Bluetooth " +
                        "and is never written to disk or uploaded anywhere. See docs/BLE_PROTOCOL.md " +
                        "in the project repository for the full data-handling notes.",
                    style = MaterialTheme.typography.bodyMedium,
                    color = InkFaint,
                )
            }
        }
    }
}
