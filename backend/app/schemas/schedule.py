from datetime import datetime
from typing import Optional

from pydantic import BaseModel, Field

from app.models.schedule import RepeatRule


class ScheduleCreate(BaseModel):
    device_id: str
    message: str = Field(min_length=1, max_length=1000)
    scheduled_at: datetime = Field(description="First/next fire time, UTC ISO-8601")
    repeat_rule: RepeatRule = RepeatRule.ONCE


class ScheduleOut(BaseModel):
    id: str
    device_id: str
    message: str
    scheduled_at: datetime
    repeat_rule: RepeatRule
    enabled: bool
    last_fired_at: Optional[datetime]
    created_at: datetime

    class Config:
        from_attributes = True
