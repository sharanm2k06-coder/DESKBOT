import enum
import uuid
from datetime import datetime, timezone

from sqlalchemy import Column, DateTime, Enum, ForeignKey, String, Text
from sqlalchemy.orm import relationship

from app.database import Base


def _uuid() -> str:
    return str(uuid.uuid4())


class MessageSource(str, enum.Enum):
    APP = "APP"
    WHATSAPP = "WHATSAPP"
    SYSTEM = "SYSTEM"
    SCHEDULED = "SCHEDULED"


class MessagePriority(str, enum.Enum):
    NORMAL = "NORMAL"
    IMPORTANT = "IMPORTANT"
    ALERT = "ALERT"


class MessageStatus(str, enum.Enum):
    PENDING = "PENDING"       # created, not yet delivered to device
    DELIVERED = "DELIVERED"   # device polled/received it
    ACKED = "ACKED"           # device confirmed display/queue-accept
    READ = "READ"             # user acknowledged on-device
    EXPIRED = "EXPIRED"


class Message(Base):
    """
    NOTE ON WHATSAPP PRIVACY (see docs/PRIVACY.md and spec §43):
    WhatsApp-sourced message bodies forwarded phone -> BLE -> ESP32 do NOT
    pass through this table or this backend at all in the Phase-1/4
    architecture — they go directly over BLE. This table exists for
    APP / SYSTEM / SCHEDULED messages sent via the web/API path. If a
    future version ever mirrors WhatsApp content here for history, that
    must be an explicit, user-controlled opt-in, off by default.
    """

    __tablename__ = "messages"

    id = Column(String(36), primary_key=True, default=_uuid)
    device_id = Column(String(36), ForeignKey("devices.id", ondelete="CASCADE"), nullable=False, index=True)

    source = Column(Enum(MessageSource), nullable=False, default=MessageSource.APP)
    sender = Column(String(100), nullable=True)
    title = Column(String(100), nullable=True)
    body = Column(Text, nullable=False)
    priority = Column(Enum(MessagePriority), nullable=False, default=MessagePriority.NORMAL)
    status = Column(Enum(MessageStatus), nullable=False, default=MessageStatus.PENDING)

    created_at = Column(DateTime(timezone=True), default=lambda: datetime.now(timezone.utc))
    delivered_at = Column(DateTime(timezone=True), nullable=True)
    read_at = Column(DateTime(timezone=True), nullable=True)
    expires_at = Column(DateTime(timezone=True), nullable=True)

    device = relationship("Device", back_populates="messages")
