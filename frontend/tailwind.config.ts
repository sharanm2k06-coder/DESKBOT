import type { Config } from "tailwindcss";

// Design tokens — see frontend/README.md "Design plan" for the reasoning.
// Deliberately monochrome (matching the OLED it's a companion to), with
// color reserved only for functional status (online/alert), never decoration.
const config: Config = {
  content: ["./app/**/*.{ts,tsx}", "./components/**/*.{ts,tsx}"],
  theme: {
    extend: {
      colors: {
        void: "#08090A",       // page background
        panel: "#111315",      // card/panel background
        "panel-raised": "#17191C",
        line: "#2A2D30",       // default hairline border
        "line-bright": "#4A4E52",
        ink: "#EDEFF0",        // primary text
        "ink-dim": "#9A9FA3",  // secondary text
        "ink-faint": "#5C6165",
        signal: "#F5F7F8",     // the one "bright" tone — near-white, used sparingly
        online: "#7FDDA0",
        alert: "#E86A5C",
        important: "#E8C15C",
      },
      fontFamily: {
        mono: ["var(--font-jetbrains)", "ui-monospace", "SFMono-Regular", "monospace"],
        sans: ["var(--font-inter)", "ui-sans-serif", "system-ui", "sans-serif"],
      },
      borderRadius: {
        panel: "6px",
      },
      keyframes: {
        blink: {
          "0%, 92%, 100%": { transform: "scaleY(1)" },
          "96%": { transform: "scaleY(0.1)" },
        },
        pulseDot: {
          "0%, 100%": { opacity: "1" },
          "50%": { opacity: "0.35" },
        },
        scan: {
          "0%": { transform: "translateY(-100%)" },
          "100%": { transform: "translateY(100%)" },
        },
      },
      animation: {
        blink: "blink 5s infinite",
        pulseDot: "pulseDot 1.6s ease-in-out infinite",
        scan: "scan 3s linear infinite",
      },
    },
  },
  plugins: [],
};

export default config;
