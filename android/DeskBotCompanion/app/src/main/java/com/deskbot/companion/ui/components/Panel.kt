package com.deskbot.companion.ui.components

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.deskbot.companion.ui.theme.*

@Composable
fun Panel(
    title: String? = null,
    modifier: Modifier = Modifier,
    content: @Composable ColumnScope.() -> Unit,
) {
    Column(
        modifier
            .fillMaxWidth()
            .background(Panel, RoundedCornerShape(6.dp))
            .border(1.dp, Line, RoundedCornerShape(6.dp))
            .padding(16.dp),
    ) {
        if (title != null) {
            Text(title, style = MaterialTheme.typography.labelSmall, color = InkFaint)
            Spacer(Modifier.height(10.dp))
        }
        content()
    }
}

enum class PillTone { ONLINE, OFFLINE, ALERT, IMPORTANT, NEUTRAL }

@Composable
fun StatusPill(text: String, tone: PillTone = PillTone.NEUTRAL) {
    val color = when (tone) {
        PillTone.ONLINE -> Online
        PillTone.OFFLINE -> InkFaint
        PillTone.ALERT -> Alert
        PillTone.IMPORTANT -> Important
        PillTone.NEUTRAL -> InkDim
    }
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier
            .border(BorderStroke(1.dp, color.copy(alpha = 0.45f)), RoundedCornerShape(3.dp))
            .padding(horizontal = 8.dp, vertical = 3.dp),
    ) {
        Box(Modifier.size(6.dp).background(color, CircleShape))
        Spacer(Modifier.width(6.dp))
        Text(text, style = MaterialTheme.typography.labelSmall, color = color)
    }
}
