from typing import List, Optional

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session

from app.database import get_db
from app.deps import get_current_user, get_device_from_headers, get_device_from_key, get_owned_device
from app.models.device import Device
from app.models.user import User
from app.schemas.message import MessageAckRequest, MessageCreate, MessageOut
from app.services import message_service

router = APIRouter(prefix="/api/v1/messages", tags=["messages"])
device_router = APIRouter(prefix="/api/v1/devices", tags=["messages"])


@router.post("", response_model=MessageOut, status_code=status.HTTP_201_CREATED)
def create_message(
    payload: MessageCreate,
    db: Session = Depends(get_db),
    user: User = Depends(get_current_user),
):
    # Ownership check: the target device must belong to the caller.
    get_owned_device(payload.device_id, db, user)
    return message_service.create_message(db, payload)


@router.get("", response_model=List[MessageOut])
def list_messages(
    device_id: Optional[str] = None,
    db: Session = Depends(get_db),
    user: User = Depends(get_current_user),
):
    return message_service.list_messages_for_owner(db, user.id, device_id)


@router.get("/{message_id}", response_model=MessageOut)
def get_message(
    message_id: str,
    db: Session = Depends(get_db),
    user: User = Depends(get_current_user),
):
    return message_service.get_owned_message(db, user.id, message_id)


@router.post("/{message_id}/read", response_model=MessageOut)
def read_message(
    message_id: str,
    db: Session = Depends(get_db),
    user: User = Depends(get_current_user),
):
    return message_service.mark_read(db, user.id, message_id)


# --- Device-authenticated endpoints (called by ESP32 firmware) --------------

@device_router.get("/{device_id}/messages/pending", response_model=Optional[MessageOut])
def get_pending(
    db: Session = Depends(get_db),
    device: Device = Depends(get_device_from_key),
):
    """Polled by the ESP32 every DEVICE_POLL_INTERVAL_MS (~5s, configurable).
    Returns 200 with `null` body when there's nothing pending — never 404 —
    so firmware can treat "no message" as the normal case, not an error."""
    return message_service.get_pending_for_device(db, device)


@router.post("/{message_id}/ack", response_model=MessageOut)
def ack_message(
    message_id: str,
    payload: MessageAckRequest,
    db: Session = Depends(get_db),
    device: Device = Depends(get_device_from_headers),
):
    return message_service.ack_message(db, device, message_id)
