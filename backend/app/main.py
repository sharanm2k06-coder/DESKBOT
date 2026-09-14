import asyncio
import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from app.config import get_settings
from app.database import Base, engine
from app.rate_limit import RateLimitMiddleware
from app.routes import auth, devices, messages, schedules
from app.services.scheduler_service import run_due_schedules_standalone

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("deskbot")
settings = get_settings()


async def _scheduler_loop():
    interval = settings.scheduler_poll_interval_seconds
    while True:
        try:
            run_due_schedules_standalone()
        except Exception:  # noqa: BLE001 — one bad tick must not kill the loop
            logger.exception("Scheduled-message tick failed")
        await asyncio.sleep(interval)


@asynccontextmanager
async def lifespan(app: FastAPI):
    # In production, prefer Alembic migrations over create_all(); this is
    # kept for fast local/dev bring-up and for the test suite.
    Base.metadata.create_all(bind=engine)
    task = asyncio.create_task(_scheduler_loop())
    logger.info("DeskBot backend started (env=%s)", settings.environment)
    yield
    task.cancel()


app = FastAPI(
    title="DeskBot API",
    description="Backend for the DeskBot smart desktop companion.",
    version="1.0.0",
    lifespan=lifespan,
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.cors_origin_list,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
app.add_middleware(RateLimitMiddleware)

app.include_router(auth.router)
app.include_router(devices.router)
app.include_router(messages.router)
app.include_router(messages.device_router)
app.include_router(schedules.router)


@app.get("/", tags=["meta"])
def root():
    return {"service": "deskbot-backend", "status": "ok"}


@app.get("/healthz", tags=["meta"])
def healthz():
    return {"status": "ok"}
