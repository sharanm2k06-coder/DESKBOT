"use client";

import { useEffect } from "react";
import { useRouter } from "next/navigation";
import { useAuth } from "@/lib/auth";

export default function AuthGate({ children }: { children: React.ReactNode }) {
  const { token, isLoading } = useAuth();
  const router = useRouter();

  useEffect(() => {
    if (!isLoading && !token) router.replace("/login");
  }, [isLoading, token, router]);

  if (isLoading || !token) {
    return (
      <div className="flex h-[60vh] items-center justify-center">
        <span className="label-tech animate-pulseDot">connecting to deskbot···</span>
      </div>
    );
  }

  return <>{children}</>;
}
