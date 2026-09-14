const TONE = {
  online: "border-online/40 text-online",
  offline: "border-ink-faint/40 text-ink-faint",
  alert: "border-alert/50 text-alert",
  important: "border-important/50 text-important",
  neutral: "border-line-bright text-ink-dim",
} as const;

export default function StatusPill({
  children,
  tone = "neutral",
  pulse = false,
}: {
  children: React.ReactNode;
  tone?: keyof typeof TONE;
  pulse?: boolean;
}) {
  return (
    <span
      className={`inline-flex items-center gap-1.5 rounded-sm border px-2 py-0.5 font-mono text-[11px] tracking-wide ${TONE[tone]}`}
    >
      <span
        className={`h-1.5 w-1.5 rounded-full bg-current ${pulse ? "animate-pulseDot" : ""}`}
        aria-hidden="true"
      />
      {children}
    </span>
  );
}
