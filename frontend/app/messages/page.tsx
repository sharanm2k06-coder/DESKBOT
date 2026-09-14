"use client";

import { useEffect, useState, useCallback, FormEvent } from "react";
import AuthGate from "@/components/AuthGate";
import Panel from "@/components/Panel";
import StatusPill from "@/components/StatusPill";
import { api, ApiError } from "@/lib/api";
import { Device, Message, MessagePriority } from "@/lib/types";

function MessagesContent() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [messages, setMessages] = useState<Message[]>([]);
  const [body, setBody] = useState("");
  const [priority, setPriority] = useState<MessagePriority>("NORMAL");
  const [deviceId, setDeviceId] = useState<string>("");
  const [sending, setSending] = useState(false);
  const [feedback, setFeedback] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);

  const load = useCallback(async () => {
    const [d, m] = await Promise.all([api.listDevices(), api.listMessages()]);
    setDevices(d);
    setMessages(m);
    setDeviceId((prev) => prev || d[0]?.id || "");
  }, []);

  useEffect(() => {
    load();
  }, [load]);

  const submit = async (e: FormEvent) => {
    e.preventDefault();
    if (!deviceId || !body.trim()) return;
    setSending(true);
    setError(null);
    setFeedback(null);
    try {
      await api.sendMessage({ device_id: deviceId, body: body.trim(), priority });
      setBody("");
      setFeedback("Sent to DeskBot.");
      await load();
    } catch (err) {
      setError(err instanceof ApiError ? err.message : "Couldn't send the message.");
    } finally {
      setSending(false);
    }
  };

  return (
    <div className="space-y-5">
      <Panel title="Send message">
        {devices.length === 0 ? (
          <p className="text-sm text-ink-faint">Register a device first, from the Device page.</p>
        ) : (
          <form onSubmit={submit} className="space-y-4">
            <div>
              <label htmlFor="body" className="label-tech mb-1.5 block">
                Message
              </label>
              <textarea
                id="body"
                required
                maxLength={1000}
                rows={3}
                value={body}
                onChange={(e) => setBody(e.target.value)}
                placeholder="Team meeting at 11:00"
                className="w-full resize-none rounded-sm border border-line bg-panel-raised px-3 py-2 text-sm text-ink outline-none focus:border-line-bright"
              />
            </div>

            <div className="flex flex-wrap items-end gap-4">
              <div>
                <label htmlFor="device" className="label-tech mb-1.5 block">
                  Device
                </label>
                <select
                  id="device"
                  value={deviceId}
                  onChange={(e) => setDeviceId(e.target.value)}
                  className="rounded-sm border border-line bg-panel-raised px-3 py-2 text-sm text-ink outline-none focus:border-line-bright"
                >
                  {devices.map((d) => (
                    <option key={d.id} value={d.id}>
                      {d.name}
                    </option>
                  ))}
                </select>
              </div>

              <div>
                <label htmlFor="priority" className="label-tech mb-1.5 block">
                  Priority
                </label>
                <select
                  id="priority"
                  value={priority}
                  onChange={(e) => setPriority(e.target.value as MessagePriority)}
                  className="rounded-sm border border-line bg-panel-raised px-3 py-2 text-sm text-ink outline-none focus:border-line-bright"
                >
                  <option value="NORMAL">Normal</option>
                  <option value="IMPORTANT">Important</option>
                  <option value="ALERT">Alert</option>
                </select>
              </div>

              <button
                type="submit"
                disabled={sending}
                className="rounded-sm bg-signal px-4 py-2 font-mono text-sm text-void hover:opacity-90 disabled:opacity-50"
              >
                {sending ? "Sending···" : "Send to DeskBot"}
              </button>
            </div>

            {feedback && <p className="text-sm text-online">{feedback}</p>}
            {error && <p className="text-sm text-alert">{error}</p>}
          </form>
        )}
      </Panel>

      <Panel title="History">
        {messages.length === 0 ? (
          <p className="text-sm text-ink-faint">No messages yet.</p>
        ) : (
          <ul className="divide-y divide-line">
            {messages.map((m) => (
              <li key={m.id} className="py-3">
                <div className="flex items-start justify-between gap-4">
                  <div className="min-w-0">
                    <p className="text-sm text-ink">{m.body}</p>
                    <p className="mt-1 text-xs text-ink-faint">
                      {m.source.toLowerCase()} · {new Date(m.created_at).toLocaleString()}
                    </p>
                  </div>
                  <div className="flex shrink-0 gap-1.5">
                    {m.priority !== "NORMAL" && (
                      <StatusPill tone={m.priority === "ALERT" ? "alert" : "important"}>
                        {m.priority.toLowerCase()}
                      </StatusPill>
                    )}
                    <StatusPill tone="neutral">{m.status.toLowerCase()}</StatusPill>
                  </div>
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
      <MessagesContent />
    </AuthGate>
  );
}
