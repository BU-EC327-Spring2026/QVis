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


class SampleRequest(BaseModel):
    circuit: list[GateOp]
    shots: int


class SampleResponse(BaseModel):
    counts: dict[str, int]


GATE_REGISTRY = {
    "h": hadamard,
}


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

    for op in req.circuit:
        gate_fn = GATE_REGISTRY.get(op.gate)
        if gate_fn is None:
            raise HTTPException(status_code=400, detail=f"Unknown gate: {op.gate!r}")
        matrix = gate_fn()
        for target in op.targets:
            sv.apply(matrix, target)

    counts = sample(sv, req.shots)
    return SampleResponse(counts=counts)


if __name__ == "__main__":
    import uvicorn

    uvicorn.run("backend.app:app", host="127.0.0.1", port=8000, reload=True)
