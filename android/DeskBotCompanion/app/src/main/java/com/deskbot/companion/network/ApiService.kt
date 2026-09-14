package com.deskbot.companion.network

data class LoginResult(val accessToken: String)

data class DeviceRegisterResult(
    val id: String,
    val deviceUid: String,
    val name: String,
    val deviceKey: String,
)

data class BackendDevice(
    val id: String,
    val deviceUid: String,
    val name: String,
    val status: String,
)

class ApiException(message: String, val statusCode: Int) : Exception(message)
