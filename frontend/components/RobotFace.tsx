"use client";

type FaceState = "NORMAL" | "HAPPY" | "THINKING" | "ALERT" | "OFFLINE";

/**
 * A vector rendering of the same face vocabulary drawn on the physical
 * OLED (see firmware/DisplayManager.cpp) — same eye/mouth grammar, scaled
 * up. Kept intentionally simple (stroke-based, no fills) to read as an
 * extension of the device rather than a separate mascot.
 */
export default function RobotFace({ state = "NORMAL" }: { state?: FaceState }) {
  const eyeY = 44;

  return (
    <svg viewBox="0 0 200 100" className="h-28 w-56 sm:h-32 sm:w-64" aria-hidden="true">
      <rect x="1" y="1" width="198" height="98" rx="10" fill="none" stroke="#2A2D30" strokeWidth="1.5" />

      {state === "OFFLINE" ? (
        <text x="100" y="56" textAnchor="middle" className="fill-ink-faint font-mono" fontSize="13">
          offline
        </text>
      ) : (
        <>
          {/* Eyes */}
          {state === "ALERT" ? (
            <>
              <circle cx="66" cy={eyeY} r="9" fill="none" stroke="#EDEFF0" strokeWidth="3" />
              <circle cx="134" cy={eyeY} r="9" fill="none" stroke="#EDEFF0" strokeWidth="3" />
            </>
          ) : state === "THINKING" ? (
            <>
              <circle cx="66" cy={eyeY} r="3" fill="#EDEFF0" />
              <circle cx="134" cy={eyeY} r="3" fill="#EDEFF0" />
            </>
          ) : (
            <>
              <rect
                x="58" y={eyeY - 12} width="16" height="24" rx="8"
                fill="#EDEFF0" className="origin-center animate-blink"
                style={{ transformOrigin: "66px 44px" }}
              />
              <rect
                x="126" y={eyeY - 12} width="16" height="24" rx="8"
                fill="#EDEFF0" className="origin-center animate-blink"
                style={{ transformOrigin: "134px 44px" }}
              />
            </>
          )}

          {/* Mouth */}
          {state === "HAPPY" ? (
            <path d="M 75 68 Q 100 82 125 68" stroke="#EDEFF0" strokeWidth="3" fill="none" strokeLinecap="round" />
          ) : state === "ALERT" ? (
            <rect x="85" y="66" width="30" height="6" rx="3" fill="#EDEFF0" />
          ) : state === "THINKING" ? (
            <line x1="88" y1="70" x2="112" y2="70" stroke="#EDEFF0" strokeWidth="3" strokeLinecap="round" />
          ) : (
            <line x1="82" y1="70" x2="118" y2="70" stroke="#EDEFF0" strokeWidth="3" strokeLinecap="round" />
          )}
        </>
      )}
    </svg>
  );
}
