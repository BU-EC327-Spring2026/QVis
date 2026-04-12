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
    counts = resp.json()["counts"]
    assert "0" in counts
    assert "1" in counts
    assert counts["0"] + counts["1"] == 1000


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
