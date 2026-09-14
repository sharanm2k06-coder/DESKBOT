package com.deskbot.companion.ble

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Handler
import android.os.Looper
import com.deskbot.companion.data.model.ConnectionState
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withTimeoutOrNull
import java.util.UUID

sealed class SendResult {
    object Delivered : SendResult()
    object Failed : SendResult()
}

/**
 * Owns the whole BLE lifecycle: scan → connect → discover → subscribe →
 * send/receive → reconnect-with-backoff on drop. UI and the foreground
 * service both just observe `connectionState`/`incomingMessages` and call
 * `connect()`/`send()`/`disconnect()` — none of the GATT plumbing leaks out.
 *
 * Permission checks are NOT done here (Android throws SecurityException,
 * not a soft error, if you're missing BLUETOOTH_CONNECT/SCAN) — callers
 * (MainActivity / DeskBotForegroundService) must have already obtained
 * permissions before calling into this class. See ui/screens/DevicesScreen.kt
 * for the request flow.
 */
class BleManager(private val context: Context) {

    private val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val adapter: BluetoothAdapter? get() = bluetoothManager.adapter
    private val mainHandler = Handler(Looper.getMainLooper())

    private var gatt: BluetoothGatt? = null
    private var messageChar: BluetoothGattCharacteristic? = null
    private var commandChar: BluetoothGattCharacteristic? = null
    private var statusChar: BluetoothGattCharacteristic? = null

    private val reassembler = BleProtocol.Reassembler()
    private var pendingAck: CompletableDeferred<Boolean>? = null
    private var pendingAckMessageId: Long = -1

    private var autoReconnectEnabled = false
    private var reconnectAttempt = 0
    private var lastKnownAddress: String? = null
    private val backoffScheduleMs = longArrayOf(1_000, 2_000, 5_000, 10_000, 30_000)

    private val _connectionState = MutableStateFlow(ConnectionState.DISCONNECTED)
    val connectionState: StateFlow<ConnectionState> = _connectionState.asStateFlow()

    private val _scanResults = MutableStateFlow<List<BleScanResult>>(emptyList())
    val scanResults: StateFlow<List<BleScanResult>> = _scanResults.asStateFlow()

    /** Fully reassembled (type, payload) pairs arriving on the Status characteristic. */
    private val _incoming = MutableSharedFlow<Pair<BleProtocol.MessageType, ByteArray>>(extraBufferCapacity = 16)
    val incoming = _incoming.asSharedFlow()

    // --- Scanning ------------------------------------------------------------

    @SuppressLint("MissingPermission")
    fun startScan() {
        val scanner = adapter?.bluetoothLeScanner ?: return
        _scanResults.value = emptyList()
        _connectionState.value = ConnectionState.SCANNING
        val filters = listOf(
            android.bluetooth.le.ScanFilter.Builder()
                .setServiceUuid(android.os.ParcelUuid(DeskBotGatt.SERVICE_UUID))
                .build()
        )
        val settings = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
        scanner.startScan(filters, settings, scanCallback)
        mainHandler.postDelayed({ stopScan() }, 10_000)
    }

    @SuppressLint("MissingPermission")
    fun stopScan() {
        adapter?.bluetoothLeScanner?.stopScan(scanCallback)
        if (_connectionState.value == ConnectionState.SCANNING) _connectionState.value = ConnectionState.DISCONNECTED
    }

    private val scanCallback = object : ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val entry = BleScanResult(result.device, result.device.name, result.device.address, result.rssi)
            _scanResults.value = (_scanResults.value.filterNot { it.address == entry.address } + entry)
        }

        override fun onScanFailed(errorCode: Int) {
            _connectionState.value = ConnectionState.DISCONNECTED
        }
    }

    // --- Connecting ------------------------------------------------------------

    @SuppressLint("MissingPermission")
    fun connect(address: String, autoReconnect: Boolean = true) {
        stopScan()
        autoReconnectEnabled = autoReconnect
        lastKnownAddress = address
        reconnectAttempt = 0
        val device = adapter?.getRemoteDevice(address) ?: return
        _connectionState.value = ConnectionState.CONNECTING
        gatt = device.connectGatt(context, false, gattCallback)
    }

    @SuppressLint("MissingPermission")
    fun disconnect() {
        autoReconnectEnabled = false
        mainHandler.removeCallbacksAndMessages(RECONNECT_TOKEN)
        gatt?.disconnect()
        gatt?.close()
        gatt = null
        reassembler.reset()
        _connectionState.value = ConnectionState.DISCONNECTED
    }

    private fun scheduleReconnect() {
        if (!autoReconnectEnabled) return
        val delay = backoffScheduleMs[minOf(reconnectAttempt, backoffScheduleMs.lastIndex)]
        reconnectAttempt++
        mainHandler.postDelayed({
            lastKnownAddress?.let { connect(it, autoReconnect = true) }
        }, RECONNECT_TOKEN, delay)
    }

    @SuppressLint("MissingPermission")
    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    reconnectAttempt = 0
                    g.discoverServices()
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    _connectionState.value = ConnectionState.DISCONNECTED
                    reassembler.reset()
                    g.close()
                    if (gatt === g) gatt = null
                    scheduleReconnect()
                }
            }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            val service = g.getService(DeskBotGatt.SERVICE_UUID)
            messageChar = service?.getCharacteristic(DeskBotGatt.CHAR_MESSAGE)
            commandChar = service?.getCharacteristic(DeskBotGatt.CHAR_COMMAND)
            statusChar = service?.getCharacteristic(DeskBotGatt.CHAR_STATUS)

            statusChar?.let { char ->
                g.setCharacteristicNotification(char, true)
                val cccd = char.getDescriptor(DeskBotGatt.CCCD_UUID)
                cccd?.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                cccd?.let { g.writeDescriptor(it) }
            }

            _connectionState.value = ConnectionState.CONNECTED
        }

        override fun onCharacteristicChanged(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            val raw = characteristic.value ?: return
            when (val result = BleProtocol.parse(raw)) {
                is BleProtocol.ParseResult.Success -> {
                    val complete = reassembler.accept(result.packet) ?: return
                    if (complete.first == BleProtocol.MessageType.ACK && result.packet.messageId == pendingAckMessageId) {
                        pendingAck?.complete(true)
                    } else {
                        _incoming.tryEmit(complete)
                    }
                }
                else -> { /* malformed/unsupported fragment — silently dropped per protocol doc */ }
            }
        }
    }

    // --- Sending -----------------------------------------------------------------

    /**
     * Sends a full (possibly multi-fragment) message on the Message
     * characteristic and waits for the matching ACK, retrying once on
     * timeout, per docs/BLE_PROTOCOL.md's "Acknowledgement & retry" section.
     */
    @SuppressLint("MissingPermission")
    suspend fun sendMessage(type: BleProtocol.MessageType, payload: ByteArray): SendResult {
        val char = messageChar ?: return SendResult.Failed
        val messageId = BleProtocol.randomMessageId()
        val fragments = BleProtocol.fragment(type, payload, messageId)

        repeat(2) { attempt -> // one send + one retry
            pendingAckMessageId = messageId
            val deferred = CompletableDeferred<Boolean>()
            pendingAck = deferred

            for (fragment in fragments) {
                char.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT // Write Request, per protocol doc
                char.value = fragment
                gatt?.writeCharacteristic(char)
            }

            val acked = withTimeoutOrNull(3_000) { deferred.await() } ?: false
            if (acked) return SendResult.Delivered
        }
        return SendResult.Failed
    }

    @SuppressLint("MissingPermission")
    fun sendCommand(cmdJson: String) {
        val char = commandChar ?: return
        val fragments = BleProtocol.fragment(BleProtocol.MessageType.COMMAND, cmdJson.toByteArray())
        for (fragment in fragments) {
            char.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            char.value = fragment
            gatt?.writeCharacteristic(char)
        }
    }

    companion object {
        private const val RECONNECT_TOKEN = "deskbot_reconnect"
    }
}
