"""
Minimal in-memory sliding-window rate limiter, keyed by client IP.

This is deliberately simple for Phase 2 (single Render instance, no Redis
dependency). If DeskBot ever runs multiple backend instances, swap this for
a shared store (Redis) — the interface (one ASGI middleware) stays the same.
"""
import time
from collections import defaultdict, deque

from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import JSONResponse

from app.config import get_settings

settings = get_settings()


class RateLimitMiddleware(BaseHTTPMiddleware):
    def __init__(self, app):
        super().__init__(app)
        self._hits: dict[str, deque] = defaultdict(deque)
        self._limit = settings.rate_limit_per_minute
        self._window = 60.0

    async def dispatch(self, request: Request, call_next):
        client_ip = request.client.host if request.client else "unknown"
        now = time.monotonic()
        hits = self._hits[client_ip]

        while hits and now - hits[0] > self._window:
            hits.popleft()

        if len(hits) >= self._limit:
            return JSONResponse(status_code=429, content={"detail": "Rate limit exceeded"})

        hits.append(now)
        return await call_next(request)
