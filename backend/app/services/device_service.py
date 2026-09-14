from datetime import datetime, timezone

from fastapi import HTTPException, status
from sqlalchemy.orm import Session

from app.models.device import Device, DeviceStatus, generate_device_key
from app.models.device_event import DeviceEvent
from app.models.user import User
from app.schemas.device import DeviceRegisterRequest, HeartbeatRequest
from app.security import hash_device_key

# A device is considered OFFLINE if we haven't heard a heartbeat in this long.
# Applied lazily (computed on read) rather than via a background sweep, to
# keep Phase-2 simple; a scheduler pass could mark rows explicitly later.
HEARTBEAT_STALE_SECONDS = 90


def register_device(db: Session, owner: User, payload: DeviceRegisterRequest) -> tuple[Device, str]:
    existing = db.query(Device).filter(Device.device_uid == payload.device_uid).first()
    if existing:
        raise HTTPException(status.HTTP_409_CONFLICT, "device_uid already registered")

    raw_key = generate_device_key()
    device = Device(
        owner_id=owner.id,
        device_uid=payload.device_uid,
        name=payload.name or payload.device_uid,
        device_key_hash=hash_device_key(raw_key),
        firmware_version=payload.firmware_version,
        status=DeviceStatus.UNKNOWN,
    )
    db.add(device)
    db.flush()
    db.add(DeviceEvent(device_id=device.id, event_type="REGISTERED"))
    db.commit()
    db.refresh(device)
    return device, raw_key


def record_heartbeat(db: Session, device: Device, payload: HeartbeatRequest) -> Device:
    now = datetime.now(timezone.utc)
    device.last_seen = now
    device.status = DeviceStatus.ONLINE
    if payload.firmware_version:
        device.firmware_version = payload.firmware_version

    detail_parts = []
    if payload.wifi_connected is not None:
        detail_parts.append(f"wifi={payload.wifi_connected}")
    if payload.ble_connected is not None:
        detail_parts.append(f"ble={payload.ble_connected}")
    if payload.temperature_c is not None:
        detail_parts.append(f"temp={payload.temperature_c}")
    if payload.humidity_pct is not None:
        detail_parts.append(f"hum={payload.humidity_pct}")

    db.add(DeviceEvent(device_id=device.id, event_type="HEARTBEAT", detail=", ".join(detail_parts) or None))
    db.commit()
    db.refresh(device)
    return device


def effective_status(device: Device) -> DeviceStatus:
    """Computes OFFLINE on read if the last heartbeat is stale, without
    mutating the row (a background job could do that later if needed)."""
    if device.status == DeviceStatus.ONLINE and device.last_seen:
        age = (datetime.now(timezone.utc) - device.last_seen).total_seconds()
        if age > HEARTBEAT_STALE_SECONDS:
            return DeviceStatus.OFFLINE
    return device.status
