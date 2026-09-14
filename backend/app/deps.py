from fastapi import Depends, HTTPException, Header, status
from fastapi.security import HTTPAuthorizationCredentials, HTTPBearer
from sqlalchemy.orm import Session

from app.database import get_db
from app.models.device import Device
from app.models.user import User
from app.security import decode_access_token, verify_device_key

bearer_scheme = HTTPBearer(auto_error=False)


def get_current_user(
    credentials: HTTPAuthorizationCredentials = Depends(bearer_scheme),
    db: Session = Depends(get_db),
) -> User:
    if credentials is None:
        raise HTTPException(status.HTTP_401_UNAUTHORIZED, "Missing bearer token")
    user_id = decode_access_token(credentials.credentials)
    if not user_id:
        raise HTTPException(status.HTTP_401_UNAUTHORIZED, "Invalid or expired token")
    user = db.get(User, user_id)
    if not user:
        raise HTTPException(status.HTTP_401_UNAUTHORIZED, "User not found")
    return user


def get_device_from_key(
    device_id: str,
    x_device_key: str = Header(..., description="Device's bearer secret issued at registration"),
    db: Session = Depends(get_db),
) -> Device:
    """
    Authenticates a *device* (ESP32/Android acting on its behalf), not a
    user. Used for heartbeat, pending-message polling, and ack endpoints —
    the ones firmware calls directly and repeatedly, so we avoid making the
    device carry a user's JWT.
    """
    device = db.get(Device, device_id)
    if not device:
        raise HTTPException(status.HTTP_404_NOT_FOUND, "Device not found")
    if not verify_device_key(x_device_key, device.device_key_hash):
        raise HTTPException(status.HTTP_401_UNAUTHORIZED, "Invalid device key")
    return device


def get_device_from_headers(
    x_device_id: str = Header(..., description="Device's own id (returned at registration)"),
    x_device_key: str = Header(..., description="Device's bearer secret issued at registration"),
    db: Session = Depends(get_db),
) -> Device:
    """
    Same as get_device_from_key, but for endpoints whose URL has no
    {device_id} path segment (e.g. POST /messages/{message_id}/ack) — the
    device instead identifies itself via X-Device-Id/X-Device-Key headers.
    """
    device = db.get(Device, x_device_id)
    if not device:
        raise HTTPException(status.HTTP_404_NOT_FOUND, "Device not found")
    if not verify_device_key(x_device_key, device.device_key_hash):
        raise HTTPException(status.HTTP_401_UNAUTHORIZED, "Invalid device key")
    return device


def get_owned_device(
    device_id: str,
    db: Session = Depends(get_db),
    user: User = Depends(get_current_user),
) -> Device:
    """Used by user-facing endpoints (send message, view device, schedule)."""
    device = db.get(Device, device_id)
    if not device or device.owner_id != user.id:
        raise HTTPException(status.HTTP_404_NOT_FOUND, "Device not found")
    return device
