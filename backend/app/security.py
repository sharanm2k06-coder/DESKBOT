"""
Password hashing, JWT issuance/verification, and device-key hashing.

Two distinct credential types exist, deliberately kept separate:
  - User JWT: issued at /auth/login, used for web/app calls that act on
    behalf of a person (registering devices, sending messages, viewing
    history).
  - Device key: a long random token issued once at device registration,
    hashed at rest exactly like a password, used only by the device itself
    for heartbeat + pending-message polling + ack. A device never receives
    or needs a user JWT.
"""
import hashlib
import hmac
from datetime import datetime, timedelta, timezone
from typing import Optional

from jose import JWTError, jwt
from passlib.context import CryptContext

from app.config import get_settings

settings = get_settings()
pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")


# --- User passwords ---------------------------------------------------------

def hash_password(password: str) -> str:
    return pwd_context.hash(password)


def verify_password(password: str, hashed: str) -> bool:
    return pwd_context.verify(password, hashed)


# --- User JWTs ---------------------------------------------------------------

def create_access_token(subject: str) -> str:
    expire = datetime.now(timezone.utc) + timedelta(minutes=settings.access_token_expire_minutes)
    payload = {"sub": subject, "exp": expire}
    return jwt.encode(payload, settings.jwt_secret, algorithm=settings.jwt_algorithm)


def decode_access_token(token: str) -> Optional[str]:
    try:
        payload = jwt.decode(token, settings.jwt_secret, algorithms=[settings.jwt_algorithm])
        return payload.get("sub")
    except JWTError:
        return None


# --- Device keys --------------------------------------------------------------
# Device keys are high-entropy random tokens (see models.device.generate_device_key)
# rather than user-chosen passwords, so a fast, constant-time-compare SHA-256
# hash is appropriate here (unlike user passwords, where bcrypt's slowness is
# the point). This keeps device auth cheap enough for frequent polling.

def hash_device_key(raw_key: str) -> str:
    return hashlib.sha256(raw_key.encode("utf-8")).hexdigest()


def verify_device_key(raw_key: str, hashed: str) -> bool:
    return hmac.compare_digest(hash_device_key(raw_key), hashed)
