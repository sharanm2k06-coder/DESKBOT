from datetime import datetime
from typing import Optional

from pydantic import BaseModel, Field

from app.models.message import MessagePriority, MessageSource, MessageStatus


class MessageCreate(BaseModel):
    device_id: str
    body: str = Field(min_length=1, max_length=1000)
    source: MessageSource = MessageSource.APP
    sender: Optional[str] = Field(default=None, max_length=100)
    title: Optional[str] = Field(default=None, max_length=100)
    priority: MessagePriority = MessagePriority.NORMAL
    ttl_seconds: Optional[int] = Field(
        default=None, ge=1, le=60 * 60 * 24 * 7,
        description="Optional expiry; message stops being delivered after this many seconds.",
    )


class MessageOut(BaseModel):
    id: str
    device_id: str
    source: MessageSource
    sender: Optional[str]
    title: Optional[str]
    body: str
    priority: MessagePriority
    status: MessageStatus
    created_at: datetime
    delivered_at: Optional[datetime]
    read_at: Optional[datetime]
    expires_at: Optional[datetime]

    class Config:
        from_attributes = True


class MessageAckRequest(BaseModel):
    note: Optional[str] = Field(default=None, max_length=200)
