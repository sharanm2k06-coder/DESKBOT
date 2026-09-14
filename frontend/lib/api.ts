const API_URL = process.env.NEXT_PUBLIC_API_URL || "http://localhost:8000";

export class ApiError extends Error {
  status: number;
  constructor(message: string, status: number) {
    super(message);
    this.status = status;
  }
}

function getToken(): string | null {
  if (typeof window === "undefined") return null;
  return window.localStorage.getItem("deskbot_token");
}

async function request<T>(
  path: string,
  options: RequestInit & { auth?: boolean } = {}
): Promise<T> {
  const { auth = true, headers, ...rest } = options;
  const finalHeaders: Record<string, string> = {
    "Content-Type": "application/json",
    ...(headers as Record<string, string>),
  };

  if (auth) {
    const token = getToken();
    if (token) finalHeaders["Authorization"] = `Bearer ${token}`;
  }

  const res = await fetch(`${API_URL}${path}`, { ...rest, headers: finalHeaders });

  if (!res.ok) {
    let detail = res.statusText;
    try {
      const body = await res.json();
      detail = body.detail || detail;
    } catch {
      /* response had no JSON body */
    }
    throw new ApiError(detail, res.status);
  }

  if (res.status === 204) return undefined as T;
  return (await res.json()) as T;
}

export const api = {
  // --- auth ---
  register: (email: string, password: string) =>
    request<{ id: string; email: string }>("/api/v1/auth/register", {
      method: "POST",
      body: JSON.stringify({ email, password }),
      auth: false,
    }),
  login: (email: string, password: string) =>
    request<{ access_token: string; token_type: string }>("/api/v1/auth/login", {
      method: "POST",
      body: JSON.stringify({ email, password }),
      auth: false,
    }),

  // --- devices ---
  listDevices: () => request<import("./types").Device[]>("/api/v1/devices"),
  getDevice: (id: string) => request<import("./types").Device>(`/api/v1/devices/${id}`),
  registerDevice: (device_uid: string, name?: string) =>
    request<import("./types").DeviceRegisterResponse>("/api/v1/devices/register", {
      method: "POST",
      body: JSON.stringify({ device_uid, name }),
    }),

  // --- messages ---
  listMessages: (deviceId?: string) =>
    request<import("./types").Message[]>(
      `/api/v1/messages${deviceId ? `?device_id=${deviceId}` : ""}`
    ),
  sendMessage: (payload: {
    device_id: string;
    body: string;
    priority: import("./types").MessagePriority;
    title?: string;
    sender?: string;
    ttl_seconds?: number;
  }) =>
    request<import("./types").Message>("/api/v1/messages", {
      method: "POST",
      body: JSON.stringify({ source: "APP", ...payload }),
    }),

  // --- schedules ---
  listSchedules: (deviceId?: string) =>
    request<import("./types").ScheduledMessage[]>(
      `/api/v1/schedules${deviceId ? `?device_id=${deviceId}` : ""}`
    ),
  createSchedule: (payload: {
    device_id: string;
    message: string;
    scheduled_at: string;
    repeat_rule: import("./types").RepeatRule;
  }) =>
    request<import("./types").ScheduledMessage>("/api/v1/schedules", {
      method: "POST",
      body: JSON.stringify(payload),
    }),
  deleteSchedule: (id: string) =>
    request<void>(`/api/v1/schedules/${id}`, { method: "DELETE" }),
};
