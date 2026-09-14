from datetime import datetime, timezone
from typing import List

from fastapi import APIRouter, Depends, status
from sqlalchemy.orm import Session

from app.database import get_db
from app.deps import get_current_user, get_device_from_key, get_owned_device
from app.models.device import Device
from app.models.user import User
from app.schemas.device import (
    DeviceOut,
    DeviceRegisterRequest,
    DeviceRegisterResponse,
    HeartbeatRequest,
    HeartbeatResponse,
)
from app.services.device_service import effective_status, record_heartbeat, register_device

router = APIRouter(prefix="/api/v1/devices", tags=["devices"])


@router.post("/register", response_model=DeviceRegisterResponse, status_code=status.HTTP_201_CREATED)
def register(
    payload: DeviceRegisterRequest,
    db: Session = Depends(get_db),
    user: User = Depends(get_current_user),
):
    device, raw_key = register_device(db, user, payload)
    return DeviceRegisterResponse(
        id=device.id, device_uid=device.device_uid, name=device.name, device_key=raw_key
    )


@router.get("", response_model=List[DeviceOut])
def list_devices(db: Session = Depends(get_db), user: User = Depends(get_current_user)):
    devices = db.query(Device).filter(Device.owner_id == user.id).all()
    for d in devices:
        d.status = effective_status(d)  # display-only; not persisted
    return devices


@router.get("/{device_id}", response_model=DeviceOut)
def get_device(device: Device = Depends(get_owned_device)):
    device.status = effective_status(device)
    return device


@router.post("/{device_id}/heartbeat", response_model=HeartbeatResponse)
def heartbeat(
    payload: HeartbeatRequest,
    db: Session = Depends(get_db),
    device: Device = Depends(get_device_from_key),
):
    record_heartbeat(db, device, payload)
    return HeartbeatResponse(server_time=datetime.now(timezone.utc))
