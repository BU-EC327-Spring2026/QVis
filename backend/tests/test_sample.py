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

    # Counts check
    counts = body["counts"]
    assert "0" in counts
    assert "1" in counts
    assert counts["0"] + counts["1"] == 1000

    # Steps check: initial state + 1 gate = 2 steps
    steps = body["steps"]
    assert len(steps) == 2

    # Initial state
    assert steps[0]["label"] == "Initial state"
    assert math.isclose(steps[0]["probabilities"]["0"], 1.0, abs_tol=1e-9)
    assert math.isclose(steps[0]["probabilities"]["1"], 0.0, abs_tol=1e-9)

    # After Hadamard
    assert steps[1]["label"] == "H on qubit 0"
    assert math.isclose(steps[1]["probabilities"]["0"], 0.5, abs_tol=1e-9)
    assert math.isclose(steps[1]["probabilities"]["1"], 0.5, abs_tol=1e-9)

    # Probabilities sum to 1
    for step in steps:
        total = sum(step["probabilities"].values())
        assert math.isclose(total, 1.0, abs_tol=1e-9)

    # Phases are present for all bitstrings
    for step in steps:
        assert set(step["phases"].keys()) == set(step["probabilities"].keys())


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
