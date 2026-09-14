package com.deskbot.companion.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable

private val DeskBotColorScheme = darkColorScheme(
    background = Void,
    surface = Panel,
    surfaceVariant = PanelRaised,
    primary = Signal,
    onPrimary = Void,
    onBackground = Ink,
    onSurface = Ink,
    outline = Line,
    error = Alert,
)

@Composable
fun DeskBotTheme(
    darkTheme: Boolean = isSystemInDarkTheme(), // console is dark-only in practice; parameter kept for API clarity
    content: @Composable () -> Unit,
) {
    MaterialTheme(
        colorScheme = DeskBotColorScheme,
        typography = DeskBotTypography,
        content = content,
    )
}
