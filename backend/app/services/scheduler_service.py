"""
Turns due ScheduledMessage rows into real Message rows the device can poll.

Phase-2 keeps this simple and pull-based: an async loop (started in main.py's
lifespan) wakes up every `scheduler_poll_interval_seconds` and calls
`run_due_schedules()` once. There's no separate worker process/queue yet —
see docs/API_DOCUMENTATION.md "Real-time architecture" for how this can grow
into a proper job queue (Celery/RQ) or push-based delivery without changing
the public API.
"""
import logging
from datetime import datetime, timedelta, timezone

from sqlalchemy.orm import Session

from app.database import SessionLocal
from app.models.message import Message, MessagePriority, MessageSource, MessageStatus
from app.models.schedule import RepeatRule, ScheduledMessage

logger = logging.getLogger("deskbot.scheduler")


def _next_occurrence(current: datetime, rule: RepeatRule) -> datetime:
    if rule == RepeatRule.DAILY:
        return current + timedelta(days=1)
    if rule == RepeatRule.WEEKLY:
        return current + timedelta(weeks=1)
    if rule == RepeatRule.WEEKDAYS:
        nxt = current + timedelta(days=1)
        while nxt.weekday() >= 5:  # Sat=5, Sun=6
            nxt += timedelta(days=1)
        return nxt
    return current  # ONCE: caller disables it instead of rescheduling


def run_due_schedules(db: Session) -> int:
    """Fires all due, enabled schedules. Returns the number fired."""
    now = datetime.now(timezone.utc)
    due = (
        db.query(ScheduledMessage)
        .filter(ScheduledMessage.enabled.is_(True), ScheduledMessage.scheduled_at <= now)
        .all()
    )

    fired = 0
    for sched in due:
        db.add(
            Message(
                device_id=sched.device_id,
                source=MessageSource.SCHEDULED,
                sender="Scheduled",
                title="Scheduled message",
                body=sched.message,
                priority=MessagePriority.NORMAL,
                status=MessageStatus.PENDING,
            )
        )
        sched.last_fired_at = now
        if sched.repeat_rule == RepeatRule.ONCE:
            sched.enabled = False
        else:
            sched.scheduled_at = _next_occurrence(sched.scheduled_at, sched.repeat_rule)
        fired += 1

    if fired:
        db.commit()
        logger.info("Fired %d scheduled message(s)", fired)
    return fired


def run_due_schedules_standalone() -> int:
    """Convenience wrapper that owns its own DB session, for the background loop."""
    db = SessionLocal()
    try:
        return run_due_schedules(db)
    finally:
        db.close()
