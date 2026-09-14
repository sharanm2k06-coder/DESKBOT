package com.deskbot.companion.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.navigation.NavHostController
import com.deskbot.companion.ble.BleProtocol
import com.deskbot.companion.ble.SendResult
import com.deskbot.companion.data.model.ConnectionState
import com.deskbot.companion.data.model.MessagePriority
import com.deskbot.companion.data.model.MessageSource
import com.deskbot.companion.data.model.NotificationMessage
import com.deskbot.companion.data.model.SentMessage
import com.deskbot.companion.service.BleManagerHolder
import com.deskbot.companion.ui.Routes
import com.deskbot.companion.ui.components.Panel
import com.deskbot.companion.ui.components.ScreenScaffold
import com.deskbot.companion.ui.theme.Alert
import com.deskbot.companion.ui.theme.Ink
import com.deskbot.companion.ui.theme.InkFaint
import com.deskbot.companion.ui.theme.Online
import kotlinx.coroutines.launch

/**
 * This app deliberately does NOT duplicate the web frontend's full send-a-
 * message flow (that lives in the backend/web path, master spec §11/§68 —
 * "ESP32 does not directly communicate with WhatsApp" and the app's job is
 * BLE + notification forwarding). What this screen offers instead is a
 * direct BLE test-send, useful for confirming the connection actually
 * works without needing the web app open too.
 */
@Composable
fun MessagesScreen(navController: NavHostController) {
    val context = LocalContext.current
    val bleManager = remember { BleManagerHolder.getOrCreate(context) }
    val connectionState by bleManager.connectionState.collectAsState()
    val scope = rememberCoroutineScope()

    var text by remember { mutableStateOf("") }
    var sending by remember { mutableStateOf(false) }
    var sentLog by remember { mutableStateOf(listOf<SentMessage>()) }

    ScreenScaffold(navController, title = "Messages", currentRoute = Routes.MESSAGES) { modifier ->
        Column(modifier.padding(16.dp).fillMaxSize(), verticalArrangement = Arrangement.spacedBy(16.dp)) {
            Panel(title = "Send a test message") {
                OutlinedTextField(
                    value = text,
                    onValueChange = { text = it },
                    modifier = Modifier.fillMaxWidth(),
                    placeholder = { Text("Hello from the phone") },
                    maxLines = 3,
                )
                Spacer(Modifier.height(10.dp))
                Button(
                    onClick = {
                        val body = text.trim()
                        if (body.isEmpty() || connectionState != ConnectionState.CONNECTED) return@Button
                        sending = true
                        scope.launch {
                            val notification = NotificationMessage(
                                source = MessageSource.APP,
                                sender = "You",
                                title = "DeskBot Companion",
                                body = body,
                                timestamp = System.currentTimeMillis(),
                            )
                            val result = bleManager.sendMessage(
                                BleProtocol.MessageType.MESSAGE,
                                notification.toJson().toByteArray(),
                            )
                            sentLog = listOf(
                                SentMessage(
                                    id = System.currentTimeMillis().toString(),
                                    body = body,
                                    priority = MessagePriority.NORMAL,
                                    sentAt = System.currentTimeMillis(),
                                    delivered = result is SendResult.Delivered,
                                ),
                            ) + sentLog
                            text = ""
                            sending = false
                        }
                    },
                    enabled = connectionState == ConnectionState.CONNECTED && !sending && text.isNotBlank(),
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Text(if (sending) "Sending···" else "Send over BLE")
                }
                if (connectionState != ConnectionState.CONNECTED) {
                    Spacer(Modifier.height(6.dp))
                    Text("Connect to a DeskBot first, from the Device tab.", color = InkFaint, style = MaterialTheme.typography.bodySmall)
                }
            }

            Panel(title = "Recent activity", modifier = Modifier.weight(1f)) {
                if (sentLog.isEmpty()) {
                    Text("Nothing sent this session.", color = InkFaint)
                } else {
                    LazyColumn(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                        items(sentLog) { msg ->
                            Column {
                                Text(msg.body, color = Ink)
                                Text(
                                    if (msg.delivered) "Delivered" else "Failed to deliver",
                                    color = if (msg.delivered) Online else Alert,
                                    style = MaterialTheme.typography.bodySmall,
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}
