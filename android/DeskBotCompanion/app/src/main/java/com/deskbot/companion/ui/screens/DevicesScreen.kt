package com.deskbot.companion.ui.screens

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import androidx.navigation.NavHostController
import com.deskbot.companion.data.local.AppPreferences
import com.deskbot.companion.data.model.ConnectionState
import com.deskbot.companion.service.BleManagerHolder
import com.deskbot.companion.service.DeskBotForegroundService
import com.deskbot.companion.ui.Routes
import com.deskbot.companion.ui.components.Panel
import com.deskbot.companion.ui.components.PillTone
import com.deskbot.companion.ui.components.ScreenScaffold
import com.deskbot.companion.ui.components.StatusPill
import com.deskbot.companion.ui.theme.Ink
import com.deskbot.companion.ui.theme.InkFaint
import kotlinx.coroutines.launch

private fun hasBlePermissions(context: android.content.Context): Boolean {
    val required = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        listOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
    } else {
        listOf(Manifest.permission.ACCESS_FINE_LOCATION)
    }
    return required.all { ContextCompat.checkSelfPermission(context, it) == PackageManager.PERMISSION_GRANTED }
}

@Composable
fun DevicesScreen(navController: NavHostController) {
    val context = LocalContext.current
    val bleManager = remember { BleManagerHolder.getOrCreate(context) }
    val prefs = remember { AppPreferences(context) }
    val scope = rememberCoroutineScope()

    val connectionState by bleManager.connectionState.collectAsState()
    val scanResults by bleManager.scanResults.collectAsState()
    val keepConnected by prefs.keepConnected.collectAsState(initial = true)

    var permissionMissing by remember { mutableStateOf(!hasBlePermissions(context)) }

    ScreenScaffold(navController, title = "Device", currentRoute = Routes.DEVICES) { modifier ->
        Column(
            modifier.padding(16.dp).fillMaxSize(),
            verticalArrangement = Arrangement.spacedBy(16.dp),
        ) {
            if (permissionMissing) {
                Panel(title = "Permission needed") {
                    Text(
                        "DeskBot needs Bluetooth permission to scan for and connect to your device.",
                        color = InkFaint,
                    )
                    Spacer(Modifier.height(8.dp))
                    Button(onClick = { permissionMissing = !hasBlePermissions(context) }) {
                        Text("Re-check permission")
                    }
                }
            }

            Panel(title = "Status") {
                Row(horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically, modifier = Modifier.fillMaxWidth()) {
                    StatusPill(
                        text = connectionState.name.lowercase(),
                        tone = if (connectionState == ConnectionState.CONNECTED) PillTone.ONLINE else PillTone.OFFLINE,
                    )
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text("Keep connected", color = Ink, modifier = Modifier.padding(end = 8.dp))
                        Switch(
                            checked = keepConnected,
                            onCheckedChange = { checked -> scope.launch { prefs.setKeepConnected(checked) } },
                        )
                    }
                }
            }

            Panel(title = "Scan for DeskBot") {
                Button(
                    onClick = { if (!permissionMissing) bleManager.startScan() },
                    enabled = !permissionMissing && connectionState != ConnectionState.SCANNING,
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Text(if (connectionState == ConnectionState.SCANNING) "Scanning···" else "Scan")
                }
            }

            Panel(title = "Found devices", modifier = Modifier.weight(1f)) {
                if (scanResults.isEmpty()) {
                    Text("No DeskBot found yet. Make sure it's powered on and nearby.", color = InkFaint)
                } else {
                    LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        items(scanResults) { result ->
                            Row(
                                horizontalArrangement = Arrangement.SpaceBetween,
                                verticalAlignment = Alignment.CenterVertically,
                                modifier = Modifier.fillMaxWidth(),
                            ) {
                                Column {
                                    Text(result.name ?: "Unnamed DeskBot", color = Ink)
                                    Text(result.address, color = InkFaint, style = MaterialTheme.typography.bodySmall)
                                }
                                Button(onClick = {
                                    scope.launch { prefs.setLastDevice(result.address, result.name ?: "DeskBot") }
                                    val intent = Intent(context, DeskBotForegroundService::class.java)
                                        .putExtra(DeskBotForegroundService.EXTRA_DEVICE_ADDRESS, result.address)
                                    ContextCompat.startForegroundService(context, intent)
                                }) {
                                    Text("Connect")
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
