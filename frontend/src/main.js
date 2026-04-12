import * as d3 from "d3";

const CIRCUITS = [
  {
    name: "Coin Flip (Hadamard)",
    circuit: [{ gate: "h", targets: [0] }],
    shots: 1000,
    caption:
      "A Hadamard gate puts a qubit into a 50/50 superposition. After 1000 measurements, you should see roughly equal counts of 0 and 1.",
  },
];

const app = document.querySelector("#app");

app.innerHTML = `
  <h1>QVis — Quantum Circuit Simulator</h1>
  <select id="circuit-select">
    ${CIRCUITS.map((c, i) => `<option value="${i}">${c.name}</option>`).join("")}
  </select>
  <p id="caption"></p>
  <button id="run-btn">Run</button>
  <div id="histogram"></div>
`;

const select = document.querySelector("#circuit-select");
const caption = document.querySelector("#caption");
const btn = document.querySelector("#run-btn");
const histogramDiv = document.querySelector("#histogram");

function updateCaption() {
  caption.textContent = CIRCUITS[select.selectedIndex].caption;
}

select.addEventListener("change", updateCaption);
updateCaption();

btn.addEventListener("click", async () => {
  const selected = CIRCUITS[select.selectedIndex];

  btn.disabled = true;
  btn.textContent = "Running…";
  histogramDiv.innerHTML = "";

  try {
    const resp = await fetch("/api/sample", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        circuit: selected.circuit,
        shots: selected.shots,
      }),
    });

    if (!resp.ok) {
      throw new Error(`Server error: ${resp.status}`);
    }

    const { counts } = await resp.json();
    renderHistogram(counts);
  } catch (err) {
    histogramDiv.innerHTML = `<p class="error">${err.message}</p>`;
  } finally {
    btn.disabled = false;
    btn.textContent = "Run";
  }
});

function renderHistogram(counts) {
  const data = Object.entries(counts)
    .map(([bitstring, count]) => ({ bitstring, count }))
    .sort((a, b) => a.bitstring.localeCompare(b.bitstring));

  const margin = { top: 10, right: 20, bottom: 10, left: 10 };
  const barHeight = 36;
  const width = 560;
  const height = data.length * barHeight + margin.top + margin.bottom;

  const maxCount = d3.max(data, (d) => d.count);

  const x = d3
    .scaleLinear()
    .domain([0, maxCount])
    .range([0, width - margin.left - margin.right - 120]);

  histogramDiv.innerHTML = "";

  const svg = d3
    .select("#histogram")
    .append("svg")
    .attr("width", width)
    .attr("height", height);

  const g = svg
    .append("g")
    .attr("transform", `translate(${margin.left},${margin.top})`);

  const bars = g
    .selectAll(".bar")
    .data(data)
    .enter()
    .append("g")
    .attr("class", "bar")
    .attr("transform", (_, i) => `translate(0,${i * barHeight})`);

  // Bitstring label to the left of the bar
  bars
    .append("text")
    .attr("x", 0)
    .attr("y", barHeight / 2)
    .attr("dy", "0.35em")
    .text((d) => d.bitstring);

  // Bar rectangle
  bars
    .append("rect")
    .attr("x", 30)
    .attr("y", 4)
    .attr("width", (d) => x(d.count))
    .attr("height", barHeight - 8);

  // Count label to the right of the bar
  bars
    .append("text")
    .attr("x", (d) => 30 + x(d.count) + 8)
    .attr("y", barHeight / 2)
    .attr("dy", "0.35em")
    .text((d) => d.count);
}
