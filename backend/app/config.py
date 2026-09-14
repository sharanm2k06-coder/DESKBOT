"""
DeskBot Backend — configuration.

Everything environment-specific comes from environment variables. Nothing
sensitive is hard-coded here. In local dev, values are loaded from a `.env`
file (see `.env.example`); in Render, they come from the dashboard / render.yaml.
"""
from functools import lru_cache
from typing import List

from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_file=".env", extra="ignore")

    # --- Database -----------------------------------------------------
    database_url: str = "sqlite:///./deskbot_dev.db"

    # --- Auth -----------------------------------------------------------
    jwt_secret: str = "CHANGE_ME_INSECURE_DEV_ONLY_SECRET"
    jwt_algorithm: str = "HS256"
    access_token_expire_minutes: int = 60 * 24  # 24h for user tokens

    # --- Device auth ------------------------------------------------------
    # Devices authenticate with a per-device API key issued at registration,
    # not a username/password or the user's JWT. Keeps ESP32 firmware simple
    # and means a leaked device key only exposes that one device.
    device_key_length: int = 40

    # --- CORS -------------------------------------------------------------
    cors_origins: str = "http://localhost:3000"

    # --- Scheduler --------------------------------------------------------
    scheduler_poll_interval_seconds: int = 30

    # --- Rate limiting ------------------------------------------------------
    rate_limit_per_minute: int = 120

    # --- Misc -------------------------------------------------------------
    environment: str = "development"

    @property
    def cors_origin_list(self) -> List[str]:
        return [o.strip() for o in self.cors_origins.split(",") if o.strip()]


@lru_cache
def get_settings() -> Settings:
    return Settings()
