import math
import pytest
import httpx
from backend.app import app


@pytest.mark.asyncio
async def test_sample_hadamard():
    transport = httpx.ASGITransport(app=app)
    async with httpx.AsyncClient(transport=transport, base_url="http://test") as client:
        resp = await client.post("/api/sample", json={
            "circuit": [{"gate": "h", "targets": [0]}],
            "shots": 1000,
        })

    assert resp.status_code == 200
    body = resp.json()

    counts = body["counts"]
    assert "0" in counts
    assert "1" in counts
    assert counts["0"] + counts["1"] == 1000

    steps = body["steps"]
    assert len(steps) == 2
    assert steps[0]["label"] == "Initial state"
    assert steps[1]["label"] == "H on qubit 0"

    for step in steps:
        total = sum(step["probabilities"].values())
        assert math.isclose(total, 1.0, abs_tol=1e-9)
        assert set(step["phases"].keys()) == set(step["probabilities"].keys())


@pytest.mark.asyncio
async def test_sample_bell_state():
    transport = httpx.ASGITransport(app=app)
    async with httpx.AsyncClient(transport=transport, base_url="http://test") as client:
        resp = await client.post("/api/sample", json={
            "circuit": [
                {"gate": "h", "targets": [0]},
                {"gate": "cx", "targets": [0, 1]},
            ],
            "shots": 1000,
        })

    assert resp.status_code == 200
    body = resp.json()

    counts = body["counts"]
    assert "01" not in counts
    assert "10" not in counts
    total = counts.get("00", 0) + counts.get("11", 0)
    assert total == 1000

    steps = body["steps"]
    assert len(steps) == 3
    assert steps[2]["label"] == "CX on qubits 0, 1"


@pytest.mark.asyncio
async def test_sample_unknown_gate():
    transport = httpx.ASGITransport(app=app)
    async with httpx.AsyncClient(transport=transport, base_url="http://test") as client:
        resp = await client.post("/api/sample", json={
            "circuit": [{"gate": "nonexistent", "targets": [0]}],
            "shots": 100,
        })

    assert resp.status_code == 400
    assert "Unknown gate" in resp.json()["detail"]
