"use client";

import { useEffect, useState, useCallback, FormEvent } from "react";
import AuthGate from "@/components/AuthGate";
import Panel from "@/components/Panel";
import StatusPill from "@/components/StatusPill";
import { api, ApiError } from "@/lib/api";
import { Device, RepeatRule, ScheduledMessage } from "@/lib/types";

function toLocalInputValue(d: Date) {
  const pad = (n: number) => String(n).padStart(2, "0");
  return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())}T${pad(d.getHours())}:${pad(d.getMinutes())}`;
}

function ScheduleContent() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [schedules, setSchedules] = useState<ScheduledMessage[]>([]);
  const [deviceId, setDeviceId] = useState("");
  const [message, setMessage] = useState("");
  const [when, setWhen] = useState(() => toLocalInputValue(new Date(Date.now() + 60 * 60 * 1000)));
  const [repeat, setRepeat] = useState<RepeatRule>("ONCE");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const load = useCallback(async () => {
    const [d, s] = await Promise.all([api.listDevices(), api.listSchedules()]);
    setDevices(d);
    setSchedules(s);
    setDeviceId((prev) => prev || d[0]?.id || "");
  }, []);

  useEffect(() => {
    load();
  }, [load]);

  const submit = async (e: FormEvent) => {
    e.preventDefault();
    if (!deviceId || !message.trim()) return;
    setBusy(true);
    setError(null);
    try {
      await api.createSchedule({
        device_id: deviceId,
        message: message.trim(),
        scheduled_at: new Date(when).toISOString(),
        repeat_rule: repeat,
      });
      setMessage("");
      await load();
    } catch (err) {
      setError(err instanceof ApiError ? err.message : "Couldn't create the schedule.");
    } finally {
      setBusy(false);
    }
  };

  const remove = async (id: string) => {
    await api.deleteSchedule(id);
    await load();
  };

  return (
    <div className="space-y-5">
      <Panel title="New scheduled message">
        {devices.length === 0 ? (
          <p className="text-sm text-ink-faint">Register a device first, from the Device page.</p>
        ) : (
          <form onSubmit={submit} className="space-y-4">
            <div>
              <label htmlFor="message" className="label-tech mb-1.5 block">
                Message
              </label>
              <input
                id="message"
                required
                maxLength={1000}
                value={message}
                onChange={(e) => setMessage(e.target.value)}
                placeholder="Good morning!"
                className="w-full rounded-sm border border-line bg-panel-raised px-3 py-2 text-sm text-ink outline-none focus:border-line-bright"
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
                <label htmlFor="when" className="label-tech mb-1.5 block">
                  When
                </label>
                <input
                  id="when"
                  type="datetime-local"
                  value={when}
                  onChange={(e) => setWhen(e.target.value)}
                  className="rounded-sm border border-line bg-panel-raised px-3 py-2 text-sm text-ink outline-none focus:border-line-bright"
                />
              </div>

              <div>
                <label htmlFor="repeat" className="label-tech mb-1.5 block">
                  Repeat
                </label>
                <select
                  id="repeat"
                  value={repeat}
                  onChange={(e) => setRepeat(e.target.value as RepeatRule)}
                  className="rounded-sm border border-line bg-panel-raised px-3 py-2 text-sm text-ink outline-none focus:border-line-bright"
                >
                  <option value="ONCE">Once</option>
                  <option value="DAILY">Daily</option>
                  <option value="WEEKDAYS">Weekdays</option>
                  <option value="WEEKLY">Weekly</option>
                </select>
              </div>

              <button
                type="submit"
                disabled={busy}
                className="rounded-sm bg-signal px-4 py-2 font-mono text-sm text-void hover:opacity-90 disabled:opacity-50"
              >
                {busy ? "Saving···" : "Schedule it"}
              </button>
            </div>

            {error && <p className="text-sm text-alert">{error}</p>}
          </form>
        )}
      </Panel>

      <Panel title="Scheduled">
        {schedules.length === 0 ? (
          <p className="text-sm text-ink-faint">Nothing scheduled.</p>
        ) : (
          <ul className="divide-y divide-line">
            {schedules.map((s) => (
              <li key={s.id} className="flex items-center justify-between gap-4 py-3">
                <div className="min-w-0">
                  <p className="text-sm text-ink">{s.message}</p>
                  <p className="mt-1 text-xs text-ink-faint">
                    {new Date(s.scheduled_at).toLocaleString()} · {s.repeat_rule.toLowerCase()}
                  </p>
                </div>
                <div className="flex shrink-0 items-center gap-3">
                  <StatusPill tone={s.enabled ? "online" : "offline"}>
                    {s.enabled ? "active" : "done"}
                  </StatusPill>
                  <button
                    onClick={() => remove(s.id)}
                    className="font-mono text-xs text-ink-faint hover:text-alert"
                  >
                    Delete
                  </button>
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
      <ScheduleContent />
    </AuthGate>
  );
}
