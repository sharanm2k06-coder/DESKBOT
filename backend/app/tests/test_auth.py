def test_register_and_login(client):
    r = client.post("/api/v1/auth/register", json={"email": "a@b.com", "password": "supersecret1"})
    assert r.status_code == 201
    assert r.json()["email"] == "a@b.com"

    r2 = client.post("/api/v1/auth/login", json={"email": "a@b.com", "password": "supersecret1"})
    assert r2.status_code == 200
    assert "access_token" in r2.json()


def test_duplicate_registration_rejected(client):
    client.post("/api/v1/auth/register", json={"email": "dup@b.com", "password": "supersecret1"})
    r = client.post("/api/v1/auth/register", json={"email": "dup@b.com", "password": "supersecret1"})
    assert r.status_code == 409


def test_login_wrong_password(client):
    client.post("/api/v1/auth/register", json={"email": "c@b.com", "password": "supersecret1"})
    r = client.post("/api/v1/auth/login", json={"email": "c@b.com", "password": "wrongpassword"})
    assert r.status_code == 401


def test_protected_route_requires_token(client):
    r = client.get("/api/v1/devices")
    assert r.status_code == 401
