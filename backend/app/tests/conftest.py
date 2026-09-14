import os

os.environ.setdefault("DATABASE_URL", "sqlite:///./test_deskbot.db")
os.environ.setdefault("JWT_SECRET", "test-secret")

import pytest
from fastapi.testclient import TestClient
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker

from app.database import Base, get_db
from app.main import app

TEST_DB_URL = "sqlite:///./test_deskbot.db"
engine = create_engine(TEST_DB_URL, connect_args={"check_same_thread": False})
TestingSessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)


@pytest.fixture(scope="function", autouse=True)
def _fresh_db():
    Base.metadata.create_all(bind=engine)
    yield
    Base.metadata.drop_all(bind=engine)


def _override_get_db():
    db = TestingSessionLocal()
    try:
        yield db
    finally:
        db.close()


app.dependency_overrides[get_db] = _override_get_db


@pytest.fixture()
def client():
    with TestClient(app) as c:
        yield c


@pytest.fixture()
def auth_headers(client):
    client.post("/api/v1/auth/register", json={"email": "user@example.com", "password": "password123"})
    resp = client.post("/api/v1/auth/login", json={"email": "user@example.com", "password": "password123"})
    token = resp.json()["access_token"]
    return {"Authorization": f"Bearer {token}"}


@pytest.fixture()
def registered_device(client, auth_headers):
    resp = client.post(
        "/api/v1/devices/register",
        json={"device_uid": "DESKBOT-01", "name": "Test DeskBot"},
        headers=auth_headers,
    )
    return resp.json()  # {id, device_uid, name, device_key}
