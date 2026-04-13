import cmath

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

from bindings.qvis_engine_ext import sample
from backend.circuit import run_circuit_with_steps

app = FastAPI(title="QVis", version="0.1.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000"],
    allow_methods=["*"],
    allow_headers=["*"],
)


class GateOp(BaseModel):
    gate: str
    targets: list[int]


class StepSnapshot(BaseModel):
    label: str
    probabilities: dict[str, float]
    phases: dict[str, float]


class SampleRequest(BaseModel):
    circuit: list[GateOp]
    shots: int


class SampleResponse(BaseModel):
    counts: dict[str, int]
    steps: list[StepSnapshot]


def _bitstring(index: int, num_qubits: int) -> str:
    """Convert basis state index to little-endian bitstring (qubit 0 = rightmost)."""
    return "".join(
        str((index >> q) & 1) for q in range(num_qubits - 1, -1, -1)
    )


def _amplitudes_to_snapshot(label: str, amplitudes: list, num_qubits: int) -> StepSnapshot:
    """Convert a label and amplitude list to a StepSnapshot."""
    probabilities = {}
    phases = {}
    for i, amp in enumerate(amplitudes):
        bs = _bitstring(i, num_qubits)
        probabilities[bs] = abs(amp) ** 2
        phases[bs] = cmath.phase(amp)
    return StepSnapshot(label=label, probabilities=probabilities, phases=phases)


@app.get("/health")
def health():
    return {"status": "ok"}


@app.post("/api/sample", response_model=SampleResponse)
def api_sample(req: SampleRequest):
    circuit_dicts = [op.model_dump() for op in req.circuit]

    try:
        sv, raw_steps = run_circuit_with_steps(circuit_dicts)
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))

    n = sv.num_qubits()
    steps = [_amplitudes_to_snapshot(label, amps, n) for label, amps in raw_steps]
    counts = sample(sv, req.shots)

    return SampleResponse(counts=counts, steps=steps)


if __name__ == "__main__":
    import uvicorn

    uvicorn.run("backend.app:app", host="127.0.0.1", port=8000, reload=True)
