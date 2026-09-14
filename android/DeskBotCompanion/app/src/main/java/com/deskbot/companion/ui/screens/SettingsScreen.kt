package com.deskbot.companion.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.navigation.NavHostController
import com.deskbot.companion.network.ApiClient
import com.deskbot.companion.network.ApiException
import com.deskbot.companion.security.SecureStorage
import com.deskbot.companion.ui.Routes
import com.deskbot.companion.ui.components.Panel
import com.deskbot.companion.ui.components.ScreenScaffold
import com.deskbot.companion.ui.theme.Alert
import com.deskbot.companion.ui.theme.Ink
import com.deskbot.companion.ui.theme.InkFaint
import com.deskbot.companion.ui.theme.Online
import kotlinx.coroutines.launch

// Same default the web frontend uses locally — override per-build via
// BuildConfig/resource overlay for a real deployment.
private const val DEFAULT_BACKEND_URL = "https://deskbot-backend.onrender.com"

@Composable
fun SettingsScreen(navController: NavHostController) {
    val context = LocalContext.current
    val secureStorage = remember { SecureStorage(context) }
    val scope = rememberCoroutineScope()

    var backendUrl by remember { mutableStateOf(DEFAULT_BACKEND_URL) }
    var email by remember { mutableStateOf("") }
    var password by remember { mutableStateOf("") }
    var status by remember { mutableStateOf<String?>(null) }
    var statusIsError by remember { mutableStateOf(false) }
    var signedIn by remember { mutableStateOf(secureStorage.backendToken != null) }

    ScreenScaffold(navController, title = "Settings", currentRoute = Routes.SETTINGS) { modifier ->
        Column(modifier.padding(16.dp).fillMaxSize(), verticalArrangement = Arrangement.spacedBy(16.dp)) {
            Panel(title = "Backend") {
                OutlinedTextField(
                    value = backendUrl,
                    onValueChange = { backendUrl = it },
                    label = { Text("API URL") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth(),
                )
            }

            Panel(title = "Account") {
                if (signedIn) {
                    Text("Signed in.", color = Online)
                    Spacer(Modifier.height(8.dp))
                    OutlinedButton(onClick = {
                        secureStorage.clear()
                        signedIn = false
                    }) {
                        Text("Sign out")
                    }
                } else {
                    OutlinedTextField(
                        value = email, onValueChange = { email = it },
                        label = { Text("Email") }, singleLine = true, modifier = Modifier.fillMaxWidth(),
                    )
                    Spacer(Modifier.height(8.dp))
                    OutlinedTextField(
                        value = password, onValueChange = { password = it },
                        label = { Text("Password") }, singleLine = true,
                        visualTransformation = androidx.compose.ui.text.input.PasswordVisualTransformation(),
                        modifier = Modifier.fillMaxWidth(),
                    )
                    Spacer(Modifier.height(8.dp))
                    Button(onClick = {
                        scope.launch {
                            try {
                                val client = ApiClient(backendUrl)
                                val result = client.login(email, password)
                                secureStorage.backendToken = result.accessToken
                                signedIn = true
                                status = "Signed in."
                                statusIsError = false
                            } catch (e: ApiException) {
                                status = e.message
                                statusIsError = true
                            } catch (e: Exception) {
                                status = "Couldn't reach the backend. Check the API URL."
                                statusIsError = true
                            }
                        }
                    }) {
                        Text("Sign in")
                    }
                }
                status?.let {
                    Spacer(Modifier.height(8.dp))
                    Text(it, color = if (statusIsError) Alert else Online, style = MaterialTheme.typography.bodySmall)
                }
            }

            Panel(title = "About") {
                Text("DeskBot Companion — Phase 4", color = Ink)
                Spacer(Modifier.height(4.dp))
                Text("com.deskbot.companion · 1.0.0", color = InkFaint, style = MaterialTheme.typography.bodySmall)
                Spacer(Modifier.height(8.dp))
                OutlinedButton(onClick = { navController.navigate(Routes.ABOUT) }) {
                    Text("More info")
                }
            }
        }
    }
}
