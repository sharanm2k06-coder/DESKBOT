package com.deskbot.companion.ui

import androidx.compose.runtime.Composable
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.deskbot.companion.ui.screens.*

object Routes {
    const val SPLASH = "splash"
    const val HOME = "home"
    const val DEVICES = "devices"
    const val MESSAGES = "messages"
    const val WHATSAPP = "whatsapp"
    const val SCHEDULES = "schedules"
    const val SETTINGS = "settings"
    const val ABOUT = "about"
}

@Composable
fun AppNavHost(navController: NavHostController = rememberNavController()) {
    NavHost(navController = navController, startDestination = Routes.SPLASH) {
        composable(Routes.SPLASH) { SplashScreen(navController) }
        composable(Routes.HOME) { HomeScreen(navController) }
        composable(Routes.DEVICES) { DevicesScreen(navController) }
        composable(Routes.MESSAGES) { MessagesScreen(navController) }
        composable(Routes.WHATSAPP) { WhatsAppScreen(navController) }
        composable(Routes.SCHEDULES) { SchedulesScreen(navController) }
        composable(Routes.SETTINGS) { SettingsScreen(navController) }
        composable(Routes.ABOUT) { AboutScreen(navController) }
    }
}
