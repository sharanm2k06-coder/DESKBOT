from datetime import datetime
from typing import Optional

from pydantic import BaseModel, Field

from app.models.device import DeviceStatus


class DeviceRegisterRequest(BaseModel):
    device_uid: str = Field(min_length=1, max_length=64, description='e.g. "DESKBOT-01"')
    name: Optional[str] = Field(default=None, max_length=100)
    firmware_version: Optional[str] = Field(default=None, max_length=20)


class DeviceRegisterResponse(BaseModel):
    id: str
    device_uid: str
    name: str
    device_key: str  # returned ONCE, at registration only — never again

    class Config:
        from_attributes = True


class DeviceOut(BaseModel):
    id: str
    device_uid: str
    name: str
    status: DeviceStatus
    firmware_version: Optional[str]
    last_seen: Optional[datetime]
    created_at: datetime

    class Config:
        from_attributes = True


class HeartbeatRequest(BaseModel):
    firmware_version: Optional[str] = Field(default=None, max_length=20)
    wifi_connected: Optional[bool] = None
    ble_connected: Optional[bool] = None
    temperature_c: Optional[float] = None
    humidity_pct: Optional[float] = None


class HeartbeatResponse(BaseModel):
    status: str = "ok"
    server_time: datetime
