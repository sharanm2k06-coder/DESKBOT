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

/**
 * Scheduled (recurring) messages are created against the backend (see
 * ../../../backend and frontend/app/schedule), which the ESP32 polls
 * directly — the phone/BLE path isn't involved in that flow at all (it's
 * Web → Render → ESP32, per master spec §31/§32). This screen is
 * deliberately just a pointer, rather than a duplicate half-implementation
 * of the web app's scheduling UI.
 */
@Composable
fun SchedulesScreen(navController: NavHostController) {
    ScreenScaffold(navController, title = "Schedule", currentRoute = Routes.SCHEDULES) { modifier ->
        Column(modifier.padding(16.dp).fillMaxSize()) {
            Panel(title = "Scheduled messages") {
                Text(
                    "Scheduled messages are created from the DeskBot web console, since that's " +
                        "where the timing logic lives on the backend. Your DeskBot will receive " +
                        "them the same way it receives any other message — no phone connection " +
                        "required.",
                    color = InkFaint,
                    style = MaterialTheme.typography.bodyMedium,
                )
                Spacer(Modifier.height(10.dp))
                Text(
                    "Open the web console at the address configured in Settings to create one.",
                    color = InkFaint,
                    style = MaterialTheme.typography.bodySmall,
                )
            }
        }
    }
}
