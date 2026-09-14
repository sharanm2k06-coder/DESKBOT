package com.deskbot.companion.network

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONObject
import java.util.concurrent.TimeUnit

/**
 * Talks to the same backend the web frontend uses (see
 * ../../backend/README.md and docs/API_DOCUMENTATION.md). The Android app
 * only needs a slice of that API: login (to associate the phone with a
 * user account) and device registration/listing — sending messages is done
 * from the web app, not this app; this app's job is WhatsApp forwarding +
 * BLE, per master spec §11/§68.
 */
class ApiClient(private val baseUrl: String) {

    private val client = OkHttpClient.Builder()
        .connectTimeout(10, TimeUnit.SECONDS)
        .readTimeout(10, TimeUnit.SECONDS)
        .build()

    private val jsonMedia = "application/json".toMediaType()

    var token: String? = null

    suspend fun login(email: String, password: String): LoginResult = withContext(Dispatchers.IO) {
        val body = JSONObject().put("email", email).put("password", password).toString()
        val request = Request.Builder()
            .url("$baseUrl/api/v1/auth/login")
            .post(body.toRequestBody(jsonMedia))
            .build()
        val json = execute(request)
        LoginResult(accessToken = json.getString("access_token"))
    }

    suspend fun registerDevice(deviceUid: String, name: String?): DeviceRegisterResult = withContext(Dispatchers.IO) {
        val body = JSONObject().apply {
            put("device_uid", deviceUid)
            if (name != null) put("name", name)
        }.toString()
        val request = Request.Builder()
            .url("$baseUrl/api/v1/devices/register")
            .post(body.toRequestBody(jsonMedia))
            .authed()
            .build()
        val json = execute(request)
        DeviceRegisterResult(
            id = json.getString("id"),
            deviceUid = json.getString("device_uid"),
            name = json.getString("name"),
            deviceKey = json.getString("device_key"),
        )
    }

    suspend fun listDevices(): List<BackendDevice> = withContext(Dispatchers.IO) {
        val request = Request.Builder()
            .url("$baseUrl/api/v1/devices")
            .get()
            .authed()
            .build()
        val text = executeRaw(request)
        val array = org.json.JSONArray(text)
        (0 until array.length()).map { i ->
            val obj = array.getJSONObject(i)
            BackendDevice(
                id = obj.getString("id"),
                deviceUid = obj.getString("device_uid"),
                name = obj.getString("name"),
                status = obj.getString("status"),
            )
        }
    }

    private fun Request.Builder.authed(): Request.Builder =
        token?.let { header("Authorization", "Bearer $it") } ?: this

    private fun execute(request: Request): JSONObject = JSONObject(executeRaw(request))

    private fun executeRaw(request: Request): String {
        client.newCall(request).execute().use { response ->
            val text = response.body?.string().orEmpty()
            if (!response.isSuccessful) {
                val detail = runCatching { JSONObject(text).optString("detail") }.getOrNull()
                throw ApiException(detail ?: response.message, response.code)
            }
            return text
        }
    }
}
