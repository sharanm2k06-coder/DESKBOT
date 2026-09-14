"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { useAuth } from "@/lib/auth";

const LINKS = [
  { href: "/", label: "Dashboard" },
  { href: "/messages", label: "Messages" },
  { href: "/schedule", label: "Schedule" },
  { href: "/device", label: "Device" },
  { href: "/settings", label: "Settings" },
];

export default function NavBar() {
  const pathname = usePathname();
  const { token, logout } = useAuth();

  if (pathname === "/login") return null;

  return (
    <header className="sticky top-0 z-10 border-b border-line bg-void/90 backdrop-blur">
      <div className="mx-auto flex max-w-5xl items-center justify-between px-4 py-3 sm:px-6">
        <Link href="/" className="flex items-center gap-2.5">
          <span className="h-2 w-2 rounded-full bg-signal" aria-hidden="true" />
          <span className="font-mono text-[15px] tracking-wide text-ink">deskbot</span>
        </Link>

        <nav className="flex items-center gap-1 overflow-x-auto">
          {LINKS.map((link) => {
            const active = pathname === link.href;
            return (
              <Link
                key={link.href}
                href={link.href}
                className={`whitespace-nowrap rounded-sm px-3 py-1.5 font-mono text-[13px] transition-colors ${
                  active
                    ? "bg-panel-raised text-ink"
                    : "text-ink-dim hover:text-ink"
                }`}
              >
                {link.label}
              </Link>
            );
          })}
          {token && (
            <button
              onClick={logout}
              className="ml-1 whitespace-nowrap rounded-sm px-3 py-1.5 font-mono text-[13px] text-ink-faint hover:text-alert"
            >
              Log out
            </button>
          )}
        </nav>
      </div>
    </header>
  );
}
