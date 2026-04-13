"""End-to-end test: send a Bell-state circuit through the full stack and verify
that the response reflects correct entanglement behavior."""

import math

import httpx
import pytest

from backend.app import app

BELL_CIRCUIT = {
    "circuit": [
        {"gate": "h", "targets": [0]},
        {"gate": "cx", "targets": [0, 1]},
    ],
    "shots": 1000,
}


@pytest.mark.asyncio
async def test_bell_state_e2e():
    transport = httpx.ASGITransport(app=app)
    async with httpx.AsyncClient(transport=transport, base_url="http://test") as client:
        resp = await client.post("/api/sample", json=BELL_CIRCUIT)

    assert resp.status_code == 200
    body = resp.json()

    # --- 3 steps: initial, H, CNOT -------------------------------------------
    steps = body["steps"]
    assert len(steps) == 3, f"Expected 3 steps, got {len(steps)}"

    # --- Final step: only |00⟩ and |11⟩ have non-zero probability -------------
    final_probs = steps[-1]["probabilities"]
    for bs, prob in final_probs.items():
        if bs in ("00", "11"):
            assert prob > 0.0, f"Expected non-zero probability for |{bs}⟩"
        else:
            assert math.isclose(prob, 0.0, abs_tol=1e-9), (
                f"Expected ~0 probability for |{bs}⟩, got {prob}"
            )

    # --- Counts: only "00" and "11" keys --------------------------------------
    counts = body["counts"]
    assert set(counts.keys()) == {"00", "11"}, (
        f"Expected counts only for '00' and '11', got {set(counts.keys())}"
    )
    assert counts["00"] + counts["11"] == BELL_CIRCUIT["shots"]
