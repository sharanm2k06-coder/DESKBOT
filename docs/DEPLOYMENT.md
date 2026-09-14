# DeskBot Deployment

## Backend → Render

### Option A: `render.yaml` (recommended, one shot)

1. Push the `DESKBOT/` repo (or just `backend/`) to GitHub.
2. In Render: **New → Blueprint**, point it at the repo. Render reads
   `backend/render.yaml` and creates:
   - a free Postgres database (`deskbot-db`)
   - a Docker web service (`deskbot-backend`) wired to that database via
     `DATABASE_URL`, with `JWT_SECRET` auto-generated.
3. Edit the `CORS_ORIGINS` env var in the Render dashboard once you know
   your Vercel URL (Phase 3) — it's a placeholder in `render.yaml`.
4. Deploy. Render builds `backend/Dockerfile` and runs
   `uvicorn app.main:app --host 0.0.0.0 --port 8000`; health checks hit
   `/healthz`.

### Option B: manual service creation

1. **New → PostgreSQL** — note the **Internal Database URL**.
2. **New → Web Service** → connect the repo, root directory `backend/`,
   environment **Docker**.
3. Environment variables:
   | Key | Value |
   |---|---|
   | `DATABASE_URL` | the Postgres Internal Database URL from step 1 |
   | `JWT_SECRET` | a long random string (`openssl rand -hex 32`) |
   | `CORS_ORIGINS` | your Vercel frontend URL, comma-separated if multiple |
   | `ENVIRONMENT` | `production` |
4. Health check path: `/healthz`.

### Build/start commands (if not using the Dockerfile path)

- Build: `pip install -r requirements.txt`
- Start: `uvicorn app.main:app --host 0.0.0.0 --port $PORT`

### Verifying the deploy

```bash
curl https://<your-service>.onrender.com/healthz
# {"status": "ok"}
```

Then run through the smoke test in `backend/README.md`.

### Database schema

Phase 2 creates tables via `Base.metadata.create_all()` at process startup
(see `app/main.py`) — there is no separate migration step to run. This is
fine for getting a single-owner deployment running quickly; if the schema
needs to evolve later without dropping data, introduce Alembic at that
point rather than continuing to rely on `create_all`.

---

## Frontend → Vercel

*(Covered in full once Phase 3 — the Next.js frontend — is implemented.
Summary for now: set `NEXT_PUBLIC_API_URL` to the Render backend URL,
`vercel deploy` from the `frontend/` directory.)*

## Android → Play Store / sideload

*(Covered once Phase 4 — the Android companion — is implemented.)*

## ESP32 firmware

Already covered in `firmware/README.md` and `docs/HARDWARE_WIRING.md`
(Phase 1). No change needed for Phase 2 beyond eventually adding the
backend base URL and device credentials to `config.h` once the ESP32's
`ApiClient` module (Phase 5 integration) is wired up to call this backend.

## Environment variable summary

| Component | Variable | Notes |
|---|---|---|
| Backend | `DATABASE_URL` | Postgres connection string (Render provides this) |
| Backend | `JWT_SECRET` | never commit; rotate if leaked (invalidates all sessions) |
| Backend | `CORS_ORIGINS` | comma-separated list of allowed frontend origins |
| Backend | `ACCESS_TOKEN_EXPIRE_MINUTES` | default 1440 (24h) |
| Backend | `SCHEDULER_POLL_INTERVAL_SECONDS` | default 30 |
| Backend | `RATE_LIMIT_PER_MINUTE` | default 120 |
| Frontend | `NEXT_PUBLIC_API_URL` | set once backend is deployed |
| ESP32 | (none yet) | Phase 5 will add backend URL + device key to `config.h` |
