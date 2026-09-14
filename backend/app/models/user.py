import uuid
from datetime import datetime, timezone

from sqlalchemy import Column, DateTime, String
from sqlalchemy.orm import relationship

from app.database import Base


def _uuid() -> str:
    return str(uuid.uuid4())


class User(Base):
    """
    A String(36) UUID primary key is used (rather than the postgres-only
    UUID type) so the same models work against both Postgres in
    production and SQLite in tests/local dev without dialect branching.
    """

    __tablename__ = "users"

    id = Column(String(36), primary_key=True, default=_uuid)
    email = Column(String(255), unique=True, nullable=False, index=True)
    hashed_password = Column(String(255), nullable=False)
    created_at = Column(DateTime(timezone=True), default=lambda: datetime.now(timezone.utc))

    devices = relationship("Device", back_populates="owner", cascade="all, delete-orphan")
