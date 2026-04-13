#!/usr/bin/env bash
# run-demo.sh — Start the QVis backend and frontend with one command.
# Usage: ./demo/run-demo.sh   (run from the repo root)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

cleanup() {
    echo ""
    echo "Shutting down servers..."
    kill "$BACKEND_PID" "$FRONTEND_PID" 2>/dev/null || true
    wait "$BACKEND_PID" "$FRONTEND_PID" 2>/dev/null || true
    echo "Done."
}
trap cleanup EXIT INT TERM

# --- Backend (FastAPI on port 8000) ------------------------------------------
echo "Starting backend server..."
python -m backend.app &
BACKEND_PID=$!

# Wait for the backend to accept connections
for i in $(seq 1 30); do
    if curl -sf http://127.0.0.1:8000/health > /dev/null 2>&1; then
        break
    fi
    sleep 0.5
done

if ! curl -sf http://127.0.0.1:8000/health > /dev/null 2>&1; then
    echo "ERROR: Backend failed to start. Check that 'pip install -e .' was run."
    exit 1
fi
echo "Backend ready on http://127.0.0.1:8000"

# --- Frontend (Vite on port 3000) --------------------------------------------
echo "Starting frontend server..."
cd "$REPO_ROOT/frontend" && npm run dev &
FRONTEND_PID=$!
cd "$REPO_ROOT"

# Wait for Vite to start
sleep 2

echo ""
echo "============================================"
echo "  QVis is running at http://localhost:3000"
echo "  Press Ctrl+C to stop both servers."
echo "============================================"
echo ""

wait
