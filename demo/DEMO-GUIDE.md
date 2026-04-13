# QVis Demo Guide

How to present QVis to a non-quantum audience in ~5 minutes.

## Before You Start

1. From the repo root, run:
   ```bash
   ./demo/run-demo.sh
   ```
2. Open **http://localhost:3000** in your browser.
3. Make sure the browser window is visible to the audience (projector, screen share, etc.).

## Part 1: Coin Flip — Superposition (~2 min)

### Setup

The dropdown should already show **"Coin Flip (Hadamard)"**. Read the caption aloud or paraphrase it.

**What to say:**

> "A classical bit is always 0 or 1. A qubit can be in both states at the same time — that's called superposition. We're about to put one qubit into a perfect 50/50 superposition and then measure it 1,000 times."

### Run it

1. Click **Run**.

### Walk through the state evolution

2. The first step shows **"All qubits start at |0>"** — a single bar at 100% for state 0.

   > "This is our starting point. The qubit is definitely in state 0, just like a classical bit."

3. Click **Step Forward**. The label changes to **"Put qubit 0 in superposition"** — two bars appear at 50% each.

   > "Now the qubit is in superposition. It's not 0 or 1 — it's both, with equal probability. The bar colors show the phase of each state — think of phase as a hidden angle that affects how states combine."

### Measure

4. Click **Measure**. A histogram appears with counts for 0 and 1.

   > "When we measure, superposition collapses. Each of the 1,000 measurements randomly lands on 0 or 1. You can see it's roughly 50/50 — like flipping a perfectly fair coin. But unlike a coin, this randomness is fundamental, not from imperfect flipping."

### Transition

> "That's one qubit. Now let's see what happens when two qubits talk to each other."

## Part 2: Bell State — Entanglement (~3 min)

### Setup

5. Refresh the page (or the app returns to the setup screen after you re-open it).
6. Select **"Bell State (Entanglement)"** from the dropdown. Read the caption.

**What to say:**

> "We're going to create the most famous quantum state: the Bell state. Two qubits become perfectly linked — measuring one instantly tells you the other. Einstein called this 'spooky action at a distance.'"

### Run it

7. Click **Run**.

### Walk through the state evolution

8. Step 1 — **"All qubits start at |0>"**: one bar at 100% for state 00.

   > "Both qubits start at 0. Four possible outcomes exist — 00, 01, 10, 11 — but right now only 00 has any probability."

9. Click **Step Forward**. Step 2 — **"Put qubit 0 in superposition"**: bars for 00 and 01 at 50% each.

   > "We put qubit 0 into superposition. Now we have a 50/50 mix of 00 and 01. Qubit 1 is still definitely 0 — the qubits are independent so far."

   *Note: the display shows 00 and 01 because qubit 0 is the rightmost bit in little-endian ordering, but you don't need to explain this to the audience — just focus on the probability split.*

10. Click **Step Forward**. Step 3 — **"Link the two qubits together"**: bars for 00 and 11 at 50% each; 01 and 10 drop to 0%.

    > "This is the magic step. We linked the two qubits with a CNOT gate. Now 01 and 10 are gone — only 00 and 11 survive. The qubits are entangled: if one is 0, the other must be 0. If one is 1, the other must be 1. There's no classical explanation for this correlation."

### Measure

11. Click **Measure**. The histogram shows counts split between 00 and 11 only.

    > "1,000 measurements, and every single one is either 00 or 11 — never 01 or 10. The two qubits always agree, even though each individual result is random. This is entanglement."

## Explaining the Visuals

If the audience asks about the charts, here's a quick reference:

| Visual element | What it shows |
|---|---|
| **Vertical bars (state evolution)** | Probability of each possible outcome. Taller bar = more likely to be measured. |
| **Bar colors** | Phase of the quantum state. Different colors mean different phases. When states combine, phase determines whether they add up or cancel out — like waves reinforcing or canceling. |
| **Phase legend** | Maps colors to angles: cyan = 0 degrees (positive), red = 180 degrees (negative). |
| **Horizontal bars (measurement)** | Actual counts from 1,000 simulated measurements. This is what you'd see from a real quantum computer. |

## Tips

- **Keep it visual.** Point at the bars as they change. The animation is the explanation.
- **Don't over-explain phase.** Mention that colors mean something, but don't derive the math unless asked.
- **Use the plain-English labels.** They're designed to be read aloud — "Put qubit 0 in superposition" is more useful than "H on qubit 0" for a general audience.
- **Contrast with classical.** The power of the demo is showing things a classical computer can't do naturally: true randomness (coin flip) and instant correlation (Bell state).
- **If someone asks "is this a real quantum computer?"** — No, it's a classical simulation of quantum behavior. Real quantum computers have noise and errors; this simulation is perfect. But the math is identical.
