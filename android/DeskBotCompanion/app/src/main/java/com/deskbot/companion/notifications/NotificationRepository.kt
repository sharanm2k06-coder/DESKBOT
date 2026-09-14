package com.deskbot.companion.notifications

import com.deskbot.companion.data.model.NotificationMessage
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow

/**
 * A process-wide event bus between DeskBotNotificationListenerService
 * (which can't easily hold app-scoped dependencies like BleManager — it's
 * instantiated by the system) and DeskBotForegroundService (which owns the
 * BLE connection and actually forwards messages).
 *
 * Deliberately in-memory only, `extraBufferCapacity` small — this is a
 * live hand-off, not a durable queue. If nothing is collecting when a
 * notification arrives (e.g. the foreground service isn't running because
 * the user disabled "Keep DeskBot connected"), the notification is dropped
 * rather than persisted, consistent with "do not persist WhatsApp content"
 * (master spec §43).
 */
object NotificationRepository {
    private val _messages = MutableSharedFlow<NotificationMessage>(extraBufferCapacity = 8)
    val messages: SharedFlow<NotificationMessage> = _messages

    fun emit(message: NotificationMessage) {
        _messages.tryEmit(message)
    }
}
