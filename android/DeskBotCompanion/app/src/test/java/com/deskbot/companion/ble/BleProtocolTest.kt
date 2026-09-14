package com.deskbot.companion.ble

import org.junit.Assert.*
import org.junit.Test

class BleProtocolTest {

    @Test
    fun `short message fits in a single fragment`() {
        val payload = "hello deskbot".toByteArray()
        val fragments = BleProtocol.fragment(BleProtocol.MessageType.MESSAGE, payload, messageId = 42)
        assertEquals(1, fragments.size)

        val parsed = BleProtocol.parse(fragments[0])
        assertTrue(parsed is BleProtocol.ParseResult.Success)
        val packet = (parsed as BleProtocol.ParseResult.Success).packet
        assertEquals(0, packet.sequence)
        assertEquals(1, packet.total)
        assertEquals(42L, packet.messageId)
        assertArrayEquals(payload, packet.payload)
    }

    @Test
    fun `long message is split into multiple fragments`() {
        val payload = "x".repeat(500).toByteArray()
        val fragments = BleProtocol.fragment(BleProtocol.MessageType.MESSAGE, payload, messageId = 7)

        val expectedFragments = (payload.size + BleProtocol.MAX_PAYLOAD_PER_FRAGMENT - 1) / BleProtocol.MAX_PAYLOAD_PER_FRAGMENT
        assertEquals(expectedFragments, fragments.size)
        fragments.forEach { assertTrue(it.size <= BleProtocol.MAX_PACKET_SIZE) }
    }

    @Test
    fun `reassembler reconstructs original payload from fragments in order`() {
        val payload = "y".repeat(400).toByteArray()
        val fragments = BleProtocol.fragment(BleProtocol.MessageType.MESSAGE, payload, messageId = 99)
        val reassembler = BleProtocol.Reassembler()

        var result: Pair<BleProtocol.MessageType, ByteArray>? = null
        for (raw in fragments) {
            val parsed = BleProtocol.parse(raw) as BleProtocol.ParseResult.Success
            result = reassembler.accept(parsed.packet) ?: result
        }

        assertNotNull(result)
        assertEquals(BleProtocol.MessageType.MESSAGE, result!!.first)
        assertArrayEquals(payload, result.second)
    }

    @Test
    fun `reassembler handles out-of-order fragments`() {
        val payload = "z".repeat(400).toByteArray()
        val fragments = BleProtocol.fragment(BleProtocol.MessageType.MESSAGE, payload, messageId = 5)
        val reassembler = BleProtocol.Reassembler()

        val shuffled = fragments.reversed()
        var result: Pair<BleProtocol.MessageType, ByteArray>? = null
        for (raw in shuffled) {
            val parsed = BleProtocol.parse(raw) as BleProtocol.ParseResult.Success
            result = reassembler.accept(parsed.packet) ?: result
        }

        assertNotNull(result)
        assertArrayEquals(payload, result!!.second)
    }

    @Test
    fun `corrupted payload fails CRC check`() {
        val fragments = BleProtocol.fragment(BleProtocol.MessageType.PING, ByteArray(0), messageId = 1)
        val corrupted = fragments[0].copyOf()
        corrupted[corrupted.size - 3] = (corrupted[corrupted.size - 3] + 1).toByte() // flip a payload/header byte

        val result = BleProtocol.parse(corrupted)
        assertTrue(result is BleProtocol.ParseResult.CrcMismatch || result is BleProtocol.ParseResult.TooShort)
    }

    @Test
    fun `unsupported version is rejected`() {
        val fragments = BleProtocol.fragment(BleProtocol.MessageType.PING, ByteArray(0), messageId = 1)
        val tampered = fragments[0].copyOf()
        tampered[0] = 99 // bogus version byte

        val result = BleProtocol.parse(tampered)
        assertTrue(result is BleProtocol.ParseResult.UnsupportedVersion)
    }

    @Test
    fun `packet too short to contain a header is rejected`() {
        val result = BleProtocol.parse(ByteArray(4))
        assertEquals(BleProtocol.ParseResult.TooShort, result)
    }

    @Test
    fun `crc16 is deterministic for the same input`() {
        val data = "deterministic".toByteArray()
        assertEquals(BleProtocol.crc16(data), BleProtocol.crc16(data))
    }
}
