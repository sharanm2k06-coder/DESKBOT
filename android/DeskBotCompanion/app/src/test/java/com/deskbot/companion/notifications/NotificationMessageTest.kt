package com.deskbot.companion.notifications

import com.deskbot.companion.data.model.MessageSource
import com.deskbot.companion.data.model.NotificationMessage
import org.junit.Assert.*
import org.junit.Test

class NotificationMessageTest {

    @Test
    fun `round trips through JSON`() {
        val original = NotificationMessage(
            source = MessageSource.WHATSAPP,
            sender = "Rahul",
            title = "WhatsApp",
            body = "Are you coming to the meeting?",
            timestamp = 1732400000000,
        )

        val parsed = NotificationMessage.fromJson(original.toJson())

        assertNotNull(parsed)
        assertEquals(original, parsed)
    }

    @Test
    fun `missing body yields null instead of throwing`() {
        val json = """{"source":"WHATSAPP","sender":"Rahul","title":"WhatsApp","timestamp":123}"""
        assertNull(NotificationMessage.fromJson(json))
    }

    @Test
    fun `missing sender falls back to null, not a crash`() {
        val json = """{"source":"WHATSAPP","body":"hi","timestamp":123}"""
        val parsed = NotificationMessage.fromJson(json)
        assertNotNull(parsed)
        assertNull(parsed!!.sender)
        assertEquals("hi", parsed.body)
    }

    @Test
    fun `unknown source falls back to SYSTEM rather than crashing`() {
        val json = """{"source":"TELEGRAM","body":"hi","timestamp":123}"""
        val parsed = NotificationMessage.fromJson(json)
        assertNotNull(parsed)
        assertEquals(MessageSource.SYSTEM, parsed!!.source)
    }

    @Test
    fun `malformed json returns null`() {
        assertNull(NotificationMessage.fromJson("not json at all"))
    }

    @Test
    fun `blank body is treated as missing`() {
        val json = """{"source":"WHATSAPP","body":"   ","timestamp":123}"""
        assertNull(NotificationMessage.fromJson(json))
    }
}
