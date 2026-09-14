import enum
import secrets
import uuid
from datetime import datetime, timezone

from sqlalchemy import Column, DateTime, Enum, ForeignKey, String
from sqlalchemy.orm import relationship

from app.database import Base


def _uuid() -> str:
    return str(uuid.uuid4())


def generate_device_key() -> str:
    """URL-safe secret handed to the ESP32/Android at registration time.

    This is the device's bearer credential for /devices/{id}/heartbeat and
    /devices/{id}/messages/pending — deliberately separate from the owning
    user's JWT so firmware never needs to hold a user password or long-lived
    user token.
    """
    return secrets.token_urlsafe(30)


class DeviceStatus(str, enum.Enum):
    ONLINE = "ONLINE"
    OFFLINE = "OFFLINE"
    UNKNOWN = "UNKNOWN"


class Device(Base):
    __tablename__ = "devices"

    id = Column(String(36), primary_key=True, default=_uuid)
    owner_id = Column(String(36), ForeignKey("users.id", ondelete="CASCADE"), nullable=False)

    device_uid = Column(String(64), unique=True, nullable=False, index=True)  # e.g. "DESKBOT-01"
    name = Column(String(100), nullable=False, default="DeskBot")
    device_key_hash = Column(String(255), nullable=False)  # hashed, never returned after creation

    status = Column(Enum(DeviceStatus), default=DeviceStatus.UNKNOWN, nullable=False)
    firmware_version = Column(String(20), nullable=True)
    last_seen = Column(DateTime(timezone=True), nullable=True)
    created_at = Column(DateTime(timezone=True), default=lambda: datetime.now(timezone.utc))

    owner = relationship("User", back_populates="devices")
    messages = relationship("Message", back_populates="device", cascade="all, delete-orphan")
    schedules = relationship("ScheduledMessage", back_populates="device", cascade="all, delete-orphan")
    events = relationship("DeviceEvent", back_populates="device", cascade="all, delete-orphan")
