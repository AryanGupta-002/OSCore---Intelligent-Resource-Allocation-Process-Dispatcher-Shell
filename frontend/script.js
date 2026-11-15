/* =========================================================
   OSCORE — FRONTEND INTERACTIONS
   Vanilla JavaScript only. No React required.

   The frontend is served by Flask and uses the existing API
   routes. Results are rendered from data persisted by the C engine.
========================================================= */

"use strict";

/* ------------------------- API CONFIG ------------------------- */

const API_BASE_URL = "";

const API_ENDPOINTS = {
    workloads: "/api/workloads",
    runSimulation: "/api/run",
    simulationHistory: "/api/runs",
    deleteSimulationHistory: "/api/runs",
    simulationDetails: (runId) =>
        `/api/runs/${encodeURIComponent(runId)}/results`
};

/* ------------------------- APP STATE ------------------------- */

const state = {
    selectedWorkload: null,
    uploadedFile: null,
    selectedTestbed: null,
    selectedAlgorithm: null,
    latestRun: null,
    history: [],
    chartInstances: {},
    availableWorkloads: [],
    executionMode: null,
    selectedAlgorithms: [],
    timeQuantum: null,
    batchResults: [],
    batchChartRenderVersion: 0,
    isRunning: false
};

const disabledBeforeRun = new WeakMap();

const STANDARD_ALGORITHMS = [
    "FCFS",
    "SJF",
    "SRTF",
    "PRIORITY",
    "RR",
    "MLFQ"
];

const IMPLEMENTED_ALGORITHMS = [
    "HOST",
    ...STANDARD_ALGORITHMS
];

const $ = (selector, root = document) => root.querySelector(selector);
const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];

function apiUrl(path) {
    return `${API_BASE_URL}${path}`;
}

function escapeHtml(value) {
    return String(value ?? "").replace(/[&<>"']/g, (char) => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;"
    })[char]);
}

function showToast(message, type = "info") {
    let toast = $("#oscoreToast");

    if (!toast) {
        toast = document.createElement("div");
        toast.id = "oscoreToast";
        toast.className = "oscore-toast";
        toast.setAttribute("role", "status");
        document.body.appendChild(toast);
    }

    toast.textContent = message;
    toast.dataset.type = type;
    toast.classList.add("visible");

    clearTimeout(showToast.timeout);
    showToast.timeout = setTimeout(
        () => toast.classList.remove("visible"),
        4200
    );
}

async function requestJson(path, options = {}) {
    const response = await fetch(apiUrl(path), options);
    const contentType = response.headers.get("content-type") || "";

    if (!response.ok) {
        let detail = `Request failed (${response.status})`;

        try {
            const body = contentType.includes("application/json")
                ? await response.json()
                : await response.text();

            const message = typeof body === "string"
                ? body
                : body.message || body.error || body.detail || detail;
            const extra = typeof body === "object"
                ? body.details || body.output
                : null;
            detail = [message, extra].filter(Boolean).join(": ");
        } catch (_) {}

        throw new Error(detail);
    }

    if (!contentType.includes("application/json")) {
        throw new Error(
            "The backend response was not JSON. Check the Flask API route."
        );
    }

    return response.json();
}

/* ------------------------- EXPLORE POPUPS ------------------------- */

const EXPLORE_CONTENT = {
    "OSCore Working": {
        eyebrow: "PLATFORM OVERVIEW",
        title: "How OSCore works",
        intro: "OSCore brings workload input, process scheduling, memory allocation and finite I/O resource management into one operating-system simulation environment.",
        sections: [
            ["1. Provide a workload",
                "Load a CSV file containing process arrival time, priority, CPU burst, memory requirement and I/O device requirements."],
            ["2. Choose an execution mode",
                "Run the workload through the HOST testbed or select one of the standalone scheduling algorithms for comparison."],
            ["3. Execute and observe",
                "The C engine performs the simulation and produces process-level results, scheduling information and resource decisions."],
            ["4. Analyse and retain results",
                "The backend stores supported run data in PostgreSQL and presents the returned results through tables, charts and history views."]
        ],
        note: "OSCore is a simulator and testbed. It models operating-system concepts; it does not replace or modify the computer's actual operating-system kernel."
    },

    "HOST Dispatcher": {
        eyebrow: "PROCESS DISPATCHING",
        title: "HOST Dispatcher",
        intro: "HOST (Hypothetical Operating System Testbed) is the project's predefined simulation environment for process admission, priority-based dispatching and resource management.",
        sections: [
            ["Four priority levels",
                "The current design uses a real-time priority level and three user-process priority levels. The dispatcher selects eligible ready processes according to HOST's priority rules."],
            ["Admission and resource checks",
                "Before a process is admitted, the simulator checks the configured memory and finite I/O resource requirements."],
            ["Priority management",
                "The dispatcher applies the implemented priority handling and demotion rules as processes are managed."],
            ["Execution and cleanup",
                "The simulation records relevant events and releases allocated resources when a process finishes or is rejected, according to the implemented flow."]
        ],
        note: "HOST is a project-defined simulation policy. Its exact dispatching behaviour should be understood from the current C implementation, not assumed to be identical to a standard production OS scheduler."
    },

    "Scheduling Algorithms": {
        eyebrow: "CPU SCHEDULING",
        title: "Scheduling algorithms",
        intro: "Run a workload using different CPU scheduling policies and compare how their decisions affect process waiting time, turnaround time and execution order.",
        sections: [
            ["FCFS — First Come, First Served",
                "Processes are selected in order of arrival. It is straightforward but can make short jobs wait behind a long job."],
            ["SJF — Shortest Job First",
                "Among eligible processes, the process with the shortest CPU burst is selected. The non-preemptive form runs the selected process until completion."],
            ["SRTF — Shortest Remaining Time First",
                "A preemptive form of shortest-job scheduling. A newly available process can take the CPU if its remaining burst is shorter."],
            ["Priority Scheduling",
                "The next process is selected according to its priority value and the priority convention used by the implementation."],
            ["RR — Round Robin",
                "Ready processes receive CPU time in turns using a configurable time quantum."]
        ],
        note: "The standalone algorithms are separate from HOST's four-level dispatcher. Confirm the priority-number convention and RR quantum used by the current C engine."
    },

    "Resource Allocation": {
        eyebrow: "FINITE I/O RESOURCES",
        title: "Resource allocation",
        intro: "The resource manager represents a machine with a limited number of I/O devices. Each process declares the device quantities it needs, and the simulator checks whether those resources are available.",
        sections: [
            ["Configured device pool",
                "The current HOST configuration includes 2 printers, 1 scanner, 1 modem and 2 CD drives."],
            ["Availability checking",
                "A process request is checked against the remaining capacity in each device pool."],
            ["Allocation and release",
                "Resources are accounted for during process execution and released during the relevant cleanup path."],
            ["Admission outcomes",
                "Insufficient resources can affect whether a process is admitted. The simulation results should make completed and rejected processes distinguishable."]
        ],
        note: "These are simulated resource capacities. They do not represent physical devices connected to the user's computer."
    },

    "Memory Allocation": {
        eyebrow: "MEMORY MANAGEMENT",
        title: "Memory allocation",
        intro: "OSCore models memory as a collection of free and allocated blocks. Its memory manager uses a linked-list representation and the First Fit allocation strategy.",
        sections: [
            ["HOST memory configuration",
                "The current configuration has 1024 MB total memory, with 64 MB reserved for real-time processes and 960 MB available to user processes."],
            ["First Fit",
                "The allocator scans the free-block list and selects the first block large enough for the requested allocation."],
            ["Splitting blocks",
                "If a free block is larger than the request, the remaining space is represented as another free block."],
            ["Freeing and merging",
                "When memory is released, adjacent free blocks can be merged to reduce external fragmentation."]
        ],
        note: "The simulator demonstrates allocation behaviour. Its memory model is not a measurement of actual RAM usage by the browser or operating system."
    },

    "Analysis": {
        eyebrow: "RESULTS & INSIGHTS",
        title: "Simulation analysis",
        intro: "The analysis area is designed to turn simulation output into evidence that can be inspected, compared and explained.",
        sections: [
            ["Execution timeline",
                "A Gantt chart can show the order and duration of process execution when the backend returns valid start and finish times."],
            ["Process metrics",
                "Process-level results can be used to inspect arrival, burst, completion, waiting and turnaround times."],
            ["Algorithm comparison",
                "Runs using the same workload can be compared using average waiting time, average turnaround time, CPU utilisation and throughput where those values are available."],
            ["Resource behaviour",
                "HOST results can be examined for admitted and rejected processes, memory allocation and I/O device usage when the relevant records are persisted."]
        ],
        note: "Charts and summary values are populated from actual simulation responses or database records. No sample performance figures are presented as real results."
    }
};

function ensureInfoDialog() {
    let dialog = $("#exploreInfoDialog");

    if (dialog) return dialog;

    dialog = document.createElement("dialog");
    dialog.id = "exploreInfoDialog";
    dialog.className = "modal-dialog info-dialog";

    dialog.innerHTML = `
        <div class="modal-header">
            <div>
                <p class="eyebrow" id="infoEyebrow"></p>
                <h2 id="infoTitle"></h2>
            </div>
            <button type="button"
                class="icon-button"
                id="closeInfoDialog"
                aria-label="Close information">×</button>
        </div>

        <p class="info-intro" id="infoIntro"></p>
        <div class="info-sections" id="infoSections"></div>
        <p class="info-note" id="infoNote"></p>
    `;

    document.body.appendChild(dialog);

    $("#closeInfoDialog", dialog).addEventListener(
        "click",
        () => dialog.close()
    );

    dialog.addEventListener("click", (event) => {
        if (event.target === dialog) dialog.close();
    });

    return dialog;
}

function openInfoPopup(topic) {
    const content = EXPLORE_CONTENT[topic];

    if (!content) return;

    const dialog = ensureInfoDialog();

    $("#infoEyebrow", dialog).textContent = content.eyebrow;
    $("#infoTitle", dialog).textContent = content.title;
    $("#infoIntro", dialog).textContent = content.intro;

    $("#infoSections", dialog).innerHTML =
        content.sections.map(([heading, body]) => `
            <article class="info-section-item">
                <h3>${escapeHtml(heading)}</h3>
                <p>${escapeHtml(body)}</p>
            </article>
        `).join("");

    $("#infoNote", dialog).textContent = content.note;

    if (!dialog.open) dialog.showModal();
}

function setupExplorePopups() {
    $$(".explore-nav a").forEach((link) => {
        const label = link.textContent.trim();

        const topic = Object.keys(EXPLORE_CONTENT)
            .find((key) => label === key);

        if (topic) {
            link.href = "#";
            link.setAttribute("aria-haspopup", "dialog");

            link.addEventListener("click", () => {
                openInfoPopup(topic);
            });
        }
    });
}

/* ------------------------- CSV PARSING & PREVIEW ------------------------- */

function parseCsv(text) {
    const rows = [];
    let row = [];
    let field = "";
    let quoted = false;

    for (let i = 0; i < text.length; i += 1) {
        const char = text[i];
        const next = text[i + 1];

        if (char === '"' && quoted && next === '"') {
            field += '"';
            i += 1;
        } else if (char === '"') {
            quoted = !quoted;
        } else if (char === "," && !quoted) {
            row.push(field.trim());
            field = "";
        } else if (
            (char === "\n" || char === "\r") &&
            !quoted
        ) {
            if (char === "\r" && next === "\n") i += 1;

            row.push(field.trim());

            if (row.some((cell) => cell !== "")) {
                rows.push(row);
            }

            row = [];
            field = "";
        } else {
            field += char;
        }
    }

    row.push(field.trim());

    if (row.some((cell) => cell !== "")) {
        rows.push(row);
    }

    return rows;
}

function validateWorkloadHeaders(rows) {
    if (!rows.length) {
        return {
            valid: false,
            message: "The CSV file is empty."
        };
    }

    const expected = [
        "arrival",
        "priority",
        "cpu",
        "memory",
        "printers",
        "scanners",
        "modems",
        "cds"
    ];

    const headers = rows[0].map((header) =>
        header.toLowerCase().replace(/^\uFEFF/, "")
    );

    const missing = expected.filter(
        (column) => !headers.includes(column)
    );

    if (missing.length) {
        return {
            valid: false,
            message: `Missing required column${missing.length > 1 ? "s" : ""}: ${missing.join(", ")}`
        };
    }

    if (rows.length < 2) {
        return {
            valid: false,
            message: "The CSV contains headers but no process rows."
        };
    }

    return {
        valid: true,
        message: `${rows.length - 1} process row(s) detected.`
    };
}

function renderCsvTable(rows) {
    if (!rows.length) {
        return "<p class='helper-text'>No CSV data to display.</p>";
    }

    const [headers, ...body] = rows;

    return `
        <div class="csv-table-wrap">
            <table class="csv-table">
                <thead>
                    <tr>
                        ${headers.map((cell) =>
                            `<th>${escapeHtml(cell)}</th>`
                        ).join("")}
                    </tr>
                </thead>

                <tbody>
                    ${body.slice(0, 30).map((cells) => `
                        <tr>
                            ${headers.map((_, index) =>
                                `<td>${escapeHtml(cells[index] ?? "")}</td>`
                            ).join("")}
                        </tr>
                    `).join("")}
                </tbody>
            </table>
        </div>

        ${body.length > 30
            ? `<p class="helper-text">Showing first 30 of ${body.length} process rows.</p>`
            : ""}
    `;
}

function ensureCsvDialog() {
    let dialog = $("#csvPreviewDialog");

    if (!dialog) {
        dialog = document.createElement("dialog");
        dialog.id = "csvPreviewDialog";
        dialog.className = "modal-dialog";

        dialog.innerHTML = `
            <div class="modal-header">
                <div>
                    <p class="eyebrow">WORKLOAD INSPECTION</p>
                    <h2>CSV Preview</h2>
                </div>

                <button type="button"
                    id="closeCsvPreviewBtn"
                    class="icon-button"
                    aria-label="Close">×</button>
            </div>

            <div id="csvPreviewContent" class="csv-preview-content"></div>
        `;

        document.body.appendChild(dialog);
    }

    const close = $("#closeCsvPreviewBtn", dialog);

    if (close && !close.dataset.bound) {
        close.dataset.bound = "true";
        close.addEventListener("click", () => dialog.close());
    }

    if (!dialog.dataset.bound) {
        dialog.dataset.bound = "true";

        dialog.addEventListener("click", (event) => {
            if (event.target === dialog) dialog.close();
        });
    }

    return dialog;
}

function openCsvPreview(rows, filename) {
    const dialog = ensureCsvDialog();
    const validation = validateWorkloadHeaders(rows);

    $("#csvPreviewContent", dialog).innerHTML = `
        <div class="csv-validation ${validation.valid ? "valid" : "invalid"}">
            <strong>
                ${validation.valid
                    ? "CSV structure recognised"
                    : "CSV needs attention"}
            </strong>
            <span>${escapeHtml(validation.message)}</span>
        </div>

        <p class="csv-filename">${escapeHtml(filename)}</p>

        ${renderCsvTable(rows)}
    `;

    if (!dialog.open) dialog.showModal();
}

async function readLocalCsv(file) {
    const text = await file.text();
    const rows = parseCsv(text);
    const validation = validateWorkloadHeaders(rows);

    state.uploadedFile = file;

    state.selectedWorkload = {
        type: "upload",
        name: file.name,
        rows,
        valid: validation.valid
    };

    renderSelectedFile();
    updateSelectionSummary();

    if (!validation.valid) {
        showToast(validation.message, "error");
    } else {
        showToast(
            `${file.name} loaded. ${validation.message}`,
            "success"
        );
    }
}

function renderSelectedFile() {
    const list = $("#workloadFileList");

    if (!list) return;

    if (!state.selectedWorkload) {
        list.innerHTML = `
            <p class="helper-text">
                Choose or upload a CSV workload to see it here.
            </p>
        `;
        return;
    }

    list.innerHTML = `
        <div class="file-list-item selected-file">
            <span class="file-name">
                ${escapeHtml(state.selectedWorkload.name)}
            </span>
            <span class="file-status">
                ${state.selectedWorkload.valid ? "Selected" : "Check CSV"}
            </span>
        </div>
    `;

    const previewButton = $("#previewCsvBtn");

    if (previewButton) {
        previewButton.disabled = !state.selectedWorkload.rows;
    }
}

async function loadAvailableWorkloads() {
    const select = $("#workloadSelect");

    if (!select) return;

    try {
        const payload = await requestJson(API_ENDPOINTS.workloads);

        const files = Array.isArray(payload)
            ? payload
            : (payload.workloads || payload.files || payload.data || []);

        state.availableWorkloads = files;

        select.innerHTML = `
            <option value="">Select an available CSV file</option>
        `;

        files.forEach((item, index) => {
            const name = typeof item === "string"
                ? item
                : (
                    item.filename ||
                    item.name ||
                    item.source_filename ||
                    `Workload ${index + 1}`
                );

            const value = typeof item === "string"
                ? item
                : (
                    item.id ??
                    item.workload_id ??
                    item.path ??
                    name
                );

            const option = document.createElement("option");

            option.value = String(value);
            option.textContent = name;

            select.appendChild(option);
        });

        const list = $("#workloadFileList");
        if (!files.length) {
            select.innerHTML = `
                <option value="">No saved workloads found</option>
            `;
            if (list) {
                list.innerHTML = `
                    <p class="helper-text">No CSV workloads are available. Upload a CSV file to continue.</p>
                `;
            }
        } else if (list) {
            list.innerHTML = `
                <p class="helper-text">${files.length} CSV workload(s) available. Choose one above to inspect it.</p>
            `;
        }

    } catch (error) {
        select.innerHTML = `
            <option value="">Upload a CSV file to continue</option>
        `;

        const list = $("#workloadFileList");

        if (list) {
            list.innerHTML = `
                <p class="helper-text">
                    Could not load saved workloads from Flask:
                    ${escapeHtml(error.message)}. You can still upload a CSV file.
                </p>
            `;
        }
    }
}

/* ------------------------- EXECUTION MODE & SELECTION ------------------------- */

function clearAllSelections() {
    $$("[data-testbed], [data-algorithm]").forEach((button) => {
        button.classList.remove("selected");
        button.setAttribute("aria-pressed", "false");
    });

    const runAllButton = $("#runAllStandardBtn");
    runAllButton?.classList.remove("selected");
    runAllButton?.setAttribute("aria-pressed", "false");

    $("#customMachineConfig")?.classList.add("hidden");
    $("#rrQuantumPanel")?.classList.add("hidden");
    state.selectedAlgorithms = [];
    state.selectedTestbed = null;
    state.selectedAlgorithm = null;
}

function updateAlgorithmSelection() {
    $$("[data-testbed='host']").forEach((button) => {
        const selected = state.selectedTestbed === "host";
        button.classList.toggle("selected", selected);
        button.setAttribute("aria-pressed", String(selected));
    });

    $$("[data-testbed='custom']").forEach((button) => {
        const selected = state.selectedTestbed === "custom";
        button.classList.toggle("selected", selected);
        button.setAttribute("aria-pressed", String(selected));
    });

    $$("[data-algorithm]").forEach((button) => {
        const selected = state.selectedAlgorithms.includes(button.dataset.algorithm);
        button.classList.toggle("selected", selected);
        button.setAttribute("aria-pressed", String(selected));
    });

    if (!["host", "custom"].includes(state.selectedTestbed)) {
        state.selectedTestbed = null;
    }
    state.selectedAlgorithm = state.selectedAlgorithms.find((algorithm) =>
        STANDARD_ALGORITHMS.includes(algorithm)
    ) || null;
    state.executionMode = state.selectedTestbed
        ? state.selectedTestbed
        : state.selectedAlgorithms.length
            ? "algorithms"
            : null;
    state.timeQuantum = state.selectedAlgorithms.includes("RR")
        ? readTimeQuantum()
        : null;
    $("#rrQuantumPanel")?.classList.toggle(
        "hidden",
        !state.selectedAlgorithms.includes("RR")
    );
    $("#customMachineConfig")?.classList.toggle(
        "hidden",
        state.selectedTestbed !== "custom"
    );
    const allSelected = state.selectedAlgorithms.length === STANDARD_ALGORITHMS.length &&
        STANDARD_ALGORITHMS.every((algorithm) =>
            state.selectedAlgorithms.includes(algorithm)
        );
    const runAllButton = $("#runAllStandardBtn");
    runAllButton?.classList.toggle("selected", allSelected);
    runAllButton?.setAttribute("aria-pressed", String(allSelected));
    if (runAllButton) {
        runAllButton.textContent = allSelected
            ? "All 6 algorithms selected"
            : "Select all 6 algorithms";
    }
    updateSelectionSummary();
}

function toggleAlgorithmSelection(algorithm) {
    if (!IMPLEMENTED_ALGORITHMS.includes(algorithm)) return;

    if (algorithm === "HOST") {
        state.selectedAlgorithms = ["HOST"];
        state.selectedTestbed = "host";
        updateAlgorithmSelection();
        return;
    }

    if (state.selectedTestbed) {
        state.selectedAlgorithms = [];
        state.selectedTestbed = null;
    }

    const allSchedulingAlgorithmsSelected =
        state.selectedAlgorithms.length === STANDARD_ALGORITHMS.length &&
        STANDARD_ALGORITHMS.every((selected) =>
            state.selectedAlgorithms.includes(selected)
        );

    if (allSchedulingAlgorithmsSelected) {
        state.selectedAlgorithms = [algorithm];
        updateAlgorithmSelection();
        return;
    }

    if (state.selectedAlgorithms.includes(algorithm)) {
        state.selectedAlgorithms = state.selectedAlgorithms.filter(
            (selected) => selected !== algorithm
        );
    } else {
        state.selectedAlgorithms = [...state.selectedAlgorithms, algorithm];
    }

    updateAlgorithmSelection();
}

function selectTestbed(testbed) {
    if (!["host", "custom"].includes(testbed)) {
        return;
    }

    state.selectedAlgorithms = ["HOST"];
    state.selectedTestbed = testbed;
    updateAlgorithmSelection();
}

function selectAlgorithm(algorithm) {
    if (!STANDARD_ALGORITHMS.includes(algorithm)) {
        showToast(`${algorithm} is not available in the current backend.`, "error");
        return;
    }

    if (state.selectedTestbed) {
        state.selectedAlgorithms = [];
        state.selectedTestbed = null;
    }
    toggleAlgorithmSelection(algorithm);
    if (algorithm === "RR" && state.selectedAlgorithms.includes("RR")) {
        syncTimeQuantum();
    }
}

function readTimeQuantum() {
    const input = $("#rrTimeQuantum");
    const value = Number(input?.value);

    return input?.value.trim() && Number.isInteger(value) && value > 0
        ? value
        : null;
}

function syncTimeQuantum() {
    state.timeQuantum = readTimeQuantum();
    updateSelectionSummary();
}

function updateSelectionSummary() {
    const summary = $("#selectionSummary");
    const continueButton = $("#continueToAnalysisBtn");

    const workloadReady = Boolean(
        state.selectedWorkload?.valid &&
        (
            state.selectedWorkload.type === "upload" ||
            state.selectedWorkload.id
        )
    );

    const customMachineConfig = state.selectedTestbed === "custom"
        ? readCustomMachineConfig()
        : null;
    const modeReady = state.selectedAlgorithms.length > 0 &&
        (!state.selectedAlgorithms.includes("RR") || state.timeQuantum !== null) &&
        (state.selectedTestbed !== "custom" || customMachineConfig !== null);

    if (summary) {
        const selected = state.selectedTestbed === "custom"
            ? "Custom machine"
            : state.selectedTestbed === "host"
                ? "HOST"
                : state.selectedAlgorithms.length
                    ? state.selectedAlgorithms.join(", ")
                    : "No algorithm selected";
        const quantum = state.selectedAlgorithms.includes("RR")
            ? ` • RR quantum: ${state.timeQuantum ?? "enter a positive integer"}`
            : "";
        summary.textContent = `${state.selectedWorkload?.name || "No workload selected"} • ${selected}${quantum}`;
    }

    if (continueButton) {
        continueButton.disabled =
            !workloadReady || !modeReady || state.isRunning;
        continueButton.innerHTML =
            state.selectedAlgorithms.length > 1
                ? `Run ${state.selectedAlgorithms.length} Selected Algorithms <span aria-hidden="true">→</span>`
                : `Run Selected Algorithm <span aria-hidden="true">→</span>`;
    }
}

function readCustomMachineConfig() {
    const fields = [
        ["total_memory_mb", "#machineMemory", 2],
        ["printers", "#machinePrinters", 0],
        ["scanners", "#machineScanners", 0],
        ["modems", "#machineModems", 0],
        ["cds", "#machineCds", 0]
    ];
    const config = {};

    for (const [key, selector, minimum] of fields) {
        const input = $(selector);
        const value = Number(input?.value);
        if (
            !input?.value.trim() ||
            !Number.isInteger(value) ||
            value < minimum ||
            value > 2147483647
        ) {
            return null;
        }
        config[key] = value;
    }

    return config;
}

/* ------------------------- SIMULATION REQUEST ------------------------- */

function setSimulationLoading(loading) {
    state.isRunning = loading;
    const button = $("#continueToAnalysisBtn");

    $$("[data-testbed], [data-algorithm], #runAllStandardBtn, #workloadSelect, #csvUpload, #rrTimeQuantum, #machineMemory, #machinePrinters, #machineScanners, #machineModems, #machineCds")
        .forEach((control) => {
            if (loading) {
                if (!disabledBeforeRun.has(control)) {
                    disabledBeforeRun.set(control, control.disabled);
                }
                control.disabled = true;
            } else {
                control.disabled = disabledBeforeRun.get(control) ?? false;
            }
        });

    if (button && loading) button.textContent = "Running simulation...";
    if (!loading) updateSelectionSummary();
}

function ensureAnalysisWorkspace() {
    const section = $("#analysis");

    if (!section) return null;

    let workspace = $("#analysisWorkspace");

    if (workspace) return workspace;

    workspace = document.createElement("div");

    workspace.id = "analysisWorkspace";
    workspace.className = "analysis-workspace";

    workspace.innerHTML = `
        <div class="analysis-toolbar">
            <div>
                <p class="eyebrow">LATEST EXECUTION</p>
                <h3 id="analysisRunTitle">Simulation results</h3>
                <p id="analysisRunMeta" class="helper-text">
                    Run a simulation to populate this workspace.
                </p>
            </div>

            <button type="button"
                id="refreshHistoryBtn"
                class="outline-button">
                Refresh history
            </button>
        </div>

        <div id="analysisMessage" class="analysis-message">
            No simulation results loaded yet.
            Run a workload to view real metrics and charts.
        </div>

        <div id="metricsGrid" class="metrics-grid"></div>

        <div class="charts-grid">

            <article class="chart-panel" id="executionTimelinePanel">
                <div class="chart-panel-heading">
                    <h3>Process execution timeline</h3>
                    <p>Gantt-style view of persisted CPU execution intervals.</p>
                </div>

                <div class="chart-canvas-wrap">
                    <canvas id="ganttChart"></canvas>
                </div>

                <p class="chart-empty" id="ganttEmpty">
                    Timeline will appear when process timing data is returned.
                </p>
            </article>

            <article class="chart-panel" id="performanceChartPanel">
                <div class="chart-panel-heading">
                    <h3>Process performance</h3>
                    <p>Arrival, CPU burst, completion, turnaround, waiting, and response time by process.</p>
                </div>

                <div class="chart-canvas-wrap">
                    <canvas id="processMetricsChart"></canvas>
                </div>

                <p class="chart-empty" id="processMetricsEmpty">
                    Process metrics will appear when available.
                </p>
            </article>

            <article class="chart-panel" id="processLineChartPanel">
                <div class="chart-panel-heading">
                    <h3>Process timing comparison</h3>
                    <p>Line chart comparing timing measures across every process.</p>
                </div>

                <div class="chart-canvas-wrap">
                    <canvas id="processTimelineChart"></canvas>
                </div>

                <p class="chart-empty" id="processTimelineEmpty">
                    Process timing data will appear when available.
                </p>
            </article>

            <article class="chart-panel" id="resourceChartPanel">
                <div class="chart-panel-heading">
                    <h3>Resource outcomes</h3>
                    <p>Completed and rejected process counts.</p>
                </div>

                <div class="chart-canvas-wrap small-chart">
                    <canvas id="resourceChart"></canvas>
                </div>

                <p class="chart-empty" id="resourceEmpty">
                    Resource outcome data will appear when available.
                </p>
            </article>

            <article class="chart-panel" id="processTablePanel">
                <div class="chart-panel-heading">
                    <h3>Process results</h3>
                    <p>Detailed data returned by the simulation engine.</p>
                </div>

                <div class="table-scroll" id="processResultsTable">
                    <p class="helper-text">No process results loaded.</p>
                </div>
            </article>

        </div>

        <div class="history-panel">
            <div class="history-heading-row">
                <div class="chart-panel-heading">
                    <h3>Simulation history</h3>
                    <p>Showing the 10 most recent saved runs.</p>
                </div>
                <button type="button" id="deleteHistoryBtn" class="outline-button">
                    Delete history
                </button>
            </div>

            <div class="table-scroll" id="simulationHistoryTable">
                <p class="helper-text">
                    History will appear when the history API is connected.
                </p>
            </div>
        </div>
    `;

    const previewGrid = $(".analysis-preview-grid", section);

    if (previewGrid) {
        previewGrid.insertAdjacentElement("afterend", workspace);
    } else {
        section.appendChild(workspace);
    }

    $("#refreshHistoryBtn", workspace).addEventListener(
        "click",
        loadSimulationHistory
    );
    $("#deleteHistoryBtn", workspace).addEventListener(
        "click",
        deleteSimulationHistory
    );

    return workspace;
}

function ensureBatchAnalysisViews() {
    const views = [
        ["executionTimelinePanel", "batchGanttCharts", null],
        ["performanceChartPanel", "batchAlgorithmMetricsChart", "canvas"],
        ["processLineChartPanel", "batchProcessMetricsChart", "canvas"],
        ["resourceChartPanel", "batchResourceChart", "canvas"]
    ];

    for (const [panelId, viewId, canvasId] of views) {
        const panel = $(`#${panelId}`);
        if (!panel) continue;

        let view = $(`#${viewId}`, panel);
        if (!view) {
            view = document.createElement("div");
            view.id = viewId;
            view.className = "chart-canvas-wrap batch-view";
            if (canvasId) {
                const canvas = document.createElement("canvas");
                canvas.id = viewId;
                canvas.classList.add("batch-chart-canvas");
                view.appendChild(canvas);
                view.id = `${viewId}Wrap`;
            }
            panel.querySelector(".chart-canvas-wrap")?.after(view);
        }
    }

    const processPanel = $("#processTablePanel");
    if (processPanel && !$("#batchProcessTables", processPanel)) {
        const batchTables = document.createElement("div");
        batchTables.id = "batchProcessTables";
        batchTables.className = "batch-view";
        const aggregateTable = document.createElement("div");
        aggregateTable.id = "batchAggregateComparison";
        aggregateTable.className = "table-scroll";
        const processComparison = document.createElement("div");
        processComparison.id = "processAlgorithmComparison";
        processComparison.className = "table-scroll";
        const detailTables = document.createElement("div");
        detailTables.id = "batchProcessDetailTables";
        batchTables.append(aggregateTable, processComparison, detailTables);
        $("#processResultsTable", processPanel)?.after(batchTables);
    }
}

function resetBatchAnalysisViews() {
    state.batchChartRenderVersion++;
    Object.keys(state.chartInstances)
        .filter((key) => key.startsWith("batch-"))
        .forEach(destroyChart);
    $("#batchGanttCharts") && ($("#batchGanttCharts").innerHTML = "");
    $("#batchAlgorithmMetricsChart") &&
        ($("#batchAlgorithmMetricsChart").replaceWith(
            Object.assign(document.createElement("canvas"), {
                id: "batchAlgorithmMetricsChart"
            })
        ));
    $("#batchProcessMetricsChart") &&
        ($("#batchProcessMetricsChart").replaceWith(
            Object.assign(document.createElement("canvas"), {
                id: "batchProcessMetricsChart"
            })
        ));
    $("#batchResourceChart") &&
        ($("#batchResourceChart").replaceWith(
            Object.assign(document.createElement("canvas"), {
                id: "batchResourceChart"
            })
        ));
    $("#batchAggregateComparison") &&
        ($("#batchAggregateComparison").innerHTML = "");
    $("#processAlgorithmComparison") &&
        ($("#processAlgorithmComparison").innerHTML = "");
    $("#batchProcessDetailTables") &&
        ($("#batchProcessDetailTables").innerHTML = "");

    const headings = [
        ["executionTimelinePanel", "Process execution timeline", "Gantt-style view of persisted CPU execution intervals."],
        ["performanceChartPanel", "Process performance", "Arrival, CPU burst, completion, turnaround, waiting, and response time by process."],
        ["processLineChartPanel", "Process timing comparison", "Line chart comparing timing measures across every process."],
        ["resourceChartPanel", "Resource outcomes", "Completed and rejected process counts."],
        ["processTablePanel", "Process results", "Detailed data returned by the simulation engine."]
    ];
    for (const [panelId, title, description] of headings) {
        const panel = $(`#${panelId}`);
        const heading = panel?.querySelector(".chart-panel-heading h3");
        const text = panel?.querySelector(".chart-panel-heading p");
        if (heading) heading.textContent = title;
        if (text) text.textContent = description;
    }
}

function firstDefined(object, keys) {
    for (const key of keys) {
        if (
            object &&
            object[key] !== undefined &&
            object[key] !== null
        ) {
            return object[key];
        }
    }

    return undefined;
}

function normaliseRun(payload) {
    const root = payload.result || payload.data || payload;

    return {
        run: root.run || root.simulation || root,
        metrics: root.metrics || root.performance_metrics || {},
        processes:
            root.processes ||
            root.process_results ||
            root.results ||
            [],
        events: root.events || root.execution_events || [],
        raw: root
    };
}

function metricCard(label, value, suffix = "") {
    if (
        value === undefined ||
        value === null ||
        value === ""
    ) {
        return "";
    }

    return `
        <article class="metric-card">
            <span>${escapeHtml(label)}</span>
            <strong>
                ${escapeHtml(value)}${escapeHtml(suffix)}
            </strong>
        </article>
    `;
}

function renderMetrics(runData) {
    const container = $("#metricsGrid");

    if (!container) return;

    const run = runData.run;
    const metrics = runData.metrics;

    const values = [
        [
            "Run ID",
            firstDefined(run, ["run_id", "id"])
        ],
        [
            "Algorithm",
            firstDefined(run, ["algorithm", "scheduler"])
        ],
        [
            "Completed processes",
            firstDefined(run, ["completed_processes", "completed"])
        ],
        [
            "Rejected processes",
            firstDefined(run, ["rejected_processes", "rejected"])
        ],
        [
            "CPU utilisation",
            firstDefined(metrics, ["cpu_utilization"]) ??
            firstDefined(run, ["cpu_utilization"]),
            "%"
        ],
        [
            "Average waiting time",
            firstDefined(metrics, [
                "average_waiting_time",
                "avg_waiting_time"
            ])
        ],
        [
            "Average turnaround time",
            firstDefined(metrics, [
                "average_turnaround_time",
                "avg_turnaround_time"
            ])
        ],
        [
            "Average response time",
            firstDefined(metrics, [
                "average_response_time",
                "avg_response_time"
            ])
        ],
        [
            "Total execution time",
            firstDefined(metrics, [
                "total_execution_time"
            ]) ?? firstDefined(run, ["total_execution_time"])
        ],
        [
            "Throughput",
            firstDefined(metrics, ["throughput"])
        ]
    ];

    container.innerHTML = values.map(
        ([label, value, suffix]) =>
            metricCard(label, value, suffix)
    ).join("");

    if (!container.innerHTML.trim()) {
        container.innerHTML = `
            <p class="helper-text">
                The API returned a run, but no recognised metric
                fields were found. Inspect the JSON response and
                map its field names in renderMetrics().
            </p>
        `;
    }
}

function renderProcessTable(processes) {
    const container = $("#processResultsTable");

    if (!container) return;

    if (!Array.isArray(processes) || !processes.length) {
        container.innerHTML = `
            <p class="helper-text">
                The backend did not return process-level results for this run.
            </p>
        `;
        return;
    }

    const columns = [
        ["Process", ["process_id", "pid", "id", "name"]],
        ["Arrival", ["arrival_time", "arrival"]],
        ["CPU burst", ["burst_time", "cpu_burst", "cpu"]],
        ["Start", ["start_time", "started_at"]],
        ["Completion", ["completion_time", "completed_at"]],
        ["Waiting", ["waiting_time"]],
        ["Turnaround", ["turnaround_time"]],
        ["Response", ["response_time"]],
        ["Final state", ["final_state", "status", "process_status"]]
    ];

    const available = columns.filter(([, keys]) =>
        processes.some((process) =>
            firstDefined(process, keys) !== undefined
        )
    );

    container.innerHTML = `
        <table class="data-table">
            <thead>
                <tr>
                    ${available.map(([label]) =>
                        `<th>${escapeHtml(label)}</th>`
                    ).join("")}
                </tr>
            </thead>

            <tbody>
                ${processes.map((process) => `
                    <tr>
                        ${available.map(([, keys]) =>
                            `<td>${escapeHtml(
                                firstDefined(process, keys) ?? "—"
                            )}</td>`
                        ).join("")}
                    </tr>
                `).join("")}
            </tbody>
        </table>
    `;
}

function destroyChart(key) {
    if (state.chartInstances[key]) {
        state.chartInstances[key].destroy();
        delete state.chartInstances[key];
    }
}

async function loadChartJs() {
    if (window.Chart) return;

    if (loadChartJs.promise) {
        return loadChartJs.promise;
    }

    loadChartJs.promise = new Promise((resolve, reject) => {
        const script = document.createElement("script");

        script.src = "https://cdn.jsdelivr.net/npm/chart.js";

        script.onload = resolve;

        script.onerror = () => reject(
            new Error(
                "Chart.js could not be loaded. Check your internet connection."
            )
        );

        document.head.appendChild(script);
    });

    return loadChartJs.promise;
}

function showChartEmpty(id, show) {
    const element = $(`#${id}`);

    if (element) {
        element.hidden = !show;
    }
}

function getProcessMetricChartData(processes) {
    const fields = [
        ["Arrival", "arrival_time", "#9ecae1"],
        ["CPU burst", "burst_time", "#f2b8c6"],
        ["Completion", "completion_time", "#c5b8e3"],
        ["Turnaround", "turnaround_time", "#f4c7a1"],
        ["Waiting", "waiting_time", "#b8ddc5"],
        ["Response", "response_time", "#a9d8dc"]
    ];

    return {
        labels: processes.map((process) =>
            `P${firstDefined(process, ["pid", "process_id", "id"]) ?? "?"}`
        ),
        datasets: fields.map(([label, key, color]) => ({
            label,
            borderColor: color,
            backgroundColor: color,
            data: processes.map((process) => {
                const value = firstDefined(process, [key]);
                return value === undefined ? null : Number(value);
            })
        })).filter((dataset) =>
            dataset.data.some((value) => Number.isFinite(value))
        )
    };
}

function processColor(pid) {
    const palette = [
        "#9ecae1", "#f2b8c6", "#b8ddc5",
        "#c5b8e3", "#f4c7a1", "#a9d8dc"
    ];
    const index = Math.abs(Number(pid) || 0) % palette.length;
    return palette[index];
}

async function renderBatchCharts(rows) {
    const renderVersion = ++state.batchChartRenderVersion;

    Object.keys(state.chartInstances)
        .filter((key) => key.startsWith("batch-"))
        .forEach(destroyChart);

    const ganttContainer = $("#batchGanttCharts");
    const metricsCanvas = $("#batchAlgorithmMetricsChart");
    const processCanvas = $("#batchProcessMetricsChart");
    const resourceCanvas = $("#batchResourceChart");

    if (!rows.length) {
        if (ganttContainer) ganttContainer.innerHTML = "";
        return;
    }

    try {
        await loadChartJs();
    } catch (error) {
        showToast(error.message, "error");
        return;
    }

    if (renderVersion !== state.batchChartRenderVersion) return;

    const processIds = [...new Set(rows.flatMap(({ result }) =>
        (result.processes || []).map((process) =>
            firstDefined(process, ["pid", "process_id", "id"])
        ).filter((pid) => pid !== undefined)
    ))].sort((a, b) => Number(a) - Number(b));

    if (ganttContainer) {
        ganttContainer.innerHTML = rows.map(({ algorithm, result }, index) => {
            const processCount = new Set([
                ...(result.processes || []).map((process) =>
                    firstDefined(process, ["pid", "process_id", "id"])
                ),
                ...(result.events || []).map((event) => event.pid)
            ].filter((pid) => pid !== undefined)).size;
            return `
                <section class="batch-gantt-section">
                    <h4>${escapeHtml(algorithm)}</h4>
                    <div class="chart-canvas-wrap batch-gantt-canvas"
                        style="height:${Math.max(180, Math.min(780, processCount * 32))}px">
                        <canvas id="batchGanttChart${index}"></canvas>
                    </div>
                </section>
            `;
        }).join("");

        rows.forEach(({ algorithm, result }, index) => {
            const canvas = $(`#batchGanttChart${index}`);
            if (!canvas) return;

            const events = (result.events || []).filter((event) =>
                String(event.event_type || "").toUpperCase() === "CPU_EXECUTION" &&
                Number.isFinite(Number(event.start_time)) &&
                Number.isFinite(Number(event.end_time)) &&
                Number(event.end_time) > Number(event.start_time)
            );
            const intervals = events.map((event) => ({
                pid: event.pid,
                start: Number(event.start_time),
                end: Number(event.end_time)
            })).sort((a, b) => a.start - b.start);
            const processIds = [...new Set([
                ...(result.processes || []).map((process) =>
                    firstDefined(process, ["pid", "process_id", "id"])
                ),
                ...intervals.map(({ pid }) => pid)
            ].filter((pid) => pid !== undefined))].sort((a, b) =>
                Number(a) - Number(b)
            );

            if (!intervals.length) {
                canvas.closest(".batch-gantt-canvas")?.insertAdjacentHTML(
                    "afterend",
                    '<p class="chart-empty">No saved CPU execution intervals are available for this run.</p>'
                );
                canvas.closest(".batch-gantt-canvas")?.remove();
                return;
            }
            canvas.closest(".batch-gantt-canvas").style.height =
                `${Math.max(180, Math.min(780, processIds.length * 34 + 48))}px`;

            state.chartInstances[`batch-gantt-${index}`] = new Chart(canvas, {
                type: "bar",
                data: {
                    labels: processIds.map((pid) => `P${pid}`),
                    datasets: intervals.map(({ pid, start, end }, segmentIndex) => ({
                        label: `P${pid} execution ${segmentIndex + 1}`,
                        data: processIds.map((processId) =>
                            String(processId) === String(pid) ? [start, end] : null
                        ),
                        backgroundColor: processColor(pid),
                        borderWidth: 0,
                        borderRadius: 5,
                        barPercentage: 0.72,
                        categoryPercentage: 0.82,
                        grouped: false
                    }))
                },
                options: {
                    indexAxis: "y",
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        title: {
                            display: false,
                            text: algorithm
                        },
                        legend: { display: false }
                    },
                    scales: {
                        x: {
                            title: { display: true, text: "Simulation time" },
                            min: 0,
                            ticks: { precision: 0, maxTicksLimit: 12 },
                            grid: { color: "rgba(148, 163, 184, 0.13)" }
                        },
                        y: {
                            grid: { display: false },
                            ticks: { autoSkip: true, maxTicksLimit: 30 }
                        }
                    }
                }
            });
        });
    }

    if (metricsCanvas) {
        const metricFields = [
            ["Average waiting", "waiting", "#b8ddc5"],
            ["Average turnaround", "turnaround", "#f4c7a1"],
            ["Average response", "response", "#a9d8dc"]
        ];
        const metricRows = rows.map(({ result }) => {
            const metrics = result.metrics || {};
            const run = result.run || {};
            return {
                waiting: firstDefined(metrics, [
                    "average_waiting_time", "avg_waiting_time"
                ]) ?? firstDefined(run, ["average_waiting_time", "avg_waiting_time"]),
                turnaround: firstDefined(metrics, [
                    "average_turnaround_time", "avg_turnaround_time"
                ]) ?? firstDefined(run, [
                    "average_turnaround_time", "avg_turnaround_time"
                ]),
                response: firstDefined(metrics, [
                    "average_response_time", "avg_response_time"
                ]) ?? firstDefined(run, ["average_response_time", "avg_response_time"])
            };
        });

        state.chartInstances["batch-algorithm-metrics"] = new Chart(
            metricsCanvas,
            {
                type: "bar",
                data: {
                    labels: rows.map(({ algorithm }) => algorithm),
                    datasets: metricFields.map(([label, key, color]) => ({
                        label,
                        backgroundColor: color,
                        borderRadius: 5,
                        maxBarThickness: 26,
                        data: metricRows.map((row) =>
                            row[key] === undefined || row[key] === null
                                ? null
                                : Number(row[key])
                        )
                    }))
                },
                options: {
                    indexAxis: "y",
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: {
                            position: "bottom",
                            labels: { usePointStyle: true, padding: 18 }
                        }
                    },
                    scales: {
                        x: {
                            beginAtZero: true,
                            title: { display: true, text: "Average time units" },
                            grid: { color: "rgba(148, 163, 184, 0.13)" }
                        },
                        y: { grid: { display: false } }
                    }
                }
            }
        );
    }

    if (processCanvas && processIds.length) {
        const colors = [
            "#9ecae1", "#f2b8c6", "#b8ddc5",
            "#c5b8e3", "#f4c7a1", "#a9d8dc"
        ];
        state.chartInstances["batch-process-metrics"] = new Chart(
            processCanvas,
            {
                type: "line",
                data: {
                    labels: processIds.map((pid) => `P${pid}`),
                    datasets: rows.flatMap(({ algorithm, result }, index) => {
                        const datasets = ["waiting_time", "turnaround_time"].map(
                            (field) => ({
                                label: `${algorithm} ${field === "waiting_time" ? "waiting" : "turnaround"}`,
                                data: processIds.map((pid) => {
                                    const process = (result.processes || []).find(
                                        (item) => String(firstDefined(
                                            item,
                                            ["pid", "process_id", "id"]
                                        )) === String(pid)
                                    );
                                    const value = Number(process?.[field]);
                                    return Number.isFinite(value) ? value : null;
                                }),
                                borderColor: colors[index % colors.length],
                                backgroundColor: colors[index % colors.length],
                                borderDash: field === "waiting_time" ? [] : [5, 4],
                                borderWidth: 2.5,
                                pointRadius: 3,
                                pointHoverRadius: 5,
                                tension: 0.2
                            })
                        );
                        return datasets;
                    })
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    interaction: { mode: "index", intersect: false },
                    plugins: {
                        legend: {
                            position: "bottom",
                            labels: { usePointStyle: true, boxWidth: 8, padding: 12 }
                        }
                    },
                    scales: {
                        y: {
                            beginAtZero: true,
                            title: { display: true, text: "Time units" },
                            grid: { color: "rgba(148, 163, 184, 0.13)" }
                        },
                        x: { grid: { display: false } }
                    }
                }
            }
        );
    }

    if (resourceCanvas) {
        state.chartInstances["batch-resource"] = new Chart(resourceCanvas, {
            type: "bar",
            data: {
                labels: rows.map(({ algorithm }) => algorithm),
                datasets: [
                    {
                        label: "Completed",
                        data: rows.map(({ result }) => Number(firstDefined(
                            result.run || {},
                            ["completed_processes", "completed"]
                        )) || 0),
                        backgroundColor: "#b8ddc5",
                        borderRadius: 5
                    },
                    {
                        label: "Rejected",
                        data: rows.map(({ result }) => Number(firstDefined(
                            result.run || {},
                            ["rejected_processes", "rejected"]
                        )) || 0),
                        backgroundColor: "#f2b8c6",
                        borderRadius: 5
                    }
                ]
            },
            options: {
                indexAxis: "y",
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: "bottom",
                        labels: { usePointStyle: true, padding: 18 }
                    }
                },
                scales: {
                    x: {
                        beginAtZero: true,
                        title: { display: true, text: "Process count" },
                        ticks: { precision: 0 }
                    },
                    y: { grid: { display: false } }
                }
            }
        });
    }
}

async function renderCharts(runData) {
    const processes = runData.processes;

    try {
        await loadChartJs();
    } catch (error) {
        showToast(error.message, "error");
        return;
    }

    /* GANTT-STYLE EXECUTION TIMELINE */

    const executionEvents = (runData.events || []).filter((event) =>
        String(event.event_type || "").toUpperCase() === "CPU_EXECUTION" &&
        Number.isFinite(Number(event.start_time)) &&
        Number.isFinite(Number(event.end_time)) &&
        Number(event.end_time) > Number(event.start_time)
    );
    const timelineIntervals = executionEvents.map((event) => ({
        pid: event.pid,
        start: Number(event.start_time),
        end: Number(event.end_time)
    })).sort((a, b) => a.start - b.start);

    const ganttCanvas = $("#ganttChart");

    if (ganttCanvas && timelineIntervals.length) {
        destroyChart("gantt");

        const processIds = [...new Set([
            ...processes.map((process) =>
                firstDefined(process, ["pid", "process_id", "id"])
            ),
            ...timelineIntervals.map(({ pid }) => pid)
        ].filter((pid) => pid !== undefined))].sort((a, b) =>
            Number(a) - Number(b)
        );
        const starts = timelineIntervals.map(({ start }) => start);
        const ends = timelineIntervals.map(({ end }) => end);

        ganttCanvas.closest(".chart-canvas-wrap").style.height =
            `${Math.max(220, Math.min(780, processIds.length * 38 + 50))}px`;

        if (
            starts.every(Number.isFinite) &&
            ends.every(Number.isFinite)
        ) {
            state.chartInstances.gantt = new Chart(
                ganttCanvas,
                {
                    type: "bar",

                    data: {
                        labels: processIds.map((pid) => `P${pid}`),
                        datasets: timelineIntervals.map(({ pid, start, end }, index) => ({
                            label: `P${pid} execution ${index + 1}`,
                            data: processIds.map((processId) =>
                                String(processId) === String(pid) ? [start, end] : null
                            ),
                            borderWidth: 0,
                            borderRadius: 5,
                            backgroundColor: processColor(pid),
                            grouped: false
                        }))
                    },

                    options: {
                        indexAxis: "y",
                        responsive: true,
                        maintainAspectRatio: false,

                        plugins: {
                            legend: {
                                display: false
                            }
                        },

                        scales: {
                            x: {
                                beginAtZero: true,
                                title: {
                                    display: true,
                                    text: "Simulation time"
                                },
                                grid: { color: "rgba(148, 163, 184, 0.13)" }
                            },

                            y: {
                                grid: {
                                    display: false
                                },
                                ticks: { autoSkip: true, maxTicksLimit: 40 }
                            }
                        }
                    }
                }
            );

            showChartEmpty("ganttEmpty", false);
        } else {
            showChartEmpty("ganttEmpty", true);
        }
    } else {
        showChartEmpty("ganttEmpty", true);
    }

    /* WAITING AND TURNAROUND TIME */

    const processMetricData = getProcessMetricChartData(processes);
    const processCanvas = $("#processMetricsChart");

    if (processCanvas && processMetricData.datasets.length) {
        destroyChart("processMetrics");

        state.chartInstances.processMetrics = new Chart(
            processCanvas,
            {
                type: "bar",

                data: {
                    labels: [...processMetricData.labels],
                    datasets: processMetricData.datasets.map((dataset) => ({
                        ...dataset,
                        data: [...dataset.data],
                        borderRadius: 5,
                        maxBarThickness: 26
                    }))
                },

                options: {
                    responsive: true,
                    maintainAspectRatio: false,

                    plugins: {
                        legend: {
                            position: "bottom",
                            labels: { usePointStyle: true, padding: 14 }
                        }
                    },

                    scales: {
                        y: {
                            beginAtZero: true,

                            title: {
                                display: true,
                                text: "Time units"
                            },
                            grid: { color: "rgba(148, 163, 184, 0.13)" }
                        }
                    }
                }
            }
        );

        showChartEmpty("processMetricsEmpty", false);
    } else {
        showChartEmpty("processMetricsEmpty", true);
    }

    const timelineCanvas = $("#processTimelineChart");
    if (timelineCanvas && processMetricData.datasets.length) {
        destroyChart("processTimeline");
        state.chartInstances.processTimeline = new Chart(
            timelineCanvas,
            {
                type: "line",
                data: {
                    labels: [...processMetricData.labels],
                    datasets: processMetricData.datasets.map((dataset) => ({
                        ...dataset,
                        data: [...dataset.data],
                        borderWidth: 2,
                        pointHoverRadius: 6
                    }))
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    interaction: {
                        mode: "index",
                        intersect: false
                    },
                    elements: {
                        line: { tension: 0.25 },
                        point: { radius: 3 }
                    },
                    plugins: {
                        legend: {
                            position: "bottom",
                            labels: { usePointStyle: true, padding: 14 }
                        }
                    },
                    scales: {
                        y: {
                            beginAtZero: true,
                            title: { display: true, text: "Time units" },
                            grid: { color: "rgba(148, 163, 184, 0.13)" }
                        },
                        x: {
                            grid: { display: false }
                        }
                    }
                }
            }
        );
        showChartEmpty("processTimelineEmpty", false);
    } else {
        showChartEmpty("processTimelineEmpty", true);
    }

    /* COMPLETED VS REJECTED PROCESSES */

    const run = runData.run;

    const completed = firstDefined(
        run,
        ["completed_processes", "completed"]
    );

    const rejected = firstDefined(
        run,
        ["rejected_processes", "rejected"]
    );

    const resourceCanvas = $("#resourceChart");

    if (
        resourceCanvas &&
        completed !== undefined &&
        rejected !== undefined
    ) {
        destroyChart("resource");

        state.chartInstances.resource = new Chart(
            resourceCanvas,
            {
                type: "doughnut",

                data: {
                    labels: ["Completed", "Rejected"],

                    datasets: [{
                        data: [
                            Number(completed),
                            Number(rejected)
                        ],
                        backgroundColor: ["#9ecae1", "#f2b8c6"],
                        borderColor: "#fff",
                        borderWidth: 3,
                        hoverOffset: 8
                    }]
                },

                options: {
                    responsive: true,
                    maintainAspectRatio: false,

                    plugins: {
                        legend: {
                            position: "bottom",
                            labels: { usePointStyle: true, padding: 16 }
                        }
                    }
                }
            }
        );

        showChartEmpty("resourceEmpty", false);
    } else {
        showChartEmpty("resourceEmpty", true);
    }
}

function renderRun(runData) {
    state.latestRun = runData;

    const workspace = ensureAnalysisWorkspace();

    if (!workspace) return;
    resetBatchAnalysisViews();
    workspace.classList.remove("batch-mode");

    const run = runData.run;

    const runId = firstDefined(run, ["run_id", "id"]);

    const algorithm =
        firstDefined(run, ["display_algorithm", "algorithm", "scheduler"]) ||
        "Simulation";

    const status =
        firstDefined(run, ["run_status", "status"]) ||
        "Result received";

    $("#analysisRunTitle", workspace).textContent =
        `${algorithm} · Run ${runId ?? ""}`;

    $("#analysisRunMeta", workspace).textContent =
        `Status: ${status}`;

    $("#analysisMessage", workspace).textContent =
        "Results received from the backend. The metrics and charts below use the returned data.";

    $("#analysisMessage", workspace).classList.add("success");

    renderMetrics(runData);
    renderProcessTable(runData.processes);
    renderCharts(runData);

    $("#analysis").scrollIntoView({
        behavior: "smooth",
        block: "start"
    });
}

async function submitSimulation({
    algorithm,
    timeQuantum = null,
    machineConfig = null,
    render = true,
    workload = state.selectedWorkload,
    uploadedFile = state.uploadedFile
}) {
    if (!workload?.valid) {
        throw new Error("Select a valid workload before running a simulation.");
    }

    const formData = new FormData();
    formData.append("algorithm", algorithm.toLowerCase());

    if (uploadedFile) {
        formData.append("file", uploadedFile);
    } else if (workload.csv_text) {
        formData.append(
            "file",
            new Blob([workload.csv_text], { type: "text/csv" }),
            workload.name
        );
    } else {
        throw new Error("The selected workload has no CSV file content.");
    }

    if (timeQuantum !== null) {
        formData.append("time_quantum", String(timeQuantum));
    }
    if (algorithm === "HOST" && machineConfig) {
        formData.append("machine_profile", "custom");
        formData.append("total_memory_mb", String(machineConfig.total_memory_mb));
        formData.append("printers", String(machineConfig.printers));
        formData.append("scanners", String(machineConfig.scanners));
        formData.append("modems", String(machineConfig.modems));
        formData.append("cds", String(machineConfig.cds));
    }

    const execution = await requestJson(
        API_ENDPOINTS.runSimulation,
        {
            method: "POST",
            body: formData
        }
    );
    if (!execution.run_id) {
        throw new Error("The backend did not return a simulation run ID.");
    }

    const payload = await requestJson(
        API_ENDPOINTS.simulationDetails(execution.run_id)
    );

    const result = normaliseRun(payload);

    if (
        !result.run ||
        (!Array.isArray(result.processes) || !result.processes.length) &&
        !Object.keys(result.metrics || {}).length
    ) {
        throw new Error(
            `The backend returned no recognisable results for ${algorithm}.`
        );
    }

    result.run.display_algorithm = execution.display_algorithm ||
        (machineConfig ? "Custom Machine" : undefined);

    if (render) renderRun(result);

    return {
        algorithm: result.run.display_algorithm || algorithm,
        result
    };
}

async function runSimulation() {
    if (state.isRunning) return;

    if (!state.selectedWorkload?.valid || !state.selectedAlgorithms.length) {
        showToast("Select a valid workload and at least one algorithm first.", "error");
        return;
    }

    if (state.selectedTestbed === "custom" && !readCustomMachineConfig()) {
        showToast("Enter a valid custom machine memory and device capacity.", "error");
        return;
    }

    if (state.selectedAlgorithms.includes("RR") && state.timeQuantum === null) {
        showToast("Enter a positive integer time quantum for Round Robin.", "error");
        return;
    }

    if (state.selectedAlgorithms.length > 1) {
        await executeBatch(state.selectedAlgorithms);
        return;
    }

    const algorithm = state.selectedAlgorithms[0];
    const workload = state.selectedWorkload;
    const uploadedFile = state.uploadedFile;
    $("#batchComparison")?.remove();
    ensureAnalysisWorkspace()?.classList.remove("batch-mode");
    setSimulationLoading(true);

    try {
        state.batchResults = [];
        await submitSimulation({
            algorithm,
            timeQuantum: algorithm === "RR" ? state.timeQuantum : null,
            machineConfig: state.selectedTestbed === "custom"
                ? readCustomMachineConfig()
                : null,
            workload,
            uploadedFile
        });
        showToast(
            `${state.latestRun?.run?.display_algorithm || algorithm} simulation completed.`,
            "success"
        );
        await loadSimulationHistory();
    } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        const workspace = ensureAnalysisWorkspace();
        const analysisMessage = $("#analysisMessage", workspace);

        if (analysisMessage) {
            analysisMessage.textContent =
                `Simulation could not be completed: ${message}. Check that Flask and PostgreSQL are running.`;
            analysisMessage.classList.remove("success");
        }

        $("#analysis")?.scrollIntoView({
            behavior: "smooth",
            block: "start"
        });
        showToast(message, "error");
    } finally {
        setSimulationLoading(false);
    }
}

async function executeBatch(algorithms) {
    if (state.isRunning) return;

    if (!state.selectedWorkload?.valid) {
        showToast("Please select a valid workload first.", "error");
        return;
    }

    const selectedAlgorithms = [...new Set(algorithms)].map((algorithm) => ({
        algorithm,
        timeQuantum: algorithm === "RR" ? readTimeQuantum() : null
    }));

    if (
        !selectedAlgorithms.length ||
        selectedAlgorithms.some(({ algorithm }) =>
            !IMPLEMENTED_ALGORITHMS.includes(algorithm)
        )
    ) {
        showToast("Choose one or more implemented algorithms.", "error");
        return;
    }

    if (
        selectedAlgorithms.some(({ algorithm, timeQuantum }) =>
            algorithm === "RR" &&
            (!Number.isInteger(timeQuantum) || timeQuantum <= 0)
        )
    ) {
        showToast("Enter a positive integer time quantum for Round Robin.", "error");
        return;
    }

    const workload = state.selectedWorkload;
    const uploadedFile = state.uploadedFile;
    state.batchResults = [];
    $("#batchComparison")?.remove();
    setSimulationLoading(true);

    try {
        renderBatchComparison([], `Running 0 of ${selectedAlgorithms.length} simulations...`);

        for (const [index, item] of selectedAlgorithms.entries()) {
            const execution = await submitSimulation({
                ...item,
                render: false,
                workload,
                uploadedFile
            });

            state.batchResults.push(execution);
            renderBatchComparison(
                state.batchResults,
                `Completed ${index + 1} of ${selectedAlgorithms.length} simulations.`
            );
        }

        renderBatchComparison(
            state.batchResults,
            `Completed all ${selectedAlgorithms.length} selected simulations.`
        );
        showToast(
            `Completed ${state.batchResults.length} simulation(s).`,
            "success"
        );
        await loadSimulationHistory();
    } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        renderBatchComparison(
            state.batchResults,
            `Batch stopped after ${state.batchResults.length} of ${selectedAlgorithms.length} simulations: ${message}`
        );

        showToast(`Batch execution stopped: ${message}`, "error");
        if (state.batchResults.length) await loadSimulationHistory();
    } finally {
        setSimulationLoading(false);
    }
}

function renderBatchComparisonLegacy(results, status = "") {
    const workspace = ensureAnalysisWorkspace();
    if (!workspace) return;

    let container = $("#batchComparison", workspace);

    if (!container) {
        container = document.createElement("section");
        container.id = "batchComparison";
        container.className = "history-panel batch-comparison";
        container.innerHTML = `
            <div class="chart-panel-heading">
                <h3>Simulation Results</h3>
                <p>Each selected algorithm is shown separately, followed by a side-by-side comparison.</p>
            </div>
            <p class="batch-comparison-status" id="batchComparisonStatus"></p>
            <div id="algorithmRunDetails"></div>
            <section id="algorithmComparisonSection" hidden>
                <div class="chart-panel-heading">
                    <h3>Cross-algorithm comparison</h3>
                    <p>Compare aggregate metrics and each process across the selected algorithms.</p>
                </div>
                <div class="table-scroll" id="batchComparisonTable"></div>
                <div class="table-scroll" id="processAlgorithmComparison"></div>
            </section>
        `;
        workspace.appendChild(container);
    }

    const statusElement = $("#batchComparisonStatus", container);
    if (statusElement) statusElement.textContent = status;

    const rows = results.map(({ algorithm, result }) => {
        const metrics = result.metrics || {};
        const run = result.run || {};

        return {
            algorithm,
            result,
            waiting:
                firstDefined(metrics, ["average_waiting_time", "avg_waiting_time"]) ??
                firstDefined(run, ["average_waiting_time", "avg_waiting_time"]),
            turnaround:
                firstDefined(metrics, ["average_turnaround_time", "avg_turnaround_time"]) ??
                firstDefined(run, ["average_turnaround_time", "avg_turnaround_time"]),
            cpu:
                firstDefined(metrics, ["cpu_utilization"]) ??
                firstDefined(run, ["cpu_utilization"]),
            response:
                firstDefined(metrics, ["average_response_time", "avg_response_time"]) ??
                firstDefined(run, ["average_response_time", "avg_response_time"]),
            execution:
                firstDefined(metrics, ["total_execution_time"]) ??
                firstDefined(run, ["total_execution_time"]),
            completed: firstDefined(run, ["completed_processes", "completed"]),
            rejected: firstDefined(run, ["rejected_processes", "rejected"]),
            throughput:
                firstDefined(metrics, ["throughput"]) ??
                firstDefined(run, ["throughput"])
        };
    });
    workspace.classList.toggle("batch-mode", rows.length > 0);

    const details = $("#algorithmRunDetails", container);
    if (details) {
        details.innerHTML = rows.map((row, index) => {
            const { result } = row;
            const run = result.run || {};
            const processes = Array.isArray(result.processes) ? result.processes : [];
            const metricValues = [
                ["Run ID", firstDefined(run, ["run_id", "id"])],
                ["Status", firstDefined(run, ["status", "run_status"])],
                ["Completed", row.completed],
                ["Rejected", row.rejected],
                ["CPU utilisation", row.cpu, "%"],
                ["Average waiting time", row.waiting],
                ["Average turnaround time", row.turnaround],
                ["Average response time", row.response],
                ["Total execution time", row.execution],
                ["Throughput", row.throughput]
            ];
            const processColumns = [
                ["Process", ["pid", "process_id", "id"]],
                ["Arrival", ["arrival_time", "arrival"]],
                ["CPU burst", ["burst_time", "cpu_burst", "cpu"]],
                ["Completion", ["completion_time"]],
                ["Turnaround", ["turnaround_time"]],
                ["Waiting", ["waiting_time"]],
                ["Response", ["response_time"]],
                ["Final state", ["final_state", "status"]]
            ].filter(([, keys]) =>
                processes.some((process) => firstDefined(process, keys) !== undefined)
            );

            return `
                <article class="algorithm-run-detail">
                    <h4>${index + 1}. ${escapeHtml(row.algorithm)}</h4>
                    <div class="metrics-grid">
                        ${metricValues.map(([label, value, suffix = ""]) =>
                            metricCard(label, value, suffix)
                        ).join("")}
                    </div>
                    <div class="algorithm-run-charts">
                        <div>
                            <h5>Process performance</h5>
                            <div class="chart-canvas-wrap">
                                <canvas id="batchMetricsChart${index}"></canvas>
                            </div>
                        </div>
                        <div>
                            <h5>Process timing comparison</h5>
                            <div class="chart-canvas-wrap">
                                <canvas id="batchTimelineChart${index}"></canvas>
                            </div>
                        </div>
                        <div>
                            <h5>Completed and rejected processes</h5>
                            <div class="chart-canvas-wrap small-chart">
                                <canvas id="batchResourceChart${index}"></canvas>
                            </div>
                        </div>
                    </div>
                    <h5>Process-by-process results</h5>
                    ${processes.length
                        ? `<div class="table-scroll" id="processResultsBatch${index}">
                            <table class="data-table">
                                <thead><tr>${processColumns.map(([label]) =>
                                    `<th>${escapeHtml(label)}</th>`
                                ).join("")}</tr></thead>
                                <tbody>${processes.map((process) => `
                                    <tr>${processColumns.map(([, keys]) =>
                                        `<td>${escapeHtml(firstDefined(process, keys) ?? "—")}</td>`
                                    ).join("")}</tr>
                                `).join("")}</tbody>
                            </table>
                        </div>`
                        : `<p class="helper-text">No process results were returned for this run.</p>`
                    }
                </article>
            `;
        }).join("");
    }

    renderBatchCharts(rows);

    const comparison = $("#algorithmComparisonSection", container);
    const table = $("#batchComparisonTable", container);
    const processTable = $("#processAlgorithmComparison", container);
    const hasComparison = rows.length > 1;
    if (comparison) comparison.hidden = !hasComparison;

    if (table) {
        table.innerHTML = hasComparison
            ? `
            <table class="data-table">
                <thead>
                    <tr>
                        <th>Algorithm</th>
                        <th>Avg. Waiting</th>
                        <th>Avg. Turnaround</th>
                        <th>Avg. Response</th>
                        <th>Execution time</th>
                        <th>CPU Utilisation</th>
                        <th>Throughput</th>
                        <th>Completed</th>
                        <th>Rejected</th>
                    </tr>
                </thead>
                <tbody>
                    ${rows.map((row) => `
                        <tr>
                            <td>${escapeHtml(row.algorithm)}</td>
                            <td>${escapeHtml(row.waiting ?? "—")}</td>
                            <td>${escapeHtml(row.turnaround ?? "—")}</td>
                            <td>${escapeHtml(row.response ?? "—")}</td>
                            <td>${escapeHtml(row.execution ?? "—")}</td>
                            <td>${row.cpu !== undefined ? `${escapeHtml(row.cpu)}%` : "—"}</td>
                            <td>${escapeHtml(row.throughput ?? "—")}</td>
                            <td>${escapeHtml(row.completed ?? "—")}</td>
                            <td>${escapeHtml(row.rejected ?? "—")}</td>
                        </tr>
                    `).join("")}
                </tbody>
            </table>
        `
            : "";
    }

    if (processTable) {
        const processIds = [...new Set(rows.flatMap(({ result }) =>
            (result.processes || []).map((process) =>
                firstDefined(process, ["pid", "process_id", "id"])
            ).filter((pid) => pid !== undefined)
        ))].sort((a, b) => Number(a) - Number(b));
        const comparedMetrics = [
            ["Completion", "completion_time"],
            ["Turnaround", "turnaround_time"],
            ["Waiting", "waiting_time"],
            ["Response", "response_time"]
        ];

        processTable.innerHTML = hasComparison && processIds.length
            ? `
                <h4>Per-process comparison</h4>
                <table class="data-table">
                    <thead><tr>
                        <th>Process</th>
                        <th>Measure</th>
                        ${rows.map((row) => `<th>${escapeHtml(row.algorithm)}</th>`).join("")}
                    </tr></thead>
                    <tbody>${processIds.flatMap((pid) =>
                        comparedMetrics.map(([label, key], metricIndex) => `
                            <tr>
                                ${metricIndex === 0
                                    ? `<th rowspan="${comparedMetrics.length}">P${escapeHtml(pid)}</th>`
                                    : ""}
                                <th>${escapeHtml(label)}</th>
                                ${rows.map(({ result }) => {
                                    const process = (result.processes || []).find((item) =>
                                        String(firstDefined(item, ["pid", "process_id", "id"])) === String(pid)
                                    );
                                    return `<td>${escapeHtml(process?.[key] ?? "—")}</td>`;
                                }).join("")}
                            </tr>
                        `)
                    ).join("")}</tbody>
                </table>
            `
            : "";
    }

}

function renderBatchComparison(results, status = "") {
    const workspace = ensureAnalysisWorkspace();
    if (!workspace) return;

    ensureBatchAnalysisViews();
    workspace.classList.add("batch-mode");

    const rows = results.map(({ algorithm, result }) => {
        const metrics = result.metrics || {};
        const run = result.run || {};
        return {
            algorithm,
            result,
            runId: firstDefined(run, ["run_id", "id"]),
            waiting: firstDefined(metrics, [
                "average_waiting_time", "avg_waiting_time"
            ]) ?? firstDefined(run, [
                "average_waiting_time", "avg_waiting_time"
            ]),
            turnaround: firstDefined(metrics, [
                "average_turnaround_time", "avg_turnaround_time"
            ]) ?? firstDefined(run, [
                "average_turnaround_time", "avg_turnaround_time"
            ]),
            response: firstDefined(metrics, [
                "average_response_time", "avg_response_time"
            ]) ?? firstDefined(run, [
                "average_response_time", "avg_response_time"
            ]),
            execution: firstDefined(metrics, [
                "total_execution_time"
            ]) ?? firstDefined(run, ["total_execution_time"]),
            cpu: firstDefined(metrics, ["cpu_utilization"]) ??
                firstDefined(run, ["cpu_utilization"]),
            throughput: firstDefined(metrics, ["throughput"]) ??
                firstDefined(run, ["throughput"]),
            completed: firstDefined(run, [
                "completed_processes", "completed"
            ]),
            rejected: firstDefined(run, [
                "rejected_processes", "rejected"
            ])
        };
    });

    const title = $("#analysisRunTitle", workspace);
    const meta = $("#analysisRunMeta", workspace);
    const message = $("#analysisMessage", workspace);
    if (title) title.textContent = "Algorithm comparison";
    if (meta) {
        meta.textContent = rows.length
            ? rows.map((row) => `${row.algorithm} · Run ${row.runId ?? "—"}`).join(" | ")
            : "Preparing selected algorithm runs.";
    }
    if (message) {
        message.textContent = status;
        message.classList.toggle(
            "success",
            status.startsWith("Completed all ")
        );
    }

    const metricValue = (key) => {
        const candidates = rows.map((row) => ({
            algorithm: row.algorithm,
            value: Number(row[key])
        })).filter(({ value }) => Number.isFinite(value));
        if (!candidates.length) return undefined;
        const best = candidates.reduce((left, right) =>
            right.value < left.value ? right : left
        );
        return `${best.value.toFixed(2)} · ${best.algorithm}`;
    };

    $("#metricsGrid", workspace).innerHTML = [
        ...rows.map((row) =>
            metricCard(`${row.algorithm} · Run ID`, row.runId ?? "—")
        ),
        metricCard("Lowest average waiting time", metricValue("waiting")),
        metricCard("Lowest average turnaround time", metricValue("turnaround")),
        metricCard("Lowest average response time", metricValue("response"))
    ].join("");

    const descriptions = [
        ["executionTimelinePanel", "Execution timeline by algorithm", "Each scheduler's persisted CPU intervals, stacked in separate timelines."],
        ["performanceChartPanel", "Average metrics by algorithm", "Compare average waiting, turnaround, and response times."],
        ["processLineChartPanel", "Per-process comparison", "Compare waiting and turnaround time for each process across algorithms."],
        ["resourceChartPanel", "Completed and rejected processes", "Compare process outcomes for each selected algorithm."],
        ["processTablePanel", "Algorithm comparison tables", "Aggregate metrics, per-process comparisons, and each algorithm's complete results."]
    ];
    for (const [panelId, headingText, descriptionText] of descriptions) {
        const panel = $(`#${panelId}`);
        const heading = panel?.querySelector(".chart-panel-heading h3");
        const description = panel?.querySelector(".chart-panel-heading p");
        if (heading) heading.textContent = headingText;
        if (description) description.textContent = descriptionText;
    }

    const processIds = [...new Set(rows.flatMap(({ result }) =>
        (result.processes || []).map((process) =>
            firstDefined(process, ["pid", "process_id", "id"])
        ).filter((pid) => pid !== undefined)
    ))].sort((a, b) => Number(a) - Number(b));
    const aggregateTable = $("#batchAggregateComparison");
    if (aggregateTable) {
        aggregateTable.innerHTML = rows.length
            ? `
                <h4>Aggregate results by algorithm</h4>
                <table class="data-table">
                    <thead><tr>
                        <th>Algorithm</th><th>Run ID</th>
                        <th>Avg. waiting</th><th>Avg. turnaround</th>
                        <th>Avg. response</th><th>Execution time</th>
                        <th>CPU utilisation</th><th>Throughput</th>
                        <th>Completed</th><th>Rejected</th>
                    </tr></thead>
                    <tbody>${rows.map((row) => `
                        <tr>
                            <th>${escapeHtml(row.algorithm)}</th>
                            <td>${escapeHtml(row.runId ?? "—")}</td>
                            <td>${escapeHtml(row.waiting ?? "—")}</td>
                            <td>${escapeHtml(row.turnaround ?? "—")}</td>
                            <td>${escapeHtml(row.response ?? "—")}</td>
                            <td>${escapeHtml(row.execution ?? "—")}</td>
                            <td>${row.cpu === undefined ? "—" : `${escapeHtml(row.cpu)}%`}</td>
                            <td>${escapeHtml(row.throughput ?? "—")}</td>
                            <td>${escapeHtml(row.completed ?? "—")}</td>
                            <td>${escapeHtml(row.rejected ?? "—")}</td>
                        </tr>
                    `).join("")}</tbody>
                </table>
            `
            : `<p class="helper-text">${escapeHtml(status)}</p>`;
    }

    const processComparison = $("#processAlgorithmComparison");
    if (processComparison) {
        const metrics = [
            ["Completion", "completion_time"],
            ["Turnaround", "turnaround_time"],
            ["Waiting", "waiting_time"],
            ["Response", "response_time"]
        ];
        processComparison.innerHTML = rows.length > 1 && processIds.length
            ? `
                <h4>Cross-algorithm process comparison</h4>
                <table class="data-table">
                    <thead><tr>
                        <th>Process</th><th>Measure</th>
                        ${rows.map((row) =>
                            `<th>${escapeHtml(row.algorithm)}</th>`
                        ).join("")}
                    </tr></thead>
                    <tbody>${processIds.flatMap((pid) =>
                        metrics.map(([label, key], metricIndex) => `
                            <tr>
                                ${metricIndex === 0
                                    ? `<th rowspan="${metrics.length}">P${escapeHtml(pid)}</th>`
                                    : ""}
                                <th>${escapeHtml(label)}</th>
                                ${rows.map(({ result }) => {
                                    const process = (result.processes || []).find(
                                        (item) => String(firstDefined(
                                            item,
                                            ["pid", "process_id", "id"]
                                        )) === String(pid)
                                    );
                                    return `<td>${escapeHtml(process?.[key] ?? "—")}</td>`;
                                }).join("")}
                            </tr>
                        `)
                    ).join("")}</tbody>
                </table>
            `
            : "";
    }

    const processTables = $("#batchProcessDetailTables");
    if (processTables) {
        processTables.innerHTML = rows.map(({ algorithm, result }) => {
            const processes = result.processes || [];
            const columns = [
                ["Process", ["pid", "process_id", "id"]],
                ["Arrival", ["arrival_time", "arrival"]],
                ["CPU burst", ["burst_time", "cpu_burst", "cpu"]],
                ["Completion", ["completion_time"]],
                ["Turnaround", ["turnaround_time"]],
                ["Waiting", ["waiting_time"]],
                ["Response", ["response_time"]],
                ["Final state", ["final_state", "status"]]
            ].filter(([, keys]) =>
                processes.some((process) =>
                    firstDefined(process, keys) !== undefined
                )
            );
            return `
                <section class="batch-process-table">
                    <h4>${escapeHtml(algorithm)} · Run ${
                        escapeHtml(firstDefined(result.run || {}, ["run_id", "id"]) ?? "—")
                    }</h4>
                    ${processes.length
                        ? `<div class="table-scroll">
                            <table class="data-table">
                                <thead><tr>${columns.map(([label]) =>
                                    `<th>${escapeHtml(label)}</th>`
                                ).join("")}</tr></thead>
                                <tbody>${processes.map((process) => `
                                    <tr>${columns.map(([, keys]) =>
                                        `<td>${escapeHtml(firstDefined(process, keys) ?? "—")}</td>`
                                    ).join("")}</tr>
                                `).join("")}</tbody>
                            </table>
                        </div>`
                        : `<p class="helper-text">No process results were returned.</p>`
                    }
                </section>
            `;
        }).join("");
    }

    renderBatchCharts(rows);
}

/* ------------------------- SIMULATION HISTORY ------------------------- */

async function loadSimulationHistory() {
    const workspace = ensureAnalysisWorkspace();

    const container = $("#simulationHistoryTable", workspace);

    if (!container) return;

    try {
        const payload = await requestJson(
            API_ENDPOINTS.simulationHistory
        );

        const allRuns = Array.isArray(payload)
            ? payload
            : (
                payload.simulations ||
                payload.runs ||
                payload.data ||
                []
            );
        const runs = allRuns.slice(0, 10);

        state.history = runs;

        if (!runs.length) {
            container.innerHTML = `
                <p class="helper-text">
                    No saved simulation runs were returned.
                </p>
            `;
            return;
        }

        const columns = [
            ["Run ID", ["run_id", "id"]],
            ["Workload", ["workload_name", "source_filename"]],
            ["Algorithm", ["algorithm"]],
            ["Status", ["run_status", "status"]],
            ["Completed", ["completed_processes"]],
            ["Rejected", ["rejected_processes"]],
            ["CPU utilisation", ["cpu_utilization"]]
        ];

        const available = columns.filter(([, keys]) =>
            runs.some((run) =>
                firstDefined(run, keys) !== undefined
            )
        );

        container.innerHTML = `
            <table class="data-table">
                <thead>
                    <tr>
                        ${available.map(([label]) =>
                            `<th>${escapeHtml(label)}</th>`
                        ).join("")}
                    </tr>
                </thead>

                <tbody>
                    ${runs.map((run) => `
                        <tr>
                            ${available.map(([, keys]) =>
                                `<td>${escapeHtml(
                                    firstDefined(run, keys) ?? "—"
                                )}</td>`
                            ).join("")}
                        </tr>
                    `).join("")}
                </tbody>
            </table>
        `;

    } catch (error) {
        container.innerHTML = `
            <p class="helper-text">
                Simulation history is not connected yet:
                ${escapeHtml(error.message)}.
                Check API_ENDPOINTS.simulationHistory.
            </p>
        `;
    }
}

async function deleteSimulationHistory() {
    if (!window.confirm(
        "Delete all simulation runs and their saved results? Workload records will remain."
    )) {
        return;
    }

    const button = $("#deleteHistoryBtn");
    if (button) button.disabled = true;

    try {
        const result = await requestJson(
            API_ENDPOINTS.deleteSimulationHistory,
            { method: "DELETE" }
        );
        state.history = [];
        await loadSimulationHistory();
        showToast(
            `${result.deleted_runs ?? 0} simulation run(s) deleted.`,
            "success"
        );
    } catch (error) {
        showToast(
            error instanceof Error ? error.message : String(error),
            "error"
        );
    } finally {
        if (button) button.disabled = false;
    }
}

/* ------------------------- INITIALISATION ------------------------- */

function setupFileControls() {
    const upload = $("#csvUpload");

    if (upload) {
        upload.addEventListener("change", async (event) => {
            const file = event.target.files?.[0];

            if (!file) return;

            if (!file.name.toLowerCase().endsWith(".csv")) {
                showToast(
                    "Please choose a .csv file.",
                    "error"
                );

                upload.value = "";
                return;
            }

            try {
                await readLocalCsv(file);
            } catch (error) {
                showToast(
                    `Could not read CSV: ${error.message}`,
                    "error"
                );
            }
        });
    }

    const preview = $("#previewCsvBtn");

    if (preview) {
        preview.addEventListener("click", () => {
            if (state.selectedWorkload?.rows) {
                openCsvPreview(
                    state.selectedWorkload.rows,
                    state.selectedWorkload.name
                );
            }
        });
    }

    const select = $("#workloadSelect");

    if (select) {
        select.addEventListener("change", () => {
            const selected = state.availableWorkloads.find(
                (item, index) => {
                    const value = typeof item === "string"
                        ? item
                        : (
                            item.id ??
                            item.workload_id ??
                            item.path ??
                            item.filename ??
                            item.name ??
                            `Workload ${index + 1}`
                        );

                    return String(value) === select.value;
                }
            );

            if (!selected) {
                if (!state.uploadedFile) {
                    state.selectedWorkload = null;
                }

                renderSelectedFile();
                updateSelectionSummary();
                return;
            }

            const name = typeof selected === "string"
                ? selected
                : (
                    selected.filename ||
                    selected.name ||
                    selected.source_filename ||
                    "Saved workload"
                );

            const id = typeof selected === "string"
                ? selected
                : (
                    selected.id ??
                    selected.workload_id ??
                    selected.path ??
                    name
                );

            state.uploadedFile = null;

            state.selectedWorkload = {
                type: "saved",
                id,
                name,
                valid: true,
                csv_text: selected.csv_text,

                rows: selected.csv_text
                    ? parseCsv(selected.csv_text)
                    : null
            };

            renderSelectedFile();
            updateSelectionSummary();
        });
    }
}

function setupSelectionControls() {
    $$("[data-testbed]").forEach((button) => {
        button.addEventListener("click", () => {
            if (!state.isRunning) selectTestbed(button.dataset.testbed);
        });
    });

    $$("[data-algorithm]").forEach((button) => {
        button.addEventListener("click", () => {
            if (!state.isRunning) selectAlgorithm(button.dataset.algorithm);
        });
    });

    $("#rrTimeQuantum")?.addEventListener("input", () => {
        if (state.selectedAlgorithms.includes("RR")) syncTimeQuantum();
    });

    $$("#customMachineConfig input").forEach((input) => {
        input.addEventListener("input", updateSelectionSummary);
    });

    $("#runAllStandardBtn")?.addEventListener("click", () => {
        if (state.isRunning) return;

        state.selectedAlgorithms = [...STANDARD_ALGORITHMS];
        const quantumInput = $("#rrTimeQuantum");
        if (quantumInput) quantumInput.value = "1";
        state.timeQuantum = 1;
        updateAlgorithmSelection();
    });

    $("#continueToAnalysisBtn")?.addEventListener("click", runSimulation);
}

function setupPageNavigation() {
    $$('a[href^="#"]').forEach((link) => {
        link.addEventListener("click", () => {
            const target = $(link.getAttribute("href"));

            if (target) {
                target.setAttribute("tabindex", "-1");
            }
        });
    });

    const previewTargets = [
        "executionTimelinePanel",
        "performanceChartPanel",
        "resourceChartPanel",
        "processTablePanel"
    ];

    $$(".analysis-preview-card").forEach((card, index) => {
        const scrollToTarget = () => {
            const workspace = ensureAnalysisWorkspace();
            const targets = workspace?.classList.contains("batch-mode")
                ? [
                    "batchGanttCharts",
                    "batchAlgorithmMetricsChart",
                    "batchResourceChart",
                    state.batchResults.length > 1
                        ? "processAlgorithmComparison"
                        : "batchProcessTables"
                ]
                : previewTargets;
            const target = $(`#${targets[index]}`, workspace || document);
            target?.scrollIntoView({ behavior: "smooth", block: "start" });
        };

        card.setAttribute("role", "link");
        card.setAttribute("tabindex", "0");
        card.addEventListener("click", (event) => {
            event.preventDefault();
            scrollToTarget();
        });
        card.addEventListener("keydown", (event) => {
            if (event.key === "Enter" || event.key === " ") {
                event.preventDefault();
                scrollToTarget();
            }
        });
    });
}

/* ------------------------- RUNTIME STYLES ------------------------- */

function addRuntimeStyles() {
    if ($("#oscoreRuntimeStyles")) return;

    const style = document.createElement("style");

    style.id = "oscoreRuntimeStyles";

    style.textContent = `
        .oscore-toast {
            position: fixed;
            z-index: 9999;
            right: 22px;
            bottom: 22px;
            max-width: min(420px, calc(100vw - 44px));
            padding: 13px 17px;
            border-radius: 10px;
            background: #1e293b;
            color: #fff;
            box-shadow: 0 12px 35px rgba(15,23,42,.22);
            font-size: .9rem;
            opacity: 0;
            pointer-events: none;
            transform: translateY(10px);
            transition: opacity .18s ease, transform .18s ease;
        }

        .oscore-toast.visible {
            opacity: 1;
            transform: translateY(0);
        }

        .oscore-toast[data-type="error"] {
            background: #991b1b;
        }

        .oscore-toast[data-type="success"] {
            background: #065f46;
        }

        .info-dialog {
            width: min(760px, calc(100% - 30px));
        }

        .info-intro {
            color: #334155;
            font-size: 1rem;
            line-height: 1.8;
            margin-bottom: 22px;
        }

        .info-sections {
            display: grid;
            gap: 13px;
        }

        .info-section-item {
            padding: 16px 17px;
            border: 1px solid #e2e8f0;
            border-radius: 10px;
            background: #fbfcfe;
        }

        .info-section-item h3 {
            color: #1e293b;
            font-size: 1rem;
            margin-bottom: 5px;
        }

        .info-section-item p {
            color: #64748b;
            font-size: .9rem;
            line-height: 1.7;
        }

        .info-note {
            margin-top: 18px;
            padding: 14px 16px;
            border-left: 3px solid #2563eb;
            border-radius: 0 8px 8px 0;
            background: #eff6ff;
            color: #334155;
            font-size: .85rem;
            line-height: 1.7;
        }

        .csv-validation {
            display: flex;
            flex-direction: column;
            gap: 3px;
            padding: 12px 14px;
            border-radius: 9px;
            margin-bottom: 13px;
            font-size: .86rem;
        }

        .csv-validation.valid {
            background: #ecfdf5;
            color: #065f46;
        }

        .csv-validation.invalid {
            background: #fef2f2;
            color: #991b1b;
        }

        .csv-filename {
            color: #64748b;
            font-size: .82rem;
            margin-bottom: 12px;
        }

        .selected-file {
            justify-content: space-between;
        }

        .file-name {
            overflow-wrap: anywhere;
        }

        .file-status {
            color: #0f766e;
            font-size: .75rem;
            font-weight: 800;
            white-space: nowrap;
        }

        .analysis-workspace {
            margin-top: 35px;
        }

        .batch-view {
            display: none;
        }

        .analysis-workspace.batch-mode .batch-view {
            display: block;
        }

        .analysis-workspace.batch-mode .chart-panel > .chart-canvas-wrap:not(.batch-view),
        .analysis-workspace.batch-mode .chart-panel > .chart-empty,
        .analysis-workspace.batch-mode #processResultsTable {
            display: none;
        }

        .analysis-workspace.batch-mode .batch-gantt-canvas {
            display: block;
            min-width: 0;
        }

        .analysis-workspace.batch-mode #batchGanttCharts.chart-canvas-wrap {
            height: auto;
            min-height: 0;
            overflow: visible;
        }

        .analysis-workspace.batch-mode .batch-gantt-section .chart-empty {
            margin: 8px 0 16px;
        }

        .analysis-workspace.batch-mode .batch-gantt-section + .batch-gantt-section {
            margin-top: 20px;
        }

        .analysis-workspace.batch-mode .batch-gantt-section h4,
        .analysis-workspace.batch-mode .batch-process-table h4,
        .analysis-workspace.batch-mode #batchAggregateComparison h4,
        .analysis-workspace.batch-mode #processAlgorithmComparison h4 {
            margin: 18px 0 10px;
            color: #334155;
        }

        .analysis-workspace.batch-mode #metricsGrid {
            grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
        }

        .analysis-workspace.batch-mode #batchProcessTables {
            margin-top: 22px;
        }

        .analysis-workspace.batch-mode .batch-process-table + .batch-process-table {
            margin-top: 22px;
        }

        .history-heading-row {
            display: flex;
            justify-content: space-between;
            align-items: flex-start;
            gap: 16px;
        }

        .analysis-preview-card {
            cursor: pointer;
        }

        .analysis-preview-card:focus-visible {
            outline: 3px solid #2563eb;
            outline-offset: 3px;
        }

        .chart-panel[id] {
            scroll-margin-top: 24px;
        }

        .analysis-toolbar {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 20px;
            margin-bottom: 20px;
        }

        .analysis-toolbar h3 {
            margin-top: 5px;
            font-size: 1.55rem;
            color: #1e293b;
        }

        .analysis-toolbar .outline-button {
            flex: 0 0 auto;
        }

        .analysis-message {
            padding: 15px 17px;
            border: 1px solid #cbd5e1;
            border-radius: 10px;
            background: #f8fafc;
            color: #475569;
            font-size: .88rem;
            line-height: 1.6;
            margin-bottom: 20px;
        }

        .analysis-message.success {
            border-color: #a7f3d0;
            background: #ecfdf5;
            color: #065f46;
        }

        .metrics-grid {
            display: grid;
            grid-template-columns: repeat(4, minmax(0, 1fr));
            gap: 13px;
            margin-bottom: 20px;
        }

        .metric-card {
            display: flex;
            flex-direction: column;
            gap: 8px;
            min-height: 100px;
            padding: 17px;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
            background: #fff;
        }

        .metric-card span {
            color: #64748b;
            font-size: .76rem;
            font-weight: 700;
        }

        .metric-card strong {
            color: #1e293b;
            font-size: 1.45rem;
            line-height: 1.2;
            overflow-wrap: anywhere;
        }

        .charts-grid {
            display: grid;
            grid-template-columns: minmax(0, 1fr);
            gap: 17px;
        }

        .chart-panel,
        .history-panel {
            min-width: 0;
            padding: 21px;
            border: 1px solid #e2e8f0;
            border-radius: 14px;
            background: #fff;
            box-shadow: 0 8px 25px rgba(15,23,42,.035);
        }

        .chart-panel-heading {
            margin-bottom: 16px;
        }

        .chart-panel-heading h3 {
            color: #1e293b;
            font-size: 1.05rem;
        }

        .chart-panel-heading p {
            color: #64748b;
            font-size: .8rem;
            margin-top: 4px;
        }

        .chart-canvas-wrap {
            position: relative;
            height: 270px;
        }

        .chart-canvas-wrap.small-chart {
            height: 220px;
        }

        .algorithm-run-detail {
            margin-top: 22px;
            padding: 22px;
            border: 1px solid #e2e8f0;
            border-radius: 14px;
            background: #fff;
        }

        .algorithm-run-detail > h4 {
            margin: 0 0 18px;
            color: #1e293b;
            font-size: 1.2rem;
        }

        .algorithm-run-detail > h5,
        .algorithm-run-charts h5,
        #processAlgorithmComparison h4 {
            margin: 18px 0 10px;
            color: #334155;
            font-size: .95rem;
        }

        .algorithm-run-detail .metrics-grid {
            grid-template-columns: repeat(5, minmax(0, 1fr));
        }

        .algorithm-run-charts {
            display: grid;
            grid-template-columns: minmax(0, 1fr);
            gap: 14px;
        }

        .chart-empty {
            color: #94a3b8;
            font-size: .82rem;
            margin-top: 8px;
        }

        .chart-empty[hidden] {
            display: none;
        }

        .table-scroll {
            width: 100%;
            overflow-x: auto;
        }

        .data-table {
            width: 100%;
            border-collapse: collapse;
            font-size: .82rem;
            white-space: nowrap;
        }

        .data-table th,
        .data-table td {
            padding: 11px 12px;
            border-bottom: 1px solid #e2e8f0;
            text-align: left;
        }

        .data-table th {
            background: #f1f5f9;
            color: #334155;
            font-weight: 800;
        }

        .data-table td {
            color: #475569;
        }

        .history-panel {
            margin-top: 18px;
        }

        @media (max-width: 760px) {
            .metrics-grid {
                grid-template-columns: repeat(2, minmax(0, 1fr));
            }

            .charts-grid {
                grid-template-columns: 1fr;
            }

            .algorithm-run-detail .metrics-grid {
                grid-template-columns: repeat(2, minmax(0, 1fr));
            }

            .analysis-toolbar {
                align-items: flex-start;
                flex-direction: column;
            }

            .history-heading-row {
                align-items: stretch;
                flex-direction: column;
            }

            .analysis-toolbar .outline-button {
                width: 100%;
            }
        }

        @media (max-width: 420px) {
            .metrics-grid {
                grid-template-columns: 1fr;
            }

            .algorithm-run-detail .metrics-grid {
                grid-template-columns: 1fr;
            }
        }
    `;

    document.head.appendChild(style);
}

/* ------------------------- START APPLICATION ------------------------- */

document.addEventListener("DOMContentLoaded", () => {
    addRuntimeStyles();
    const quantumInput = $("#rrTimeQuantum");
    if (quantumInput) quantumInput.value = "1";

    setupExplorePopups();
    setupFileControls();
    setupSelectionControls();
    updateAlgorithmSelection();
    setupPageNavigation();

    ensureAnalysisWorkspace();

    loadAvailableWorkloads();
    loadSimulationHistory();

    updateSelectionSummary();
});