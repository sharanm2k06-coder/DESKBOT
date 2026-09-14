"use client";

import { useEffect, useState, useCallback, FormEvent } from "react";
import AuthGate from "@/components/AuthGate";
import Panel from "@/components/Panel";
import StatusPill from "@/components/StatusPill";
import { api, ApiError } from "@/lib/api";
import { Device } from "@/lib/types";

function DeviceContent() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [deviceUid, setDeviceUid] = useState("DESKBOT-01");
  const [name, setName] = useState("");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [newKey, setNewKey] = useState<{ device_uid: string; device_key: string } | null>(null);

  const load = useCallback(async () => {
    setDevices(await api.listDevices());
  }, []);

  useEffect(() => {
    load();
  }, [load]);

  const submit = async (e: FormEvent) => {
    e.preventDefault();
    setBusy(true);
    setError(null);
    try {
      const res = await api.registerDevice(deviceUid.trim(), name.trim() || undefined);
      setNewKey({ device_uid: res.device_uid, device_key: res.device_key });
      setDeviceUid("");
      setName("");
      await load();
    } catch (err) {
      setError(err instanceof ApiError ? err.message : "Couldn't register the device.");
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="space-y-5">
      {newKey && (
        <Panel title="Device key — copy it now" className="border-important/40">
          <p className="mb-3 text-sm text-ink-dim">
            This is the only time <span className="text-ink">{newKey.device_uid}</span>&apos;s device
            key will be shown. Paste it into <code className="font-mono text-ink">config.h</code> on
            the ESP32 (or wherever the firmware reads its backend credentials).
          </p>
          <code className="block break-all rounded-sm border border-line bg-panel-raised p-3 font-mono text-xs text-ink">
            {newKey.device_key}
          </code>
          <button
            onClick={() => setNewKey(null)}
            className="mt-3 font-mono text-xs text-ink-faint hover:text-ink-dim"
          >
            I&apos;ve saved it
          </button>
        </Panel>
      )}

      <Panel title="Register a device">
        <form onSubmit={submit} className="flex flex-wrap items-end gap-4">
          <div>
            <label htmlFor="uid" className="label-tech mb-1.5 block">
              Device ID
            </label>
            <input
              id="uid"
              required
              value={deviceUid}
              onChange={(e) => setDeviceUid(e.target.value)}
              placeholder="DESKBOT-01"
              className="rounded-sm border border-line bg-panel-raised px-3 py-2 text-sm text-ink outline-none focus:border-line-bright"
            />
          </div>
          <div>
            <label htmlFor="name" className="label-tech mb-1.5 block">
              Name (optional)
            </label>
            <input
              id="name"
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="Desk"
              className="rounded-sm border border-line bg-panel-raised px-3 py-2 text-sm text-ink outline-none focus:border-line-bright"
            />
          </div>
          <button
            type="submit"
            disabled={busy}
            className="rounded-sm bg-signal px-4 py-2 font-mono text-sm text-void hover:opacity-90 disabled:opacity-50"
          >
            {busy ? "Registering···" : "Register"}
          </button>
        </form>
        {error && <p className="mt-3 text-sm text-alert">{error}</p>}
      </Panel>

      <Panel title="Your devices">
        {devices.length === 0 ? (
          <p className="text-sm text-ink-faint">No devices registered yet.</p>
        ) : (
          <ul className="divide-y divide-line">
            {devices.map((d) => (
              <li key={d.id} className="grid gap-3 py-4 sm:grid-cols-2">
                <div>
                  <p className="text-sm text-ink">{d.name}</p>
                  <p className="text-xs text-ink-faint">{d.device_uid}</p>
                </div>
                <div className="space-y-1.5 text-xs text-ink-dim sm:text-right">
                  <div className="flex items-center gap-2 sm:justify-end">
                    <span className="label-tech">status</span>
                    <StatusPill tone={d.status === "ONLINE" ? "online" : "offline"} pulse={d.status === "ONLINE"}>
                      {d.status.toLowerCase()}
                    </StatusPill>
                  </div>
                  <p>firmware {d.firmware_version || "—"}</p>
                  <p>last seen {d.last_seen ? new Date(d.last_seen).toLocaleString() : "never"}</p>
                </div>
              </li>
            ))}
          </ul>
        )}
      </Panel>
    </div>
  );
}

export default function Page() {
  return (
    <AuthGate>
      <DeviceContent />
    </AuthGate>
  );
}
