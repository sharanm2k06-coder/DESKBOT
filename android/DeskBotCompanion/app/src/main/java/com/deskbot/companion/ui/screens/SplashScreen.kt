package com.deskbot.companion.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.navigation.NavHostController
import com.deskbot.companion.ui.Routes
import com.deskbot.companion.ui.theme.InkDim
import com.deskbot.companion.ui.theme.Signal
import com.deskbot.companion.ui.theme.Void
import kotlinx.coroutines.delay

@Composable
fun SplashScreen(navController: NavHostController) {
    LaunchedEffect(Unit) {
        delay(700)
        navController.navigate(Routes.HOME) { popUpTo(Routes.SPLASH) { inclusive = true } }
    }

    Column(
        Modifier.fillMaxSize().background(Void),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Box(Modifier.size(10.dp).background(Signal, CircleShape))
        Spacer(Modifier.height(12.dp))
        Text("deskbot", style = MaterialTheme.typography.headlineSmall)
        Spacer(Modifier.height(4.dp))
        Text("companion", style = MaterialTheme.typography.bodySmall, color = InkDim)
    }
}
