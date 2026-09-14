package com.deskbot.companion.service

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import com.deskbot.companion.MainActivity
import com.deskbot.companion.R
import com.deskbot.companion.ble.BleProtocol
import com.deskbot.companion.ble.SendResult
import com.deskbot.companion.data.local.AppPreferences
import com.deskbot.companion.data.model.ConnectionState
import com.deskbot.companion.notifications.NotificationRepository
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch

/**
 * Runs while "Keep DeskBot connected" is on (master spec §46). Two jobs:
 *   1. Own the BleManager connection so it survives the app being
 *      backgrounded (a plain Activity-scoped connection would die).
 *   2. Collect NotificationRepository.messages and forward each one over
 *      BLE, *only* if WhatsApp forwarding is enabled — checked per-message
 *      so toggling the setting takes effect immediately, not just at
 *      service start.
 *
 * This is the one class that bridges "phone got a WhatsApp notification"
 * to "ESP32 displays it" — see docs/BLE_PROTOCOL.md for the wire format
 * used in step 2.
 */
class DeskBotForegroundService : Service() {

    private val job = SupervisorJob()
    private val scope = CoroutineScope(job)
    private lateinit var prefs: AppPreferences

    override fun onCreate() {
        super.onCreate()
        prefs = AppPreferences(applicationContext)
        createNotificationChannels()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        startForeground(NOTIF_ID_SERVICE, buildServiceNotification())

        val bleManager = BleManagerHolder.getOrCreate(applicationContext)
        val address = intent?.getStringExtra(EXTRA_DEVICE_ADDRESS)
        if (address != null) {
            bleManager.connect(address, autoReconnect = true)
        }

        scope.launch {
            NotificationRepository.messages.collectLatest { message ->
                val forwardingOn = prefs.whatsappForwardingEnabled.first()
                if (!forwardingOn) return@collectLatest
                if (bleManager.connectionState.first() != ConnectionState.CONNECTED) return@collectLatest

                val result = bleManager.sendMessage(BleProtocol.MessageType.MESSAGE, message.toJson().toByteArray())
                if (result is SendResult.Delivered) {
                    postDeliveredNotification(message.sender ?: "WhatsApp")
                }
            }
        }

        return START_STICKY
    }

    override fun onDestroy() {
        super.onDestroy()
        job.cancel()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun buildServiceNotification(): Notification {
        val openApp = PendingIntent.getActivity(
            this, 0, Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_IMMUTABLE,
        )
        return NotificationCompat.Builder(this, CHANNEL_SERVICE)
            .setContentTitle(getString(R.string.fg_service_title))
            .setContentText(getString(R.string.fg_service_text))
            .setSmallIcon(android.R.drawable.stat_sys_data_bluetooth)
            .setContentIntent(openApp)
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()
    }

    private fun postDeliveredNotification(sender: String) {
        val notification = NotificationCompat.Builder(this, CHANNEL_MESSAGES)
            .setContentTitle("Forwarded to DeskBot")
            .setContentText("Message from $sender shown on DeskBot")
            .setSmallIcon(android.R.drawable.stat_notify_sync)
            .setAutoCancel(true)
            .build()
        getSystemService(NotificationManager::class.java)?.notify(NOTIF_ID_MESSAGE, notification)
    }

    private fun createNotificationChannels() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return
        val manager = getSystemService(NotificationManager::class.java) ?: return
        manager.createNotificationChannel(
            NotificationChannel(CHANNEL_SERVICE, getString(R.string.notif_channel_service), NotificationManager.IMPORTANCE_LOW)
        )
        manager.createNotificationChannel(
            NotificationChannel(CHANNEL_MESSAGES, getString(R.string.notif_channel_messages), NotificationManager.IMPORTANCE_DEFAULT)
        )
    }

    companion object {
        const val EXTRA_DEVICE_ADDRESS = "device_address"
        private const val CHANNEL_SERVICE = "deskbot_service"
        private const val CHANNEL_MESSAGES = "deskbot_messages"
        private const val NOTIF_ID_SERVICE = 1
        private const val NOTIF_ID_MESSAGE = 2
    }
}
