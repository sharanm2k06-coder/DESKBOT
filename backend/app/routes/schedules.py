from typing import List, Optional

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session

from app.database import get_db
from app.deps import get_current_user, get_owned_device
from app.models.schedule import ScheduledMessage
from app.models.user import User
from app.schemas.schedule import ScheduleCreate, ScheduleOut

router = APIRouter(prefix="/api/v1/schedules", tags=["schedules"])


@router.post("", response_model=ScheduleOut, status_code=status.HTTP_201_CREATED)
def create_schedule(
    payload: ScheduleCreate,
    db: Session = Depends(get_db),
    user: User = Depends(get_current_user),
):
    get_owned_device(payload.device_id, db, user)  # ownership check
    schedule = ScheduledMessage(
        device_id=payload.device_id,
        message=payload.message,
        scheduled_at=payload.scheduled_at,
        repeat_rule=payload.repeat_rule,
    )
    db.add(schedule)
    db.commit()
    db.refresh(schedule)
    return schedule


@router.get("", response_model=List[ScheduleOut])
def list_schedules(
    device_id: Optional[str] = None,
    db: Session = Depends(get_db),
    user: User = Depends(get_current_user),
):
    query = (
        db.query(ScheduledMessage)
        .join(ScheduledMessage.device)
        .filter(ScheduledMessage.device.has(owner_id=user.id))
    )
    if device_id:
        query = query.filter(ScheduledMessage.device_id == device_id)
    return query.order_by(ScheduledMessage.scheduled_at.asc()).all()


@router.delete("/{schedule_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_schedule(
    schedule_id: str,
    db: Session = Depends(get_db),
    user: User = Depends(get_current_user),
):
    schedule = (
        db.query(ScheduledMessage)
        .join(ScheduledMessage.device)
        .filter(ScheduledMessage.id == schedule_id, ScheduledMessage.device.has(owner_id=user.id))
        .first()
    )
    if not schedule:
        raise HTTPException(status.HTTP_404_NOT_FOUND, "Schedule not found")
    db.delete(schedule)
    db.commit()
    return None
