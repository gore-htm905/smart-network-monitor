const API_BASE = 'http://localhost:8085/api';

async function fetchJson(url, options = {}) {
  try {
    const res = await fetch(url, options);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    return await res.json();
  } catch (err) {
    console.error('API Error:', err);
    return null;
  }
}

// ========== SIMULATION ==========
export const startSimulation = () => fetchJson(`${API_BASE}/simulation/start`, { method: 'POST' });
export const pauseSimulation = () => fetchJson(`${API_BASE}/simulation/pause`, { method: 'POST' });
export const resumeSimulation = () => fetchJson(`${API_BASE}/simulation/resume`, { method: 'POST' });
export const resetSimulation = () => fetchJson(`${API_BASE}/simulation/reset`, { method: 'POST' });
export const setSimulationSpeed = (speed) => fetchJson(`${API_BASE}/simulation/speed`, {
  method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ speed })
});

// ========== NETWORK CONFIGURATION ==========
export const generateNetwork = (config) => fetchJson(`${API_BASE}/network/generate`, {
  method: 'POST', headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify(config)
});
export const getNetworkConfig = () => fetchJson(`${API_BASE}/network/config`);

// ========== PACKETS ==========
export const generatePacket = (source, destination, priority, size) => fetchJson(`${API_BASE}/packets/generate`, {
  method: 'POST', headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ source, destination, priority, size })
});
export const autoGeneratePacket = () => fetchJson(`${API_BASE}/packets/auto-generate`, { method: 'POST' });
export const bulkGenerate = (count) => fetchJson(`${API_BASE}/packets/bulk-generate`, {
  method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ count })
});
export const getAllPackets = async () => {
  const data = await fetchJson(`${API_BASE}/packets/all`);
  return data?.packets || [];
};

// ========== NETWORK ==========
export const getNetworkState = () => fetchJson(`${API_BASE}/network/state`);
export const getTopology = () => fetchJson(`${API_BASE}/network/topology`);

// ========== QUEUES ==========
export const getQueues = () => fetchJson(`${API_BASE}/queues`);

// ========== HISTORY ==========
// Backend returns { count, packets: [...] } — unwrap to the array components expect.
export const getHistory = async () => {
  const data = await fetchJson(`${API_BASE}/history`);
  return data?.packets || [];
};

// ========== SCHEDULER ==========
// Backend returns { count, nodes: [...] } — unwrap to the array components expect.
export const getScheduler = async () => {
  const data = await fetchJson(`${API_BASE}/scheduler`);
  return data?.nodes || [];
};

// ========== ROUTES ==========
export const getRoutes = () => fetchJson(`${API_BASE}/routes`);

// ========== SEARCH ==========
export const searchPacket = (id) => fetchJson(`${API_BASE}/packets/search?id=${encodeURIComponent(id)}`);

// ========== SORT ==========
// Backend returns { count, packets: [...] } — unwrap to the array components expect.
export const sortPackets = async (by, order) => {
  const data = await fetchJson(`${API_BASE}/packets/sort?by=${by}&order=${order}`);
  return data?.packets || [];
};

// ========== FILTER ==========
// Backend returns { count, packets: [...] } — unwrap to the array components expect.
export const filterPackets = async (q) => {
  const data = await fetchJson(`${API_BASE}/packets/filter?q=${encodeURIComponent(q)}`);
  return data?.packets || [];
};

// ========== STATISTICS ==========
export const getStatistics = () => fetchJson(`${API_BASE}/statistics`);

// ========== ALERTS ==========
// Backend returns { count, alerts: [...] } — unwrap to the array components expect.
export const getAlerts = async () => {
  const data = await fetchJson(`${API_BASE}/alerts`);
  return data?.alerts || [];
};

// ========== NODE/LINK TOGGLE (via failure event endpoint) ==========
export const toggleNode = (nodeId, active) => fetchJson(`${API_BASE}/network/failure`, {
  method: 'POST', headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ type: 'node', nodeId, active })
});
export const toggleLink = (source, target, active) => fetchJson(`${API_BASE}/network/failure`, {
  method: 'POST', headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ type: 'link', source, target, active })
});

// ========== DATA STRUCTURES ==========
export const getDataStructures = () => fetchJson(`${API_BASE}/datastructures`);