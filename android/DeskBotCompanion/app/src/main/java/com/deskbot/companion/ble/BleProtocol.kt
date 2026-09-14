package com.deskbot.companion.ble

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.ConcurrentHashMap
import kotlin.random.Random

/**
 * Implements the packet format documented in docs/BLE_PROTOCOL.md. Keep
 * this file and that document in sync — it is the single source of truth
 * and this is its one implementation on the Android side.
 */
object BleProtocol {

    const val PROTOCOL_VERSION: Int = 1
    const val MAX_PACKET_SIZE: Int = 180
    const val HEADER_SIZE: Int = 10
    const val CRC_SIZE: Int = 2
    const val MAX_PAYLOAD_PER_FRAGMENT: Int = MAX_PACKET_SIZE - HEADER_SIZE - CRC_SIZE // 168

    enum class MessageType(val value: Int) {
        MESSAGE(1),
        ACK(2),
        COMMAND(3),
        PING(4),
        STATUS_UPDATE(5);

        companion object {
            fun fromValue(v: Int): MessageType? = entries.find { it.value == v }
        }
    }

    data class Packet(
        val version: Int,
        val type: MessageType,
        val messageId: Long,
        val sequence: Int,
        val total: Int,
        val payload: ByteArray,
    )

    sealed class ParseResult {
        data class Success(val packet: Packet) : ParseResult()
        object TooShort : ParseResult()
        object CrcMismatch : ParseResult()
        data class UnsupportedVersion(val version: Int) : ParseResult()
        data class UnknownType(val type: Int) : ParseResult()
    }

    /** CRC-16/CCITT-FALSE — poly 0x1021, init 0xFFFF, no reflection, no final XOR. */
    fun crc16(data: ByteArray, length: Int = data.size): Int {
        var crc = 0xFFFF
        for (i in 0 until length) {
            crc = crc xor ((data[i].toInt() and 0xFF) shl 8)
            repeat(8) {
                crc = if (crc and 0x8000 != 0) (crc shl 1) xor 0x1021 else crc shl 1
                crc = crc and 0xFFFF
            }
        }
        return crc
    }

    /** Splits one logical message into 1+ wire fragments, all sharing a fresh message_id. */
    fun fragment(type: MessageType, payload: ByteArray, messageId: Long = randomMessageId()): List<ByteArray> {
        val chunks = if (payload.isEmpty()) listOf(ByteArray(0)) else payload.toList().chunked(MAX_PAYLOAD_PER_FRAGMENT).map { it.toByteArray() }
        val total = chunks.size
        return chunks.mapIndexed { index, chunk -> buildFragment(type, messageId, index, total, chunk) }
    }

    private fun buildFragment(type: MessageType, messageId: Long, sequence: Int, total: Int, payload: ByteArray): ByteArray {
        val buffer = ByteBuffer.allocate(HEADER_SIZE + payload.size + CRC_SIZE).order(ByteOrder.LITTLE_ENDIAN)
        buffer.put(PROTOCOL_VERSION.toByte())
        buffer.put(type.value.toByte())
        buffer.putInt(messageId.toInt()) // wire format is 32-bit; caller keeps ids within range (see randomMessageId)
        buffer.put(sequence.toByte())
        buffer.put(total.toByte())
        buffer.putShort(payload.size.toShort())
        buffer.put(payload)

        val bytesSoFar = buffer.array().copyOfRange(0, HEADER_SIZE + payload.size)
        val crc = crc16(bytesSoFar)
        buffer.putShort(crc.toShort())
        return buffer.array()
    }

    fun parse(raw: ByteArray): ParseResult {
        if (raw.size < HEADER_SIZE + CRC_SIZE) return ParseResult.TooShort
        val buffer = ByteBuffer.wrap(raw).order(ByteOrder.LITTLE_ENDIAN)

        val version = buffer.get().toInt() and 0xFF
        if (version != PROTOCOL_VERSION) return ParseResult.UnsupportedVersion(version)

        val typeByte = buffer.get().toInt() and 0xFF
        val type = MessageType.fromValue(typeByte) ?: return ParseResult.UnknownType(typeByte)

        val messageId = buffer.int.toLong() and 0xFFFFFFFFL
        val sequence = buffer.get().toInt() and 0xFF
        val total = buffer.get().toInt() and 0xFF
        val payloadLength = buffer.short.toInt() and 0xFFFF

        if (raw.size < HEADER_SIZE + payloadLength + CRC_SIZE) return ParseResult.TooShort
        val payload = ByteArray(payloadLength)
        buffer.get(payload)

        val expectedCrc = buffer.short.toInt() and 0xFFFF
        val actualCrc = crc16(raw, HEADER_SIZE + payloadLength)
        if (expectedCrc != actualCrc) return ParseResult.CrcMismatch

        return ParseResult.Success(Packet(version, type, messageId, sequence, total, payload))
    }

    fun randomMessageId(): Long = Random.nextInt(1, Int.MAX_VALUE).toLong()

    /**
     * Buffers incoming fragments by message_id and reassembles once `total`
     * fragments for that id have arrived. One instance per BLE connection
     * (state resets on reconnect, which is correct — the ESP32 does the same).
     */
    class Reassembler {
        private data class Pending(val total: Int, val type: MessageType, val fragments: MutableMap<Int, ByteArray>)

        private val pending = ConcurrentHashMap<Long, Pending>()

        /** Returns the reassembled (type, payload) once complete, else null. */
        fun accept(packet: Packet): Pair<MessageType, ByteArray>? {
            if (packet.total <= 1) return packet.type to packet.payload

            val entry = pending.getOrPut(packet.messageId) {
                Pending(packet.total, packet.type, ConcurrentHashMap<Int, ByteArray>())
            }
            entry.fragments[packet.sequence] = packet.payload

            if (entry.fragments.size < entry.total) return null

            pending.remove(packet.messageId)
            val ordered = (0 until entry.total).map { entry.fragments[it] ?: return null }
            val combined = ordered.fold(ByteArray(0)) { acc, chunk -> acc + chunk }
            return entry.type to combined
        }

        /** Drops any partially-received messages — call on disconnect. */
        fun reset() = pending.clear()
    }
}
