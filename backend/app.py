import cmath
import math

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

from bindings import StateVector
from bindings.qvis_engine_ext import hadamard, sample

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


GATE_REGISTRY = {
    "h": hadamard,
}


def _bitstring(index: int, num_qubits: int) -> str:
    """Convert basis state index to little-endian bitstring (qubit 0 = rightmost)."""
    return "".join(
        str((index >> q) & 1) for q in range(num_qubits - 1, -1, -1)
    )


def _snapshot(sv: StateVector, label: str) -> StepSnapshot:
    """Capture probabilities and phases for every basis state."""
    n = sv.num_qubits()
    dim = sv.dimension()
    probabilities = {}
    phases = {}
    for i in range(dim):
        bs = _bitstring(i, n)
        amp = sv[i]
        probabilities[bs] = abs(amp) ** 2
        phases[bs] = cmath.phase(amp)
    return StepSnapshot(label=label, probabilities=probabilities, phases=phases)


def _gate_label(op: GateOp) -> str:
    """Human-readable label for a gate operation."""
    name = op.gate.upper()
    qubits = ", ".join(str(t) for t in op.targets)
    if len(op.targets) == 1:
        return f"{name} on qubit {qubits}"
    return f"{name} on qubits {qubits}"


@app.get("/health")
def health():
    return {"status": "ok"}


@app.post("/api/sample", response_model=SampleResponse)
def api_sample(req: SampleRequest):
    # Infer qubit count from the circuit.
    max_qubit = 0
    for op in req.circuit:
        for t in op.targets:
            if t > max_qubit:
                max_qubit = t
    num_qubits = max_qubit + 1 if req.circuit else 1

    sv = StateVector(num_qubits)

    steps = [_snapshot(sv, "Initial state")]

    for op in req.circuit:
        gate_fn = GATE_REGISTRY.get(op.gate)
        if gate_fn is None:
            raise HTTPException(status_code=400, detail=f"Unknown gate: {op.gate!r}")
        matrix = gate_fn()
        for target in op.targets:
            sv.apply(matrix, target)
        steps.append(_snapshot(sv, _gate_label(op)))

    counts = sample(sv, req.shots)
    return SampleResponse(counts=counts, steps=steps)


if __name__ == "__main__":
    import uvicorn

    uvicorn.run("backend.app:app", host="127.0.0.1", port=8000, reload=True)
