export type DeviceStatus = "ONLINE" | "OFFLINE" | "UNKNOWN";

export interface Device {
  id: string;
  device_uid: string;
  name: string;
  status: DeviceStatus;
  firmware_version: string | null;
  last_seen: string | null;
  created_at: string;
}

export interface DeviceRegisterResponse extends Pick<Device, "id" | "device_uid"> {
  name: string;
  device_key: string;
}

export type MessageSource = "APP" | "WHATSAPP" | "SYSTEM" | "SCHEDULED";
export type MessagePriority = "NORMAL" | "IMPORTANT" | "ALERT";
export type MessageStatus = "PENDING" | "DELIVERED" | "ACKED" | "READ" | "EXPIRED";

export interface Message {
  id: string;
  device_id: string;
  source: MessageSource;
  sender: string | null;
  title: string | null;
  body: string;
  priority: MessagePriority;
  status: MessageStatus;
  created_at: string;
  delivered_at: string | null;
  read_at: string | null;
  expires_at: string | null;
}

export type RepeatRule = "ONCE" | "DAILY" | "WEEKDAYS" | "WEEKLY";

export interface ScheduledMessage {
  id: string;
  device_id: string;
  message: string;
  scheduled_at: string;
  repeat_rule: RepeatRule;
  enabled: boolean;
  last_fired_at: string | null;
  created_at: string;
}
