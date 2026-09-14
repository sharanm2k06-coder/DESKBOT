# DeskBot Frontend

Next.js 14 (App Router) + TypeScript + Tailwind console for DeskBot —
register a device, send it messages, schedule recurring ones, and check its
status, from a web browser.

## Design plan

**Subject**: a control console for a small desk gadget that itself has a
128×64 monochrome OLED. The web app isn't a generic dashboard — it's the
"big screen" version of the same device, so it borrows the OLED's visual
grammar rather than an unrelated SaaS-admin look.

- **Color** — `#08090A` void background, `#111315` panels, `#2A2D30`
  hairline borders, `#EDEFF0` primary text, `#F5F7F8` ("signal") as the one
  bright tone for primary actions. True monochrome, matching the physical
  display's black-and-white constraint — color is reserved only for
  functional status (`#7FDDA0` online, `#E86A5C` alert, `#E8C15C`
  important), never decoration.
- **Type** — JetBrains Mono for headings, labels, and all technical/status
  text (this is a device-readout aesthetic, so a monospace face is a
  content-driven choice, not a generic pick); Inter for longer message
  bodies and prose, where a mono face would hurt readability.
- **Layout** — left-aligned, flat bordered panels (6px radius, 1px hairline
  border, no shadows — the OLED has no shadows either), a sticky top nav
  styled like a device status bar rather than a sidebar. No gradient
  washes, no uniform "SaaS card kit" shadow.
- **Motion** — the robot face blinks (a single ambient loop, like the real
  device), status dots pulse only when a device is actually online; no
  scroll-triggered fade-ins.

## Pages

| Route | Purpose |
|---|---|
| `/login` | Register / sign in |
| `/` | Dashboard — device summary, robot face, pending count, recent messages |
| `/messages` | Send a message to the DeskBot; full history |
| `/schedule` | Create/list/delete scheduled (recurring) messages |
| `/device` | Register a device (shows the one-time device key), list devices + status |
| `/settings` | Backend connection info, WhatsApp-forwarding note, log out |

## Local development

```bash
cd frontend
npm install
cp .env.example .env.local     # point at your local or deployed backend
npm run dev
```

Open `http://localhost:3000`. You'll need the Phase 2 backend running (see
`../backend/README.md`) — register a user there via the `/login` page's
"Register" toggle.

## Deploying to Vercel

```bash
cd frontend
vercel
```

Or via the dashboard: **New Project** → import this repo, set the root
directory to `frontend/`, and add one environment variable:

| Key | Value |
|---|---|
| `NEXT_PUBLIC_API_URL` | your Render backend URL, e.g. `https://deskbot-backend.onrender.com` |

Build command and output are Next.js defaults — Vercel detects them
automatically.

## Notes

- **Auth**: the JWT is kept in `localStorage` (`deskbot_token`) and attached
  as `Authorization: Bearer <token>` by `lib/api.ts`. `components/AuthGate.tsx`
  redirects to `/login` when there's no token.
- **Polling**: the dashboard refetches every 10s so it reflects new
  heartbeats/messages without a manual reload — matching the backend's own
  polling-based design (see `docs/API_DOCUMENTATION.md#real-time-architecture`).
  No WebSocket is needed yet.
- **This was written without network access** in the build sandbox, so
  `npm install` / `npm run build` could not actually be executed here. Every
  `.ts`/`.tsx` file was written against the exact Next.js 14 / React 18 APIs
  it uses and reviewed by hand; please run `npm install && npm run build`
  yourself as the first step before deploying, and open an issue-equivalent
  note back if the build surfaces anything.
