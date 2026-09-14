"use client";

import { useState, FormEvent } from "react";
import { useAuth } from "@/lib/auth";
import { ApiError } from "@/lib/api";

export default function LoginPage() {
  const { login, register } = useAuth();
  const [mode, setMode] = useState<"login" | "register">("login");
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [busy, setBusy] = useState(false);

  const submit = async (e: FormEvent) => {
    e.preventDefault();
    setError(null);
    setBusy(true);
    try {
      if (mode === "login") await login(email, password);
      else await register(email, password);
    } catch (err) {
      setError(err instanceof ApiError ? err.message : "Something went wrong. Try again.");
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="mx-auto mt-12 max-w-sm">
      <div className="mb-8 text-center">
        <div className="mx-auto mb-3 h-2 w-2 rounded-full bg-signal" />
        <h1 className="font-mono text-lg tracking-wide text-ink">deskbot</h1>
        <p className="mt-1 text-sm text-ink-dim">
          {mode === "login" ? "Sign in to your control console." : "Create your DeskBot account."}
        </p>
      </div>

      <form onSubmit={submit} className="panel space-y-4 p-6">
        <div>
          <label htmlFor="email" className="label-tech mb-1.5 block">
            Email
          </label>
          <input
            id="email"
            type="email"
            required
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            className="w-full rounded-sm border border-line bg-panel-raised px-3 py-2 text-sm text-ink outline-none focus:border-line-bright"
            placeholder="you@example.com"
          />
        </div>
        <div>
          <label htmlFor="password" className="label-tech mb-1.5 block">
            Password
          </label>
          <input
            id="password"
            type="password"
            required
            minLength={8}
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            className="w-full rounded-sm border border-line bg-panel-raised px-3 py-2 text-sm text-ink outline-none focus:border-line-bright"
            placeholder="At least 8 characters"
          />
        </div>

        {error && <p className="text-sm text-alert">{error}</p>}

        <button
          type="submit"
          disabled={busy}
          className="w-full rounded-sm bg-signal py-2 font-mono text-sm text-void transition-opacity hover:opacity-90 disabled:opacity-50"
        >
          {busy ? "Working···" : mode === "login" ? "Sign in" : "Create account"}
        </button>

        <button
          type="button"
          onClick={() => {
            setError(null);
            setMode(mode === "login" ? "register" : "login");
          }}
          className="w-full text-center text-xs text-ink-faint hover:text-ink-dim"
        >
          {mode === "login" ? "Need an account? Register" : "Already have an account? Sign in"}
        </button>
      </form>
    </div>
  );
}
