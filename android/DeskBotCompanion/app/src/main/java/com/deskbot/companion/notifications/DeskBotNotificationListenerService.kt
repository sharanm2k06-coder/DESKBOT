package com.deskbot.companion.notifications

import android.app.Notification
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import com.deskbot.companion.data.model.MessageSource
import com.deskbot.companion.data.model.NotificationMessage

/**
 * Detects notifications from supported source apps (WhatsApp for Phase 4;
 * the package-name map below is where Telegram/Gmail/Instagram/SMS/Teams
 * would be added later, per master spec §11's extensibility note — no
 * other code needs to change to add a source, just this map and, if the
 * new source needs different field extraction, a branch in `normalize`).
 *
 * This service does the minimum: listen, filter, normalize, forward to the
 * repository. It does NOT touch BLE directly — NotificationRepository (via
 * DeskBotForegroundService) owns the "should we even forward right now"
 * decision (forwarding toggle, connection state), keeping this class a
 * pure translator from Android's notification model to ours.
 */
class DeskBotNotificationListenerService : NotificationListenerService() {

    override fun onListenerConnected() {
        super.onListenerConnected()
        instance = this
    }

    override fun onListenerDisconnected() {
        super.onListenerDisconnected()
        if (instance === this) instance = null
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) {
        val source = SUPPORTED_PACKAGES[sbn.packageName] ?: return

        // Summary notifications for a notification *group* (e.g. "3 new
        // messages") carry no useful single sender/body — skip them rather
        // than forwarding a useless placeholder (master spec §45: handle
        // "group notifications... summary notifications").
        if (sbn.notification.flags and Notification.FLAG_GROUP_SUMMARY != 0) return

        val normalized = normalize(sbn, source) ?: return
        NotificationRepository.emit(normalized)
    }

    private fun normalize(sbn: StatusBarNotification, source: MessageSource): NotificationMessage? {
        val extras = sbn.notification.extras
        val title = extras.getCharSequence(Notification.EXTRA_TITLE)?.toString()
        val text = extras.getCharSequence(Notification.EXTRA_TEXT)?.toString()
            ?: extras.getCharSequence(Notification.EXTRA_BIG_TEXT)?.toString()

        // WhatsApp puts the sender's name in EXTRA_TITLE (or EXTRA_CONVERSATION_TITLE
        // for group chats) and the message body in EXTRA_TEXT. If body is
        // missing entirely there's nothing worth showing — skip, don't crash
        // (master spec §45).
        val body = text?.takeUnless { it.isBlank() } ?: return null

        return NotificationMessage(
            source = source,
            sender = extras.getCharSequence(Notification.EXTRA_CONVERSATION_TITLE)?.toString() ?: title,
            title = "WhatsApp",
            body = body,
            timestamp = sbn.postTime,
        )
    }

    companion object {
        private val SUPPORTED_PACKAGES = mapOf(
            "com.whatsapp" to MessageSource.WHATSAPP,
            "com.whatsapp.w4b" to MessageSource.WHATSAPP, // WhatsApp Business
        )

        @Volatile
        private var instance: DeskBotNotificationListenerService? = null

        /** True once the user has actually granted notification-listener access. */
        fun isConnected(): Boolean = instance != null
    }
}
