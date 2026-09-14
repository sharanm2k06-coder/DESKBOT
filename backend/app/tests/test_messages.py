def test_create_and_list_message(client, auth_headers, registered_device):
    device_id = registered_device["id"]
    r = client.post(
        "/api/v1/messages",
        json={"device_id": device_id, "body": "Hello DeskBot", "priority": "NORMAL"},
        headers=auth_headers,
    )
    assert r.status_code == 201
    assert r.json()["status"] == "PENDING"

    r2 = client.get("/api/v1/messages", headers=auth_headers)
    assert len(r2.json()) == 1


def test_create_message_for_unowned_device_rejected(client, auth_headers):
    r = client.post(
        "/api/v1/messages",
        json={"device_id": "does-not-exist", "body": "hi"},
        headers=auth_headers,
    )
    assert r.status_code == 404


def test_device_polls_and_acks_pending_message(client, auth_headers, registered_device):
    device_id = registered_device["id"]
    device_key = registered_device["device_key"]

    client.post(
        "/api/v1/messages",
        json={"device_id": device_id, "body": "Meeting at 3pm", "priority": "IMPORTANT"},
        headers=auth_headers,
    )

    r = client.get(f"/api/v1/devices/{device_id}/messages/pending", headers={"X-Device-Key": device_key})
    assert r.status_code == 200
    msg = r.json()
    assert msg is not None
    assert msg["body"] == "Meeting at 3pm"
    assert msg["status"] == "DELIVERED"

    # No second message pending.
    r2 = client.get(f"/api/v1/devices/{device_id}/messages/pending", headers={"X-Device-Key": device_key})
    assert r2.json() is None

    ack = client.post(
        f"/api/v1/messages/{msg['id']}/ack",
        json={},
        headers={"X-Device-Id": device_id, "X-Device-Key": device_key},
    )
    assert ack.status_code == 200
    assert ack.json()["status"] == "ACKED"


def test_pending_endpoint_rejects_wrong_key(client, registered_device):
    device_id = registered_device["id"]
    r = client.get(f"/api/v1/devices/{device_id}/messages/pending", headers={"X-Device-Key": "nope"})
    assert r.status_code == 401


def test_mark_message_read(client, auth_headers, registered_device):
    device_id = registered_device["id"]
    create = client.post(
        "/api/v1/messages", json={"device_id": device_id, "body": "hi"}, headers=auth_headers
    )
    message_id = create.json()["id"]
    r = client.post(f"/api/v1/messages/{message_id}/read", headers=auth_headers)
    assert r.status_code == 200
    assert r.json()["status"] == "READ"
