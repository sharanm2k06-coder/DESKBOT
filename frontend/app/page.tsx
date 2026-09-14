"use client";

import { useEffect, useState, useCallback } from "react";
import Link from "next/link";
import AuthGate from "@/components/AuthGate";
import Panel from "@/components/Panel";
import StatusPill from "@/components/StatusPill";
import RobotFace from "@/components/RobotFace";
import { api } from "@/lib/api";
import { Device, Message, ScheduledMessage } from "@/lib/types";

function DashboardContent() {
  const [devices, setDevices] = useState<Device[] | null>(null);
  const [messages, setMessages] = useState<Message[] | null>(null);
  const [schedules, setSchedules] = useState<ScheduledMessage[] | null>(null);
  const [error, setError] = useState<string | null>(null);

  const load = useCallback(async () => {
    try {
      const [d, m, s] = await Promise.all([
        api.listDevices(),
        api.listMessages(),
        api.listSchedules(),
      ]);
      setDevices(d);
      setMessages(m);
      setSchedules(s);
    } catch {
      setError("Couldn't reach the DeskBot backend. Check NEXT_PUBLIC_API_URL and try again.");
    }
  }, []);

  useEffect(() => {
    load();
    const interval = setInterval(load, 10_000);
    return () => clearInterval(interval);
  }, [load]);

  const device = devices?.[0];
  const pendingCount = messages?.filter((m) => m.status === "PENDING" || m.status === "DELIVERED").length ?? 0;
  const activeSchedules = schedules?.filter((s) => s.enabled).length ?? 0;

  if (error) {
    return (
      <Panel title="Connection">
        <p className="text-sm text-alert">{error}</p>
      </Panel>
    );
  }

  if (!device) {
    return devices === null ? (
      <p className="label-tech animate-pulseDot">loading···</p>
    ) : (
      <Panel title="No device yet">
        <p className="mb-4 text-sm text-ink-dim">
          You haven&apos;t registered a DeskBot on this account. Register one from the Device page
          to start sending it messages.
        </p>
        <Link
          href="/device"
          className="inline-block rounded-sm bg-signal px-4 py-2 font-mono text-sm text-void hover:opacity-90"
        >
          Go to Device
        </Link>
      </Panel>
    );
  }

  return (
    <div className="space-y-5">
      <Panel>
        <div className="flex flex-col items-center gap-5 sm:flex-row sm:justify-between">
          <div className="flex items-center gap-5">
            <RobotFace state={device.status === "OFFLINE" ? "OFFLINE" : pendingCount > 0 ? "ALERT" : "NORMAL"} />
            <div>
              <h1 className="font-mono text-lg text-ink">{device.name}</h1>
              <p className="text-sm text-ink-faint">{device.device_uid}</p>
              <div className="mt-2 flex flex-wrap gap-2">
                <StatusPill tone={device.status === "ONLINE" ? "online" : "offline"} pulse={device.status === "ONLINE"}>
                  {device.status.toLowerCase()}
                </StatusPill>
                {device.firmware_version && (
                  <StatusPill tone="neutral">fw {device.firmware_version}</StatusPill>
                )}
              </div>
            </div>
          </div>
        </div>
      </Panel>

      <div className="grid gap-5 sm:grid-cols-3">
        <Panel title="Pending messages">
          <p className="font-mono text-3xl text-ink">{pendingCount}</p>
          <Link href="/messages" className="mt-2 inline-block text-xs text-ink-faint hover:text-ink-dim">
            View messages →
          </Link>
        </Panel>
        <Panel title="Active schedules">
          <p className="font-mono text-3xl text-ink">{activeSchedules}</p>
          <Link href="/schedule" className="mt-2 inline-block text-xs text-ink-faint hover:text-ink-dim">
            Manage schedule →
          </Link>
        </Panel>
        <Panel title="Last seen">
          <p className="font-mono text-sm text-ink">
            {device.last_seen ? new Date(device.last_seen).toLocaleString() : "Never"}
          </p>
          <Link href="/device" className="mt-2 inline-block text-xs text-ink-faint hover:text-ink-dim">
            Device details →
          </Link>
        </Panel>
      </div>

      <Panel title="Recent messages">
        {messages && messages.length > 0 ? (
          <ul className="divide-y divide-line">
            {messages.slice(0, 5).map((m) => (
              <li key={m.id} className="flex items-center justify-between gap-4 py-2.5">
                <div className="min-w-0">
                  <p className="truncate text-sm text-ink">{m.body}</p>
                  <p className="text-xs text-ink-faint">{new Date(m.created_at).toLocaleString()}</p>
                </div>
                <StatusPill tone={m.priority === "ALERT" ? "alert" : m.priority === "IMPORTANT" ? "important" : "neutral"}>
                  {m.status.toLowerCase()}
                </StatusPill>
              </li>
            ))}
          </ul>
        ) : (
          <p className="text-sm text-ink-faint">Nothing sent yet.</p>
        )}
      </Panel>
    </div>
  );
}

export default function Page() {
  return (
    <AuthGate>
      <DashboardContent />
    </AuthGate>
  );
}
