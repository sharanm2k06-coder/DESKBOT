from datetime import datetime, timedelta, timezone

from app.database import SessionLocal
from app.services.scheduler_service import run_due_schedules


def test_create_and_list_schedule(client, auth_headers, registered_device):
    device_id = registered_device["id"]
    future = (datetime.now(timezone.utc) + timedelta(hours=1)).isoformat()
    r = client.post(
        "/api/v1/schedules",
        json={"device_id": device_id, "message": "Good morning", "scheduled_at": future, "repeat_rule": "DAILY"},
        headers=auth_headers,
    )
    assert r.status_code == 201

    r2 = client.get("/api/v1/schedules", headers=auth_headers)
    assert len(r2.json()) == 1
    assert r2.json()[0]["repeat_rule"] == "DAILY"


def test_delete_schedule(client, auth_headers, registered_device):
    device_id = registered_device["id"]
    future = (datetime.now(timezone.utc) + timedelta(hours=1)).isoformat()
    created = client.post(
        "/api/v1/schedules",
        json={"device_id": device_id, "message": "Once only", "scheduled_at": future, "repeat_rule": "ONCE"},
        headers=auth_headers,
    ).json()

    r = client.delete(f"/api/v1/schedules/{created['id']}", headers=auth_headers)
    assert r.status_code == 204

    r2 = client.get("/api/v1/schedules", headers=auth_headers)
    assert len(r2.json()) == 0


def test_due_schedule_fires_into_pending_message(client, auth_headers, registered_device):
    device_id = registered_device["id"]
    device_key = registered_device["device_key"]
    past = (datetime.now(timezone.utc) - timedelta(minutes=1)).isoformat()

    client.post(
        "/api/v1/schedules",
        json={"device_id": device_id, "message": "Overdue reminder", "scheduled_at": past, "repeat_rule": "ONCE"},
        headers=auth_headers,
    )

    db = SessionLocal()
    try:
        fired = run_due_schedules(db)
    finally:
        db.close()
    assert fired == 1

    pending = client.get(
        f"/api/v1/devices/{device_id}/messages/pending", headers={"X-Device-Key": device_key}
    ).json()
    assert pending is not None
    assert pending["body"] == "Overdue reminder"
