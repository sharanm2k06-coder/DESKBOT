import uuid
from datetime import datetime, timezone

from sqlalchemy import Column, DateTime, ForeignKey, String, Text
from sqlalchemy.orm import relationship

from app.database import Base


def _uuid() -> str:
    return str(uuid.uuid4())


class DeviceEvent(Base):
    """
    Lightweight audit trail: heartbeats, registrations, connection state
    changes. Kept intentionally generic (event_type + optional detail text)
    so new event kinds don't require a migration.
    """

    __tablename__ = "device_events"

    id = Column(String(36), primary_key=True, default=_uuid)
    device_id = Column(String(36), ForeignKey("devices.id", ondelete="CASCADE"), nullable=False, index=True)

    event_type = Column(String(50), nullable=False)  # e.g. HEARTBEAT, REGISTERED, WIFI_ONLINE
    detail = Column(Text, nullable=True)
    created_at = Column(DateTime(timezone=True), default=lambda: datetime.now(timezone.utc))

    device = relationship("Device", back_populates="events")
