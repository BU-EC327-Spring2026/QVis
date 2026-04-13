import * as d3 from "d3";

const CIRCUITS = [
  {
    name: "Coin Flip (Hadamard)",
    circuit: [{ gate: "h", targets: [0] }],
    shots: 1000,
    caption:
      "A Hadamard gate puts a qubit into a 50/50 superposition. After 1000 measurements, you should see roughly equal counts of 0 and 1.",
  },
  {
    name: "Bell State (Entanglement)",
    circuit: [
      { gate: "h", targets: [0] },
      { gate: "cx", targets: [0, 1] },
    ],
    shots: 1000,
    caption:
      "A Hadamard on qubit 0 creates a superposition, then a CNOT entangles qubits 0 and 1. The result is a Bell state: only |00⟩ and |11⟩ survive at 50% each — the qubits are perfectly correlated.",
  },
];

// --- Phase-to-color mapping --------------------------------------------------
// 0 rad = cyan, π/2 = green, π = red, -π/2 = purple
function phaseToColor(radians) {
  const hue = ((-radians * 180) / Math.PI + 180 + 360) % 360;
  return `hsl(${hue}, 85%, 55%)`;
}

// --- App shell ---------------------------------------------------------------

const app = document.querySelector("#app");

app.innerHTML = `
  <h1>QVis — Quantum Circuit Simulator</h1>
  <div id="setup">
    <select id="circuit-select">
      ${CIRCUITS.map((c, i) => `<option value="${i}">${c.name}</option>`).join("")}
    </select>
    <p id="caption"></p>
    <button id="run-btn">Run</button>
  </div>
  <div id="evolution" class="hidden">
    <h2 id="step-label"></h2>
    <div id="state-chart"></div>
    <div class="phase-legend">
      <span><i style="background:${phaseToColor(0)}"></i>0° (positive real)</span>
      <span><i style="background:${phaseToColor(Math.PI / 2)}"></i>90°</span>
      <span><i style="background:${phaseToColor(Math.PI)}"></i>180° (negative real)</span>
      <span><i style="background:${phaseToColor(-Math.PI / 2)}"></i>270°</span>
    </div>
    <div class="step-controls">
      <button id="step-back">Step Back</button>
      <button id="play-btn">Play</button>
      <button id="step-fwd">Step Forward</button>
    </div>
    <button id="measure-btn" class="hidden">Measure</button>
  </div>
  <div id="measurement" class="hidden">
    <h2>Measurement Results</h2>
    <div id="histogram"></div>
  </div>
`;

const select = document.querySelector("#circuit-select");
const captionEl = document.querySelector("#caption");
const runBtn = document.querySelector("#run-btn");
const setupDiv = document.querySelector("#setup");
const evolutionDiv = document.querySelector("#evolution");
const stepLabel = document.querySelector("#step-label");
const stateChart = document.querySelector("#state-chart");
const stepBackBtn = document.querySelector("#step-back");
const playBtn = document.querySelector("#play-btn");
const stepFwdBtn = document.querySelector("#step-fwd");
const measureBtn = document.querySelector("#measure-btn");
const measurementDiv = document.querySelector("#measurement");
const histogramDiv = document.querySelector("#histogram");

// --- Caption -----------------------------------------------------------------

function updateCaption() {
  captionEl.textContent = CIRCUITS[select.selectedIndex].caption;
}
select.addEventListener("change", updateCaption);
updateCaption();

// --- State -------------------------------------------------------------------

let steps = [];
let counts = {};
let currentStep = 0;
let playInterval = null;

// --- Run ---------------------------------------------------------------------

runBtn.addEventListener("click", async () => {
  const selected = CIRCUITS[select.selectedIndex];
  runBtn.disabled = true;
  runBtn.textContent = "Running…";

  try {
    const resp = await fetch("/api/sample", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        circuit: selected.circuit,
        shots: selected.shots,
      }),
    });

    if (!resp.ok) throw new Error(`Server error: ${resp.status}`);

    const body = await resp.json();
    steps = body.steps;
    counts = body.counts;
    currentStep = 0;

    setupDiv.classList.add("hidden");
    measurementDiv.classList.add("hidden");
    measureBtn.classList.add("hidden");
    evolutionDiv.classList.remove("hidden");

    initChart(steps[0]);
    renderStep(steps[0]);
    updateControls();
  } catch (err) {
    stateChart.innerHTML = `<p class="error">${err.message}</p>`;
  } finally {
    runBtn.disabled = false;
    runBtn.textContent = "Run";
  }
});

// --- Step controls -----------------------------------------------------------

stepBackBtn.addEventListener("click", () => {
  if (currentStep > 0) {
    currentStep--;
    renderStep(steps[currentStep]);
    updateControls();
  }
});

stepFwdBtn.addEventListener("click", () => {
  if (currentStep < steps.length - 1) {
    currentStep++;
    renderStep(steps[currentStep]);
    updateControls();
  }
});

playBtn.addEventListener("click", () => {
  if (playInterval) {
    stopPlay();
  } else {
    startPlay();
  }
});

measureBtn.addEventListener("click", () => {
  measureBtn.classList.add("hidden");
  measurementDiv.classList.remove("hidden");
  renderHistogram(counts);
});

function startPlay() {
  playBtn.textContent = "Pause";
  playInterval = setInterval(() => {
    if (currentStep < steps.length - 1) {
      currentStep++;
      renderStep(steps[currentStep]);
      updateControls();
    } else {
      stopPlay();
    }
  }, 1200);
}

function stopPlay() {
  clearInterval(playInterval);
  playInterval = null;
  playBtn.textContent = "Play";
}

function updateControls() {
  stepBackBtn.disabled = currentStep === 0;
  stepFwdBtn.disabled = currentStep === steps.length - 1;
  stepLabel.textContent = steps[currentStep].label;

  if (currentStep === steps.length - 1) {
    measureBtn.classList.remove("hidden");
  } else {
    measureBtn.classList.add("hidden");
    measurementDiv.classList.add("hidden");
  }
}

// --- State evolution chart (vertical bars) -----------------------------------

const margin = { top: 20, right: 20, bottom: 40, left: 50 };
const chartWidth = 520;
const chartHeight = 260;
const innerW = chartWidth - margin.left - margin.right;
const innerH = chartHeight - margin.top - margin.bottom;

let xScale, yScale, svg, gBars, gXAxis, gYAxis;

function initChart(step) {
  stateChart.innerHTML = "";

  const bitstrings = Object.keys(step.probabilities).sort();

  xScale = d3.scaleBand().domain(bitstrings).range([0, innerW]).padding(0.25);

  yScale = d3.scaleLinear().domain([0, 1]).range([innerH, 0]);

  svg = d3
    .select("#state-chart")
    .append("svg")
    .attr("width", chartWidth)
    .attr("height", chartHeight);

  const g = svg
    .append("g")
    .attr("transform", `translate(${margin.left},${margin.top})`);

  gXAxis = g
    .append("g")
    .attr("class", "axis")
    .attr("transform", `translate(0,${innerH})`)
    .call(d3.axisBottom(xScale));

  gYAxis = g.append("g").attr("class", "axis").call(
    d3.axisLeft(yScale).ticks(5).tickFormat(d3.format(".0%"))
  );

  gBars = g.append("g");
}

function renderStep(step) {
  const bitstrings = Object.keys(step.probabilities).sort();

  const data = bitstrings.map((bs) => ({
    bitstring: bs,
    probability: step.probabilities[bs],
    phase: step.phases[bs],
  }));

  // Data join
  const bars = gBars.selectAll("rect").data(data, (d) => d.bitstring);

  // Enter
  bars
    .enter()
    .append("rect")
    .attr("x", (d) => xScale(d.bitstring))
    .attr("width", xScale.bandwidth())
    .attr("y", innerH)
    .attr("height", 0)
    .attr("rx", 3)
    .merge(bars)
    .transition()
    .duration(600)
    .attr("y", (d) => yScale(d.probability))
    .attr("height", (d) => innerH - yScale(d.probability))
    .attr("fill", (d) => phaseToColor(d.phase));

  // Probability labels
  const labels = gBars.selectAll("text").data(data, (d) => d.bitstring);

  labels
    .enter()
    .append("text")
    .attr("class", "bar-label")
    .attr("text-anchor", "middle")
    .attr("x", (d) => xScale(d.bitstring) + xScale.bandwidth() / 2)
    .attr("y", innerH)
    .merge(labels)
    .transition()
    .duration(600)
    .attr("x", (d) => xScale(d.bitstring) + xScale.bandwidth() / 2)
    .attr("y", (d) => yScale(d.probability) - 6)
    .text((d) => (d.probability > 0.005 ? `${(d.probability * 100).toFixed(1)}%` : ""));
}

// --- Measurement histogram (horizontal bars) ---------------------------------

function renderHistogram(countsData) {
  const data = Object.entries(countsData)
    .map(([bitstring, count]) => ({ bitstring, count }))
    .sort((a, b) => a.bitstring.localeCompare(b.bitstring));

  const hMargin = { top: 10, right: 20, bottom: 10, left: 10 };
  const barHeight = 36;
  const width = 520;
  const height = data.length * barHeight + hMargin.top + hMargin.bottom;

  const maxCount = d3.max(data, (d) => d.count);

  const x = d3
    .scaleLinear()
    .domain([0, maxCount])
    .range([0, width - hMargin.left - hMargin.right - 120]);

  histogramDiv.innerHTML = "";

  const hSvg = d3
    .select("#histogram")
    .append("svg")
    .attr("width", width)
    .attr("height", height);

  const g = hSvg
    .append("g")
    .attr("transform", `translate(${hMargin.left},${hMargin.top})`);

  const bars = g
    .selectAll(".bar")
    .data(data)
    .enter()
    .append("g")
    .attr("class", "bar")
    .attr("transform", (_, i) => `translate(0,${i * barHeight})`);

  bars
    .append("text")
    .attr("x", 0)
    .attr("y", barHeight / 2)
    .attr("dy", "0.35em")
    .text((d) => d.bitstring);

  bars
    .append("rect")
    .attr("x", 30)
    .attr("y", 4)
    .attr("width", (d) => x(d.count))
    .attr("height", barHeight - 8);

  bars
    .append("text")
    .attr("x", (d) => 30 + x(d.count) + 8)
    .attr("y", barHeight / 2)
    .attr("dy", "0.35em")
    .text((d) => d.count);
}
