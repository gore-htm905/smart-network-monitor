import React, { useMemo, useState, useEffect, useRef } from 'react';

const PRIORITY_COLORS = {
  HIGH: { bg: '#ef4444', text: '#ffffff', glow: 'rgba(239, 68, 68, 0.6)' },
  MEDIUM: { bg: '#f59e0b', text: '#000000', glow: 'rgba(245, 158, 11, 0.6)' },
  LOW: { bg: '#10b981', text: '#ffffff', glow: 'rgba(16, 185, 129, 0.6)' },
};

/**
 * Computes deterministic 2D coordinates for any arbitrary network topology.
 * Places routers in an inner ring / core mesh and endpoints on an outer perimeter.
 */
function computeLayout(nodes = []) {
  const width = 960;
  const height = 540;
  const cx = width / 2;
  const cy = height / 2;

  const positions = {};

  const sortNum = (a, b) => {
    const na = parseInt(a.id.replace(/\D/g, '')) || 0;
    const nb = parseInt(b.id.replace(/\D/g, '')) || 0;
    return na - nb;
  };

  const routers = nodes.filter(n => n.type === 'Router' || n.isRouter || n.id.startsWith('R')).sort(sortNum);
  const endpoints = nodes.filter(n => n.type !== 'Router' && !n.isRouter && !n.id.startsWith('R')).sort(sortNum);

  // Layout routers in inner ellipse
  const numRouters = routers.length;
  const rxRouter = Math.min(220, Math.max(120, 90 + numRouters * 15));
  const ryRouter = Math.min(150, Math.max(80, 60 + numRouters * 12));

  routers.forEach((r, idx) => {
    if (numRouters === 1) {
      positions[r.id] = { x: cx, y: cy };
    } else if (numRouters === 2) {
      positions[r.id] = { x: cx + (idx === 0 ? -130 : 130), y: cy };
    } else {
      const angle = -Math.PI / 2 + (2 * Math.PI * idx) / numRouters;
      positions[r.id] = {
        x: Math.round(cx + rxRouter * Math.cos(angle)),
        y: Math.round(cy + ryRouter * Math.sin(angle)),
      };
    }
  });

  // Layout endpoints in outer perimeter
  const numEndpoints = endpoints.length;
  const rxEnd = 410;
  const ryEnd = 225;

  endpoints.forEach((ep, idx) => {
    if (numEndpoints === 1) {
      positions[ep.id] = { x: cx, y: cy - ryEnd };
    } else {
      const angle = -Math.PI / 2 + (2 * Math.PI * idx) / numEndpoints;
      positions[ep.id] = {
        x: Math.round(cx + rxEnd * Math.cos(angle)),
        y: Math.round(cy + ryEnd * Math.sin(angle)),
      };
    }
  });

  return positions;
}

const NetworkTopology = ({
  topology,
  packets = [],
  selectedPacket,
  onSelectPacket,
  onToggleNode,
  isRunning = false
}) => {
  const nodes = topology?.nodes || [];
  const links = topology?.links || [];

  const positions = useMemo(() => computeLayout(nodes), [nodes]);

  // Smooth animation interpolation for in-transit packets
  const [animProgress, setAnimProgress] = useState(0);
  const lastTimeRef = useRef(null);
  const progressRef = useRef(0);

  useEffect(() => {
    let animationFrame;
    const animate = (timestamp) => {
      if (lastTimeRef.current !== null && isRunning) {
        const delta = timestamp - lastTimeRef.current;
        // Progress 0..1 every 1200ms
        progressRef.current = (progressRef.current + delta / 1200) % 1;
        setAnimProgress(progressRef.current);
      }
      lastTimeRef.current = timestamp;
      animationFrame = requestAnimationFrame(animate);
    };
    animationFrame = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(animationFrame);
  }, [isRunning]);

  // Filter packets that are active
  const inTransitPackets = packets.filter(p => p.status === 'In Transit' || p.status === 'Rerouted');

  // Highlight links belonging to selected packet route
  const selectedRouteEdges = useMemo(() => {
    if (!selectedPacket?.routeArr || selectedPacket.routeArr.length < 2) return new Set();
    const set = new Set();
    for (let i = 0; i < selectedPacket.routeArr.length - 1; i++) {
      const u = selectedPacket.routeArr[i];
      const v = selectedPacket.routeArr[i + 1];
      set.add(`${u}|${v}`);
      set.add(`${v}|${u}`);
    }
    return set;
  }, [selectedPacket]);

  return (
    <div style={{
      position: 'relative',
      width: '100%',
      height: '520px',
      backgroundColor: '#070d19',
      borderRadius: '10px',
      border: '1px solid rgba(255, 255, 255, 0.08)',
      overflow: 'hidden'
    }}>
      {/* Topology Top Status Bar */}
      <div style={{
        position: 'absolute',
        top: 12,
        left: 16,
        right: 16,
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        zIndex: 10,
        pointerEvents: 'none'
      }}>
        <div style={{ display: 'flex', gap: '12px', alignItems: 'center' }}>
          <span style={{
            fontSize: '0.75rem',
            color: '#94a3b8',
            background: 'rgba(15, 23, 42, 0.8)',
            padding: '4px 10px',
            borderRadius: '6px',
            border: '1px solid rgba(255,255,255,0.1)'
          }}>
            Nodes: <strong style={{ color: '#38bdf8' }}>{nodes.length}</strong> ({nodes.filter(n => n.type === 'Router' || n.isRouter).length} Routers, {nodes.filter(n => n.type !== 'Router' && !n.isRouter).length} Endpoints)
          </span>
          <span style={{
            fontSize: '0.75rem',
            color: '#94a3b8',
            background: 'rgba(15, 23, 42, 0.8)',
            padding: '4px 10px',
            borderRadius: '6px',
            border: '1px solid rgba(255,255,255,0.1)'
          }}>
            Active Links: <strong style={{ color: '#10b981' }}>{links.filter(l => l.active).length}</strong>
          </span>
          {inTransitPackets.length > 0 && (
            <span style={{
              fontSize: '0.75rem',
              color: '#f59e0b',
              background: 'rgba(245, 158, 11, 0.15)',
              padding: '4px 10px',
              borderRadius: '6px',
              border: '1px solid #f59e0b'
            }}>
              ● {inTransitPackets.length} In Transit
            </span>
          )}
        </div>

        {selectedPacket && (
          <div style={{
            fontSize: '0.8rem',
            color: '#38bdf8',
            background: 'rgba(15, 23, 42, 0.9)',
            padding: '5px 14px',
            borderRadius: '6px',
            border: '1px solid #38bdf8',
            pointerEvents: 'auto',
            display: 'flex',
            gap: '8px',
            alignItems: 'center'
          }}>
            <span>Selected: <strong>{selectedPacket.id}</strong> ({selectedPacket.priority})</span>
            <span style={{ color: '#94a3b8' }}>|</span>
            <span style={{ fontFamily: 'monospace' }}>{selectedPacket.route}</span>
            <button
              onClick={() => onSelectPacket && onSelectPacket(null)}
              style={{
                background: 'none', border: 'none', color: '#94a3b8', cursor: 'pointer', padding: '0 4px', fontSize: '0.9rem'
              }}
            >✕</button>
          </div>
        )}
      </div>

      {/* SVG Canvas */}
      <svg width="100%" height="100%" viewBox="0 0 960 540" style={{ display: 'block' }}>
        <defs>
          <filter id="topo-glow" x="-20%" y="-20%" width="140%" height="140%">
            <feGaussianBlur stdDeviation="4" result="coloredBlur" />
            <feMerge>
              <feMergeNode in="coloredBlur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
          <linearGradient id="link-grad" x1="0%" y1="0%" x2="100%" y2="100%">
            <stop offset="0%" stopColor="#38bdf8" stopOpacity="0.8" />
            <stop offset="100%" stopColor="#818cf8" stopOpacity="0.8" />
          </linearGradient>
          <linearGradient id="route-highlight-grad" x1="0%" y1="0%" x2="100%" y2="100%">
            <stop offset="0%" stopColor="#f59e0b" stopOpacity="1" />
            <stop offset="100%" stopColor="#ef4444" stopOpacity="1" />
          </linearGradient>
        </defs>

        {/* 1. Network Links */}
        {(() => {
          // Build a set of offline node IDs for link rendering
          const offlineNodeSet = new Set(
            nodes.filter(n => n.status === 'Inactive').map(n => n.id)
          );

          return links.map((link, i) => {
            const src = positions[link.source];
            const tgt = positions[link.target];
            if (!src || !tgt) return null;

            const isHighlighted = selectedRouteEdges.has(`${link.source}|${link.target}`);
            const isRouterLink = link.source.startsWith('R') && link.target.startsWith('R');

            // A link is operationally inactive if it's disabled OR if either endpoint is offline
            const operationallyActive = link.active &&
              !offlineNodeSet.has(link.source) &&
              !offlineNodeSet.has(link.target);

            let stroke = operationallyActive ? 'rgba(56, 189, 248, 0.4)' : 'rgba(239, 68, 68, 0.5)';
            let strokeWidth = isRouterLink ? 2.5 : 1.5;
            let strokeDash = operationallyActive ? (isRouterLink ? 'none' : '4 3') : '6 4';

            if (isHighlighted && operationallyActive) {
              stroke = '#f59e0b';
              strokeWidth = 4;
              strokeDash = 'none';
            }

            // Midpoint for weight badge
            const midX = (src.x + tgt.x) / 2;
            const midY = (src.y + tgt.y) / 2;

            return (
              <g key={`link-${link.id || i}`}>
                <line
                  x1={src.x} y1={src.y} x2={tgt.x} y2={tgt.y}
                  stroke={stroke}
                  strokeWidth={strokeWidth}
                  strokeDasharray={strokeDash}
                  filter={isHighlighted && operationallyActive ? 'url(#topo-glow)' : undefined}
                />
                {/* Optional small weight indicator on router-router links */}
                {isRouterLink && (
                  <g transform={`translate(${midX}, ${midY})`}>
                    <circle r="8" fill="#0b1329" stroke="rgba(255,255,255,0.2)" strokeWidth="1" />
                    <text y="3" textAnchor="middle" fill="#94a3b8" fontSize="8px" fontWeight="600">
                      {link.weight || 1}
                    </text>
                  </g>
                )}
              </g>
            );
          });
        })()}

        {/* 2. In-Transit Packets Animation */}
        {(() => {
          // Build a set of offline node IDs for quick lookup
          const offlineNodes = new Set(
            nodes.filter(n => n.status === 'Inactive').map(n => n.id)
          );

          return inTransitPackets.map((pkt) => {
            const route = pkt.routeArr || [];
            const hopIdx = pkt.hopIndex || 0;
            const curId = pkt.currentNode || (route[hopIdx]);
            const nextId = (hopIdx + 1 < route.length) ? route[hopIdx + 1] : pkt.destination;

            const srcPos = positions[curId];
            const tgtPos = positions[nextId];

            // If current or next node is offline, don't animate through them.
            // Show a warning indicator at the packet's last known valid position.
            if (offlineNodes.has(curId) || offlineNodes.has(nextId)) {
              const warningPos = positions[curId] || positions[pkt.source];
              if (!warningPos) return null;
              return (
                <g
                  key={`transit-warn-${pkt.id}`}
                  transform={`translate(${warningPos.x}, ${warningPos.y - 20})`}
                  style={{ cursor: 'pointer' }}
                  onClick={() => onSelectPacket && onSelectPacket(pkt)}
                >
                  {/* Pulsing warning */}
                  <circle r="11" fill="rgba(239, 68, 68, 0.5)" opacity="0.7">
                    <animate attributeName="r" values="9;15;9" dur="1s" repeatCount="indefinite" />
                  </circle>
                  <circle r="7" fill="#ef4444" stroke="#fff" strokeWidth="1.5" />
                  <text y="3" textAnchor="middle" fill="#fff" fontSize="8px" fontWeight="bold">⚠</text>
                  <g transform="translate(0, -14)">
                    <rect x="-28" y="-12" width="56" height="12" rx="3" fill="#0f172a" stroke="#ef4444" strokeWidth="1" />
                    <text y="-3" textAnchor="middle" fill="#ef4444" fontSize="7px" fontWeight="bold">
                      {pkt.id} BLOCKED
                    </text>
                  </g>
                </g>
              );
            }

            if (!srcPos || !tgtPos) return null;

            // Interpolated position
            const px = srcPos.x + (tgtPos.x - srcPos.x) * animProgress;
            const py = srcPos.y + (tgtPos.y - srcPos.y) * animProgress;

            const pColors = PRIORITY_COLORS[pkt.priority] || PRIORITY_COLORS.LOW;
            const isRerouted = pkt.status === 'Rerouted' || pkt.rerouteCount > 0;

            return (
              <g
                key={`transit-${pkt.id}`}
                transform={`translate(${px}, ${py})`}
                style={{ cursor: 'pointer' }}
                onClick={() => onSelectPacket && onSelectPacket(pkt)}
              >
                {/* Glowing aura */}
                <circle r="14" fill={isRerouted ? 'rgba(245, 158, 11, 0.6)' : pColors.glow} opacity="0.5" filter="url(#topo-glow)" />
                {/* Main packet badge */}
                <circle r="9" fill={pColors.bg} stroke={isRerouted ? '#f59e0b' : '#ffffff'} strokeWidth={isRerouted ? 2.5 : 1.5} />
                <text y="3" textAnchor="middle" fill={pColors.text} fontSize="8px" fontWeight="bold">
                  {pkt.id.replace('P', '')}
                </text>
                {/* Packet ID tooltip banner */}
                <g transform="translate(0, -14)">
                  <rect x="-20" y="-12" width="40" height="12" rx="3" fill="#0f172a" stroke={isRerouted ? '#f59e0b' : pColors.bg} strokeWidth="1" />
                  <text y="-3" textAnchor="middle" fill="#ffffff" fontSize="8px" fontWeight="bold">
                    {pkt.id}
                  </text>
                </g>
                {/* Reroute indicator */}
                {isRerouted && (
                  <g transform="translate(12, -8)">
                    <circle r="6" fill="#f59e0b" stroke="#070d19" strokeWidth="1" />
                    <text y="3" textAnchor="middle" fill="#000" fontSize="7px" fontWeight="bold">↻</text>
                  </g>
                )}
              </g>
            );
          });
        })()}

        {/* 3. Nodes Rendering */}
        {nodes.map((node) => {
          const pos = positions[node.id];
          if (!pos) return null;

          const isRouter = node.type === 'Router' || node.isRouter;
          const isOffline = node.status === 'Inactive';
          const r = isRouter ? 26 : 18;

          // Colors
          let stroke = isRouter ? '#38bdf8' : '#06b6d4';
          let fill = isRouter ? 'rgba(30, 58, 138, 0.45)' : 'rgba(6, 182, 212, 0.2)';

          if (isOffline) {
            stroke = '#ef4444';
            fill = 'rgba(239, 68, 68, 0.2)';
          }

          // Router queue capacity
          const qSize = node.queueSize || 0;
          const qCap = node.queueCapacity || 20;
          const qUtil = qCap > 0 ? (qSize / qCap) : 0;
          const qBarColor = qUtil >= 0.85 ? '#ef4444' : qUtil >= 0.5 ? '#f59e0b' : '#10b981';

          return (
            <g
              key={node.id}
              style={{ cursor: 'pointer' }}
              onClick={() => onToggleNode && onToggleNode(node.id, isOffline)}
            >
              <title>{`${node.name || node.id} (${node.type}) - Click to toggle ${isOffline ? 'ONLINE' : 'OFFLINE'}`}</title>

              {/* Node Glow Outer Ring */}
              <circle
                cx={pos.x} cy={pos.y} r={r + 5}
                fill="none" stroke={stroke} strokeWidth="1"
                opacity={isOffline ? 0.2 : 0.4} filter="url(#topo-glow)"
              />

              {/* Node Body */}
              <circle
                cx={pos.x} cy={pos.y} r={r}
                fill={fill} stroke={stroke} strokeWidth={isRouter ? 2.5 : 2}
              />

              {/* Node ID */}
              <text
                x={pos.x} y={isRouter ? pos.y - 3 : pos.y + 4}
                fill="#ffffff" textAnchor="middle"
                fontSize={isRouter ? '13px' : '11px'} fontWeight="bold"
              >
                {node.id}
              </text>

              {/* Subtitle for router */}
              {isRouter && (
                <text x={pos.x} y={pos.y + 11} fill="#94a3b8" textAnchor="middle" fontSize="8px" fontWeight="600">
                  ROUTER
                </text>
              )}

              {/* Router Queue Display: R1 Queue: 4 / 20 */}
              {isRouter && (
                <g transform={`translate(${pos.x}, ${pos.y + r + 6})`}>
                  {/* Badge background */}
                  <rect
                    x="-34" y="0" width="68" height="19" rx="4"
                    fill="#0a1224" stroke={stroke} strokeWidth="1"
                  />
                  {/* Queue Text */}
                  <text x="0" y="10" fill="#e2e8f0" textAnchor="middle" fontSize="8.5px" fontWeight="bold">
                    Queue: {qSize}/{qCap}
                  </text>
                  {/* Fill progress mini-bar */}
                  <rect
                    x="-32" y="14" width="64" height="2" rx="1"
                    fill="rgba(255,255,255,0.1)"
                  />
                  <rect
                    x="-32" y="14" width={Math.max(0, Math.min(64, 64 * qUtil))} height="2" rx="1"
                    fill={qBarColor}
                  />
                </g>
              )}

              {/* Endpoint Queue Indicator if waiting packets */}
              {!isRouter && qSize > 0 && (
                <g transform={`translate(${pos.x + r - 4}, ${pos.y - r + 4})`}>
                  <circle r="8" fill="#f59e0b" stroke="#070d19" strokeWidth="1.5" />
                  <text y="3" fill="#000000" textAnchor="middle" fontSize="8px" fontWeight="bold">
                    {qSize}
                  </text>
                </g>
              )}

              {/* Offline Warning Tag */}
              {isOffline && (
                <g transform={`translate(${pos.x}, ${pos.y + r + (isRouter ? 28 : 14)})`}>
                  <rect x="-24" y="-8" width="48" height="15" rx="3" fill="#ef4444" />
                  <text y="3" fill="#ffffff" textAnchor="middle" fontSize="8.5px" fontWeight="bold">
                    OFFLINE
                  </text>
                </g>
              )}

              {/* Endpoint Device Label above node */}
              {!isRouter && (
                <text
                  x={pos.x} y={pos.y - r - 6}
                  fill="#94a3b8" textAnchor="middle" fontSize="10px"
                >
                  {node.name || node.id}
                </text>
              )}
            </g>
          );
        })}
      </svg>

      {/* Legend & Help at bottom */}
      <div style={{
        position: 'absolute',
        bottom: 8,
        left: 16,
        right: 16,
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        fontSize: '0.72rem',
        color: '#64748b',
        pointerEvents: 'none'
      }}>
        <div style={{ display: 'flex', gap: '16px', alignItems: 'center' }}>
          <span style={{ display: 'flex', alignItems: 'center', gap: '5px' }}>
            <span style={{ width: '10px', height: '10px', borderRadius: '50%', background: '#38bdf8', display: 'inline-block' }}></span>
            Router
          </span>
          <span style={{ display: 'flex', alignItems: 'center', gap: '5px' }}>
            <span style={{ width: '8px', height: '8px', borderRadius: '50%', background: '#06b6d4', display: 'inline-block' }}></span>
            Endpoint
          </span>
          <span style={{ display: 'flex', alignItems: 'center', gap: '5px' }}>
            <span style={{ width: '8px', height: '8px', borderRadius: '50%', background: '#ef4444', display: 'inline-block' }}></span>
            Offline Node
          </span>
          <span style={{ display: 'flex', alignItems: 'center', gap: '5px' }}>
            <span style={{ width: '14px', height: '2px', background: '#38bdf8', display: 'inline-block' }}></span>
            Active Link
          </span>
        </div>
        <div>
          <span>Tip: Click any node to simulate failure/restore</span>
        </div>
      </div>
    </div>
  );
};

export default NetworkTopology;
