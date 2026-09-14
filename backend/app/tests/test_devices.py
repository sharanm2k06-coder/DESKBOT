def test_register_device(client, auth_headers):
    r = client.post(
        "/api/v1/devices/register",
        json={"device_uid": "DESKBOT-01", "name": "Desk"},
        headers=auth_headers,
    )
    assert r.status_code == 201
    body = r.json()
    assert body["device_uid"] == "DESKBOT-01"
    assert "device_key" in body and len(body["device_key"]) > 10


def test_duplicate_device_uid_rejected(client, auth_headers):
    client.post("/api/v1/devices/register", json={"device_uid": "DUP-01"}, headers=auth_headers)
    r = client.post("/api/v1/devices/register", json={"device_uid": "DUP-01"}, headers=auth_headers)
    assert r.status_code == 409


def test_list_devices(client, auth_headers, registered_device):
    r = client.get("/api/v1/devices", headers=auth_headers)
    assert r.status_code == 200
    assert len(r.json()) == 1
    assert r.json()[0]["device_uid"] == "DESKBOT-01"


def test_heartbeat_with_valid_key(client, registered_device):
    device_id = registered_device["id"]
    key = registered_device["device_key"]
    r = client.post(
        f"/api/v1/devices/{device_id}/heartbeat",
        json={"firmware_version": "1.0.0", "wifi_connected": True},
        headers={"X-Device-Key": key},
    )
    assert r.status_code == 200
    assert r.json()["status"] == "ok"


def test_heartbeat_with_invalid_key_rejected(client, registered_device):
    device_id = registered_device["id"]
    r = client.post(
        f"/api/v1/devices/{device_id}/heartbeat",
        json={"firmware_version": "1.0.0"},
        headers={"X-Device-Key": "wrong-key"},
    )
    assert r.status_code == 401


def test_device_status_online_after_heartbeat(client, auth_headers, registered_device):
    device_id = registered_device["id"]
    key = registered_device["device_key"]
    client.post(f"/api/v1/devices/{device_id}/heartbeat", json={}, headers={"X-Device-Key": key})
    r = client.get(f"/api/v1/devices/{device_id}", headers=auth_headers)
    assert r.json()["status"] == "ONLINE"
