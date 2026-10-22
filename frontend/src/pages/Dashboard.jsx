import React, { useState, useEffect, useCallback, useMemo } from 'react';
import NetworkTopology from '../components/NetworkTopology';
import QueueVisualization from '../components/QueueVisualization';
import PacketHistory from '../components/PacketHistory';
import DataStructures from '../components/DataStructures';
import Alerts from '../components/Alerts';
import PacketDetails from '../components/PacketDetails';
import * as api from '../services/api';
import './Dashboard.css';

const SIDEBAR_ITEMS = [
  { key: 'dashboard', label: 'Dashboard', icon: '📊' },
  { key: 'topology', label: 'Network Topology', icon: '🌐' },
  { key: 'packets', label: 'Packet Management', icon: '📦' },
  { key: 'queues', label: 'Queues', icon: '📋' },
  { key: 'datastructures', label: 'Data Structures', icon: '🔗' },
  { key: 'history', label: 'Packet History', icon: '📜' },
  { key: 'search', label: 'Search & Sort', icon: '🔍' },
  { key: 'stats', label: 'Statistics', icon: '📈' },
  { key: 'alerts', label: 'Alerts', icon: '🔔' },
];

const PRESETS = [
  { label: '8 Endpoints + 3 Routers (10 pkts)', endpoints: 8, routers: 3, packets: 10, queueCap: 20 },
  { label: '10 Endpoints + 4 Routers (50 pkts)', endpoints: 10, routers: 4, packets: 50, queueCap: 20 },
  { label: '5 Endpoints + 2 Routers (20 pkts)', endpoints: 5, routers: 2, packets: 20, queueCap: 15 },
  { label: '20 Endpoints + 6 Routers (50 pkts)', endpoints: 20, routers: 6, packets: 50, queueCap: 25 },
];

const Dashboard = () => {
  const [activeTab, setActiveTab] = useState('dashboard');
  const [networkState, setNetworkState] = useState(null);
  const [topology, setTopology] = useState(null);
  const [allPackets, setAllPackets] = useState([]);
  const [queues, setQueues] = useState(null);
  const [history, setHistory] = useState([]);
  const [alerts, setAlerts] = useState([]);
  const [dsState, setDsState] = useState(null);
  const [selectedPacket, setSelectedPacket] = useState(null);
  const [isRunning, setIsRunning] = useState(false);

  // Network Configuration Form State
  const [configEndpoints, setConfigEndpoints] = useState(10);
  const [configRouters, setConfigRouters] = useState(4);
  const [configPackets, setConfigPackets] = useState(50);
  const [configQueueCap, setConfigQueueCap] = useState(20);
  const [configAlgorithm, setConfigAlgorithm] = useState('Dijkstra');
  const [isGeneratingNetwork, setIsGeneratingNetwork] = useState(false);

  // Single Packet Generation Form State
  const [genSrc, setGenSrc] = useState('N1');
  const [genDest, setGenDest] = useState('N2');
  const [genPriority, setGenPriority] = useState(2);
  const [genSize, setGenSize] = useState(512);

  // Available endpoints from topology
  const endpointNodes = useMemo(() => {
    if (!topology?.nodes) return [];
    return topology.nodes.filter(n => !n.isRouter && n.type !== 'Router');
  }, [topology]);

  // Keep genSrc & genDest aligned with available endpoints
  useEffect(() => {
    if (endpointNodes.length >= 2) {
      if (!endpointNodes.some(n => n.id === genSrc)) {
        setGenSrc(endpointNodes[0].id);
      }
      if (!endpointNodes.some(n => n.id === genDest)) {
        setGenDest(endpointNodes[endpointNodes.length - 1].id);
      }
    }
  }, [endpointNodes, genSrc, genDest]);

  const refreshAll = useCallback(async () => {
    try {
      const [ns, topo, pkts, q, h, al, ds] = await Promise.all([
        api.getNetworkState(),
        api.getTopology(),
        api.getAllPackets(),
        api.getQueues(),
        api.getHistory(),
        api.getAlerts(),
        api.getDataStructures(),
      ]);
      if (ns) { setNetworkState(ns); setIsRunning(ns.isRunning); }
      if (topo) setTopology(topo);
      if (pkts) setAllPackets(pkts);
      if (q) setQueues(q);
      if (pkts && pkts.length > 0) {
        setHistory(pkts);
      } else if (h) {
        setHistory(h);
      }
      if (al) setAlerts(al);
      if (ds) setDsState(ds);
    } catch (err) {
      console.error('Refresh error:', err);
    }
  }, []);

  // Poll faster when simulation is running for fluid packet visualization
  useEffect(() => {
    refreshAll();
    const intervalTime = isRunning ? 400 : 1500;
    const interval = setInterval(refreshAll, intervalTime);
    return () => clearInterval(interval);
  }, [refreshAll, isRunning]);

  const simState = networkState?.simulationState || (isRunning ? 'RUNNING' : (networkState?.isPaused ? 'PAUSED' : 'STOPPED'));

  // Simulation Controls
  const handleStart = async () => {
    if (simState === 'PAUSED') {
      await api.resumeSimulation();
    } else {
      await api.startSimulation();
    }
    await refreshAll();
  };

  const handlePause = async () => {
    if (simState === 'RUNNING') {
      await api.pauseSimulation();
      await refreshAll();
    }
  };

  const handleReset = async () => {
    await api.resetSimulation();
    await refreshAll();
  };

  // Network Generation
  const handleGenerateNetwork = async (customParams = null) => {
    setIsGeneratingNetwork(true);
    const params = customParams || {
      endpoints: Number(configEndpoints),
      routers: Number(configRouters),
      packets: Number(configPackets),
      queueCapacity: Number(configQueueCap),
      algorithm: configAlgorithm
    };
    try {
      await api.generateNetwork(params);
      await refreshAll();
    } finally {
      setIsGeneratingNetwork(false);
    }
  };

  const applyPreset = (preset) => {
    setConfigEndpoints(preset.endpoints);
    setConfigRouters(preset.routers);
    setConfigPackets(preset.packets);
    setConfigQueueCap(preset.queueCap);
    handleGenerateNetwork({
      endpoints: preset.endpoints,
      routers: preset.routers,
      packets: preset.packets,
      queueCapacity: preset.queueCap,
      algorithm: configAlgorithm
    });
  };

  // Packet Generation
  const handleGenerateSinglePacket = async () => {
    await api.generatePacket(genSrc, genDest, genPriority, genSize);
    refreshAll();
  };

  const handleBulkGeneratePackets = async () => {
    const count = Number(configPackets) > 0 ? Number(configPackets) : 10;
    await api.bulkGenerate(count);
    refreshAll();
  };

  const handleSearch = async (id) => {
    const result = await api.searchPacket(id);
    if (result && !result.error) setSelectedPacket(result);
  };

  const handleSort = async (by, order) => {
    const result = await api.sortPackets(by, order);
    if (result) setHistory(result);
  };

  const handleToggleNode = async (nodeId, currentlyInactive) => {
    await api.toggleNode(nodeId, currentlyInactive);
    refreshAll();
  };

  const ns = networkState || {};
  const activeEndpointsCount = endpointNodes.filter(n => n.status !== 'Inactive').length;
  const routerNodes = topology?.nodes?.filter(n => n.type === 'Router' || n.isRouter) || [];
  const activeRoutersCount = routerNodes.filter(n => n.status !== 'Inactive').length;
  const failedRoutersCount = routerNodes.filter(n => n.status === 'Inactive').length;
  const inTransitCount = allPackets.filter(p => p.status === 'In Transit' || p.status === 'Rerouted').length;

  return (
    <div className="dashboard-container">
      {/* LEFT SIDEBAR */}
      <aside className="sidebar">
        <div className="sidebar-brand">
          <div className="brand-icon">📡</div>
          <h2>SMART<br/>MONITOR</h2>
        </div>
        <nav className="sidebar-nav">
          {SIDEBAR_ITEMS.map(item => (
            <div
              key={item.key}
              className={`sidebar-item ${activeTab === item.key ? 'active' : ''}`}
              onClick={() => setActiveTab(item.key)}
            >
              <span className="sidebar-icon">{item.icon}</span>
              <span>{item.label}</span>
            </div>
          ))}
        </nav>
      </aside>

      {/* MAIN CONTENT */}
      <main className="main-content">
        {/* HEADER */}
        <header className="header">
          <div className="header-left">
            <h1>SMART COMMUNICATION NETWORK MONITORING SYSTEM</h1>
          </div>
          <div className="header-right">
            <div className="sim-controls">
              {simState === 'PAUSED' ? (
                <button className="btn btn-start" onClick={handleStart} title="Resume simulation">
                  ▶ RESUME
                </button>
              ) : (
                <button
                  className="btn btn-start"
                  onClick={handleStart}
                  disabled={simState === 'RUNNING'}
                  style={simState === 'RUNNING' ? { opacity: 0.5, cursor: 'not-allowed' } : {}}
                  title="Start real-time simulation"
                >
                  ▶ START
                </button>
              )}
              <button
                className="btn btn-pause"
                onClick={handlePause}
                disabled={simState !== 'RUNNING'}
                style={simState !== 'RUNNING' ? { opacity: 0.5, cursor: 'not-allowed' } : {}}
                title="Pause simulation"
              >
                ⏸ {simState === 'PAUSED' ? 'PAUSED' : 'PAUSE'}
              </button>
              <button className="btn btn-reset" onClick={handleReset} title="Reset simulation to initial state">
                ↻ RESET
              </button>
              <button className="btn btn-generate" onClick={handleBulkGeneratePackets} title="Generate bulk packets">
                📦 GENERATE PACKETS
              </button>
            </div>
            <div className={`status-badge ${simState === 'RUNNING' ? 'online' : simState === 'PAUSED' ? 'warning' : 'offline'}`}>
              <div className="status-dot"></div>
              <span>
                {simState === 'RUNNING' ? 'SIMULATION RUNNING' : simState === 'PAUSED' ? 'SIMULATION PAUSED' : 'SIMULATION STOPPED'}
              </span>
            </div>
          </div>
        </header>

        {/* CONTENT AREA */}
        <div className="content-area">

          {/* ==================== 1. NETWORK CONFIGURATION PANEL ==================== */}
          <section className="network-config-panel">
            <div className="config-header">
              <div className="config-title">
                <span>⚙️ Network Configuration</span>
              </div>
              <div className="presets-container">
                <span className="preset-label">Presets:</span>
                {PRESETS.map((p, idx) => (
                  <button
                    key={idx}
                    className="preset-btn"
                    onClick={() => applyPreset(p)}
                    title={`Generate ${p.endpoints} endpoints + ${p.routers} routers with ${p.packets} packets`}
                  >
                    {p.endpoints}E + {p.routers}R
                  </button>
                ))}
              </div>
            </div>

            <div className="config-form-grid">
              <div className="config-input-group">
                <label>Endpoints</label>
                <input
                  type="number"
                  min="2"
                  max="32"
                  value={configEndpoints}
                  onChange={e => setConfigEndpoints(Number(e.target.value))}
                />
              </div>

              <div className="config-input-group">
                <label>Routers</label>
                <input
                  type="number"
                  min="2"
                  max="16"
                  value={configRouters}
                  onChange={e => setConfigRouters(Number(e.target.value))}
                />
              </div>

              <div className="config-input-group">
                <label>Packets</label>
                <input
                  type="number"
                  min="0"
                  max="150"
                  value={configPackets}
                  onChange={e => setConfigPackets(Number(e.target.value))}
                />
              </div>

              <div className="config-input-group">
                <label>Queue Capacity</label>
                <input
                  type="number"
                  min="5"
                  max="100"
                  value={configQueueCap}
                  onChange={e => setConfigQueueCap(Number(e.target.value))}
                />
              </div>

              <div className="config-input-group">
                <label>Routing Algorithm</label>
                <select
                  value={configAlgorithm}
                  onChange={e => setConfigAlgorithm(e.target.value)}
                >
                  <option value="Dijkstra">Dijkstra</option>
                </select>
              </div>

              <button
                className="btn-generate-network"
                onClick={() => handleGenerateNetwork()}
                disabled={isGeneratingNetwork}
              >
                {isGeneratingNetwork ? 'GENERATING...' : '⚡ GENERATE NETWORK'}
              </button>
            </div>
          </section>

          {/* ==================== DASHBOARD VIEW ==================== */}
          {activeTab === 'dashboard' && (
            <>
              {/* COMPREHENSIVE KPI ROW */}
              <div className="kpi-row-wrap">
                <div className="kpi-mini">
                  <span className="panel-title">Total Packets</span>
                  <div className="kpi-value cyan">{ns.totalPackets || allPackets.length || 0}</div>
                </div>
                <div className="kpi-mini">
                  <span className="panel-title">Delivered</span>
                  <div className="kpi-value success">{ns.delivered || 0}</div>
                </div>
                <div className="kpi-mini">
                  <span className="panel-title">Dropped</span>
                  <div className="kpi-value danger">{ns.dropped || 0}</div>
                </div>
                <div className="kpi-mini">
                  <span className="panel-title">In Transit</span>
                  <div className="kpi-value warning">{inTransitCount}</div>
                </div>
                <div className="kpi-mini">
                  <span className="panel-title">Pending</span>
                  <div className="kpi-value warning">{ns.pending || 0}</div>
                </div>
                <div className="kpi-mini">
                  <span className="panel-title">Avg Delay</span>
                  <div className="kpi-value cyan">{ns.averageDelay || 0}ms</div>
                </div>
                <div className="kpi-mini">
                  <span className="panel-title">Utilization</span>
                  <div className="kpi-value warning">{Math.round(ns.networkUtilization || 0)}%</div>
                </div>
                <div className="kpi-mini">
                  <span className="panel-title">Active Endpoints</span>
                  <div className="kpi-value success">{activeEndpointsCount}</div>
                </div>
                <div className="kpi-mini">
                  <span className="panel-title">Active Routers</span>
                  <div className="kpi-value cyan">{activeRoutersCount}</div>
                </div>
                <div className="kpi-mini">
                  <span className="panel-title">Failed Routers</span>
                  <div className="kpi-value danger">{failedRoutersCount}</div>
                </div>
              </div>

              {/* MAIN GRID: Topology + Queues */}
              <div className="dashboard-main-grid">
                <div className="panel topology-panel">
                  <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '10px' }}>
                    <h3 className="panel-title" style={{ margin: 0 }}>Dynamic Network Topology</h3>
                    <span style={{ fontSize: '0.75rem', color: '#94a3b8' }}>
                      Click any node to toggle offline/online
                    </span>
                  </div>
                  <NetworkTopology
                    topology={topology}
                    packets={allPackets}
                    selectedPacket={selectedPacket}
                    onSelectPacket={setSelectedPacket}
                    onToggleNode={handleToggleNode}
                    isRunning={simState === 'RUNNING'}
                  />
                </div>
                <div className="panel queues-panel">
                  <h3 className="panel-title">Router & Priority Queues</h3>
                  <QueueVisualization queues={queues} dsState={dsState} />
                </div>
              </div>

              {/* BOTTOM GRID: History + Alerts */}
              <div className="dashboard-bottom-grid">
                <div className="panel history-panel-short">
                  <PacketHistory
                    history={history}
                    onSearch={handleSearch}
                    onSort={handleSort}
                    onSelectPacket={setSelectedPacket}
                  />
                </div>
                <div className="panel alerts-panel-short">
                  <Alerts alerts={alerts} />
                </div>
              </div>
            </>
          )}

          {/* ==================== TOPOLOGY VIEW ==================== */}
          {activeTab === 'topology' && (
            <div className="panel full-panel">
              <h3 className="panel-title">Network Topology Graph</h3>
              <NetworkTopology
                topology={topology}
                packets={allPackets}
                selectedPacket={selectedPacket}
                onSelectPacket={setSelectedPacket}
                onToggleNode={handleToggleNode}
                isRunning={simState === 'RUNNING'}
              />
            </div>
          )}

          {/* ==================== PACKET MANAGEMENT VIEW ==================== */}
          {activeTab === 'packets' && (
            <div className="panel full-panel">
              <h3 className="panel-title">Packet Management</h3>
              <div className="packet-gen-form">
                <div className="form-group">
                  <label>Source Endpoint</label>
                  <select value={genSrc} onChange={e => setGenSrc(e.target.value)}>
                    {endpointNodes.map(ep => (
                      <option key={ep.id} value={ep.id}>
                        {ep.id} ({ep.name || ep.id})
                      </option>
                    ))}
                  </select>
                </div>
                <div className="form-group">
                  <label>Destination Endpoint</label>
                  <select value={genDest} onChange={e => setGenDest(e.target.value)}>
                    {endpointNodes.map(ep => (
                      <option key={ep.id} value={ep.id}>
                        {ep.id} ({ep.name || ep.id})
                      </option>
                    ))}
                  </select>
                </div>
                <div className="form-group">
                  <label>Priority</label>
                  <select value={genPriority} onChange={e => setGenPriority(Number(e.target.value))}>
                    <option value={2}>HIGH (Priority 2)</option>
                    <option value={1}>MEDIUM (Priority 1)</option>
                    <option value={0}>LOW (Priority 0)</option>
                  </select>
                </div>
                <div className="form-group">
                  <label>Size (bytes)</label>
                  <input
                    type="number"
                    min="64"
                    max="1500"
                    value={genSize}
                    onChange={e => setGenSize(Number(e.target.value))}
                  />
                </div>
                <button className="btn btn-generate" onClick={handleGenerateSinglePacket}>
                  Generate Packet
                </button>
                <button className="btn btn-start" onClick={handleBulkGeneratePackets}>
                  Generate Bulk ({configPackets})
                </button>
              </div>

              {selectedPacket && (
                <PacketDetails packet={selectedPacket} onClose={() => setSelectedPacket(null)} />
              )}

              <div style={{ marginTop: '20px' }}>
                <PacketHistory
                  history={history}
                  onSearch={handleSearch}
                  onSort={handleSort}
                  onSelectPacket={setSelectedPacket}
                />
              </div>
            </div>
          )}

          {/* ==================== QUEUES VIEW ==================== */}
          {activeTab === 'queues' && (
            <div className="panel full-panel">
              <h3 className="panel-title">Queue Visualizations</h3>
              <QueueVisualization queues={queues} dsState={dsState} />
            </div>
          )}

          {/* ==================== DATA STRUCTURES VIEW ==================== */}
          {activeTab === 'datastructures' && (
            <div className="panel full-panel">
              <h3 className="panel-title">Data Structure Activity</h3>
              <DataStructures dsState={dsState} history={history} />
            </div>
          )}

          {/* ==================== HISTORY VIEW ==================== */}
          {activeTab === 'history' && (
            <div className="panel full-panel">
              <PacketHistory
                history={history}
                onSearch={handleSearch}
                onSort={handleSort}
                onSelectPacket={(p) => { setSelectedPacket(p); setActiveTab('packets'); }}
              />
            </div>
          )}

          {/* ==================== SEARCH & SORT VIEW ==================== */}
          {activeTab === 'search' && (
            <div className="panel full-panel">
              <PacketHistory
                history={history}
                onSearch={handleSearch}
                onSort={handleSort}
                onSelectPacket={(p) => { setSelectedPacket(p); setActiveTab('packets'); }}
                searchMode={true}
              />
              {selectedPacket && (
                <PacketDetails packet={selectedPacket} onClose={() => setSelectedPacket(null)} />
              )}
            </div>
          )}

          {/* ==================== STATISTICS VIEW ==================== */}
          {activeTab === 'stats' && (
            <div className="panel full-panel">
              <h3 className="panel-title">Network Statistics</h3>
              <div className="stats-grid">
                <div className="stat-box">
                  <div className="stat-label">Total Generated</div>
                  <div className="stat-value cyan">{ns.totalPackets || allPackets.length || 0}</div>
                </div>
                <div className="stat-box">
                  <div className="stat-label">Delivered</div>
                  <div className="stat-value success">{ns.delivered || 0}</div>
                </div>
                <div className="stat-box">
                  <div className="stat-label">Dropped</div>
                  <div className="stat-value danger">{ns.dropped || 0}</div>
                </div>
                <div className="stat-box">
                  <div className="stat-label">Delivery Rate</div>
                  <div className="stat-value success">
                    {ns.totalPackets > 0 ? ((ns.delivered / ns.totalPackets) * 100).toFixed(1) : 0}%
                  </div>
                </div>
                <div className="stat-box">
                  <div className="stat-label">Loss Rate</div>
                  <div className="stat-value danger">
                    {ns.totalPackets > 0 ? ((ns.dropped / ns.totalPackets) * 100).toFixed(1) : 0}%
                  </div>
                </div>
                <div className="stat-box">
                  <div className="stat-label">Avg Delay</div>
                  <div className="stat-value warning">{ns.averageDelay || 0}ms</div>
                </div>
                <div className="stat-box">
                  <div className="stat-label">Utilization</div>
                  <div className="stat-value warning">{Math.round(ns.networkUtilization || 0)}%</div>
                </div>
                <div className="stat-box">
                  <div className="stat-label">Pending / In Transit</div>
                  <div className="stat-value cyan">{ns.pending || 0}</div>
                </div>
              </div>
            </div>
          )}

          {/* ==================== ALERTS VIEW ==================== */}
          {activeTab === 'alerts' && (
            <div className="panel full-panel">
              <Alerts alerts={alerts} />
            </div>
          )}
        </div>
      </main>
    </div>
  );
};

export default Dashboard;
