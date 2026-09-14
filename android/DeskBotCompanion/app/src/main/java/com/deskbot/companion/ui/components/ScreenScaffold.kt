package com.deskbot.companion.ui.components

import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.Chat
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.navigation.NavHostController
import com.deskbot.companion.ui.Routes
import com.deskbot.companion.ui.theme.Ink
import com.deskbot.companion.ui.theme.InkFaint
import com.deskbot.companion.ui.theme.Panel
import com.deskbot.companion.ui.theme.Void

private data class NavItem(val route: String, val label: String, val icon: androidx.compose.ui.graphics.vector.ImageVector)

private val BOTTOM_NAV_ITEMS = listOf(
    NavItem(Routes.HOME, "Home", Icons.Filled.Home),
    NavItem(Routes.MESSAGES, "Messages", Icons.AutoMirrored.Filled.Chat),
    NavItem(Routes.DEVICES, "Device", Icons.Filled.Bluetooth),
    NavItem(Routes.SCHEDULES, "Schedule", Icons.Filled.Schedule),
    NavItem(Routes.SETTINGS, "Settings", Icons.Filled.Settings),
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ScreenScaffold(
    navController: NavHostController,
    title: String,
    currentRoute: String,
    content: @Composable (Modifier) -> Unit,
) {
    Scaffold(
        containerColor = Void,
        topBar = {
            TopAppBar(
                title = { Text(title, color = Ink) },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = Void),
            )
        },
        bottomBar = {
            NavigationBar(containerColor = Panel) {
                BOTTOM_NAV_ITEMS.forEach { item ->
                    NavigationBarItem(
                        selected = currentRoute == item.route,
                        onClick = {
                            if (currentRoute != item.route) {
                                navController.navigate(item.route) {
                                    popUpTo(Routes.HOME)
                                    launchSingleTop = true
                                }
                            }
                        },
                        icon = { Icon(item.icon, contentDescription = item.label) },
                        label = { Text(item.label) },
                        colors = NavigationBarItemDefaults.colors(
                            selectedIconColor = Ink,
                            selectedTextColor = Ink,
                            unselectedIconColor = InkFaint,
                            unselectedTextColor = InkFaint,
                            indicatorColor = Panel,
                        ),
                    )
                }
            }
        },
    ) { padding ->
        content(Modifier.padding(padding))
    }
}
