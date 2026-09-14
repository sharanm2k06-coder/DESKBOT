from datetime import datetime, timedelta, timezone
from typing import List, Optional

from fastapi import HTTPException, status
from sqlalchemy.orm import Session

from app.models.device import Device
from app.models.message import Message, MessageStatus
from app.schemas.message import MessageCreate


def create_message(db: Session, payload: MessageCreate) -> Message:
    device = db.get(Device, payload.device_id)
    if not device:
        raise HTTPException(status.HTTP_404_NOT_FOUND, "Device not found")

    expires_at = None
    if payload.ttl_seconds:
        expires_at = datetime.now(timezone.utc) + timedelta(seconds=payload.ttl_seconds)

    message = Message(
        device_id=device.id,
        source=payload.source,
        sender=payload.sender,
        title=payload.title,
        body=payload.body,
        priority=payload.priority,
        status=MessageStatus.PENDING,
        expires_at=expires_at,
    )
    db.add(message)
    db.commit()
    db.refresh(message)
    return message


def list_messages_for_owner(db: Session, owner_id: str, device_id: Optional[str], limit: int = 50) -> List[Message]:
    query = db.query(Message).join(Device).filter(Device.owner_id == owner_id)
    if device_id:
        query = query.filter(Message.device_id == device_id)
    return query.order_by(Message.created_at.desc()).limit(limit).all()


def get_owned_message(db: Session, owner_id: str, message_id: str) -> Message:
    message = (
        db.query(Message)
        .join(Device)
        .filter(Message.id == message_id, Device.owner_id == owner_id)
        .first()
    )
    if not message:
        raise HTTPException(status.HTTP_404_NOT_FOUND, "Message not found")
    return message


def get_pending_for_device(db: Session, device: Device) -> Optional[Message]:
    """
    Returns the single oldest actionable message for a polling ESP32, or
    None. One-at-a-time by design: the device's own RAM queue
    (MessageManager, MAX_QUEUED_MESSAGES=10) is the multi-message buffer;
    the backend just needs to hand over whatever's next and mark it
    DELIVERED once it has.
    """
    now = datetime.now(timezone.utc)

    # Lazily expire anything overdue so it's never handed to the device.
    expired = (
        db.query(Message)
        .filter(
            Message.device_id == device.id,
            Message.status.in_([MessageStatus.PENDING, MessageStatus.DELIVERED]),
            Message.expires_at.isnot(None),
            Message.expires_at < now,
        )
        .all()
    )
    for m in expired:
        m.status = MessageStatus.EXPIRED
    if expired:
        db.commit()

    message = (
        db.query(Message)
        .filter(Message.device_id == device.id, Message.status == MessageStatus.PENDING)
        .order_by(Message.created_at.asc())
        .first()
    )
    if message:
        message.status = MessageStatus.DELIVERED
        message.delivered_at = now
        db.commit()
        db.refresh(message)
    return message


def ack_message(db: Session, device: Device, message_id: str) -> Message:
    message = db.query(Message).filter(Message.id == message_id, Message.device_id == device.id).first()
    if not message:
        raise HTTPException(status.HTTP_404_NOT_FOUND, "Message not found for this device")
    if message.status not in (MessageStatus.DELIVERED, MessageStatus.PENDING):
        raise HTTPException(status.HTTP_409_CONFLICT, f"Message already in status {message.status}")
    message.status = MessageStatus.ACKED
    db.commit()
    db.refresh(message)
    return message


def mark_read(db: Session, owner_id: str, message_id: str) -> Message:
    message = get_owned_message(db, owner_id, message_id)
    message.status = MessageStatus.READ
    message.read_at = datetime.now(timezone.utc)
    db.commit()
    db.refresh(message)
    return message
