"use client";

import AuthGate from "@/components/AuthGate";
import Panel from "@/components/Panel";
import { useAuth } from "@/lib/auth";

const API_URL = process.env.NEXT_PUBLIC_API_URL || "http://localhost:8000";

function SettingsContent() {
  const { logout } = useAuth();

  return (
    <div className="space-y-5">
      <Panel title="Connection">
        <p className="text-sm text-ink-dim">
          This console talks to the DeskBot backend at:
        </p>
        <code className="mt-2 block break-all rounded-sm border border-line bg-panel-raised p-3 font-mono text-xs text-ink">
          {API_URL}
        </code>
        <p className="mt-2 text-xs text-ink-faint">
          Set via <code className="font-mono">NEXT_PUBLIC_API_URL</code> at deploy time.
        </p>
      </Panel>

      <Panel title="WhatsApp forwarding">
        <p className="text-sm text-ink-dim">
          WhatsApp notifications are forwarded phone → Bluetooth → DeskBot directly by the Android
          companion app. They never pass through this web console or the backend — turn forwarding
          on or off from the companion app&apos;s WhatsApp settings screen.
        </p>
      </Panel>

      <Panel title="Account">
        <button
          onClick={logout}
          className="rounded-sm border border-line px-4 py-2 font-mono text-sm text-ink-dim hover:border-alert/50 hover:text-alert"
        >
          Log out
        </button>
      </Panel>
    </div>
  );
}

export default function Page() {
  return (
    <AuthGate>
      <SettingsContent />
    </AuthGate>
  );
}
