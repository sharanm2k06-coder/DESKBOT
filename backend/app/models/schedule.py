import enum
import uuid
from datetime import datetime, timezone

from sqlalchemy import Boolean, Column, DateTime, Enum, ForeignKey, String, Text
from sqlalchemy.orm import relationship

from app.database import Base


def _uuid() -> str:
    return str(uuid.uuid4())


class RepeatRule(str, enum.Enum):
    ONCE = "ONCE"
    DAILY = "DAILY"
    WEEKDAYS = "WEEKDAYS"
    WEEKLY = "WEEKLY"


class ScheduledMessage(Base):
    __tablename__ = "scheduled_messages"

    id = Column(String(36), primary_key=True, default=_uuid)
    device_id = Column(String(36), ForeignKey("devices.id", ondelete="CASCADE"), nullable=False, index=True)

    message = Column(Text, nullable=False)
    scheduled_at = Column(DateTime(timezone=True), nullable=False)  # next/first fire time (UTC)
    repeat_rule = Column(Enum(RepeatRule), nullable=False, default=RepeatRule.ONCE)
    enabled = Column(Boolean, nullable=False, default=True)

    last_fired_at = Column(DateTime(timezone=True), nullable=True)
    created_at = Column(DateTime(timezone=True), default=lambda: datetime.now(timezone.utc))

    device = relationship("Device", back_populates="schedules")
