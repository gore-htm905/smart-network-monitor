import React from 'react';

const PacketDetails = ({ packet, onClose }) => {
  if (!packet) return null;

  const timeline = packet.timeline || [];
  const routeStack = packet.routeStack || [];

  return (
    <div style={{
      marginTop: '20px', padding: '25px', borderRadius: '12px',
      background: 'rgba(255,255,255,0.03)', border: '1px solid var(--border-color)', color: 'var(--text-main)'
    }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '20px' }}>
        <h3 style={{ color: 'var(--accent-cyan)', margin: 0 }}>PACKET {packet.id}</h3>
        <button onClick={onClose} style={{
          background: 'none', border: '1px solid var(--border-color)', color: 'var(--text-muted)',
          padding: '4px 12px', borderRadius: '4px', cursor: 'pointer'
        }}>✕ Close</button>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1.2fr 1.2fr 1.6fr', gap: '20px', marginBottom: '25px' }}>
        {/* Details */}
        <div>
          <h4 style={{ color: 'var(--text-muted)', fontSize: '0.8rem', marginBottom: '12px' }}>DETAILS</h4>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', fontSize: '0.88rem' }}>
            <div><span style={{ color: 'var(--text-muted)' }}>Source: </span><strong>{packet.source}</strong></div>
            <div><span style={{ color: 'var(--text-muted)' }}>Destination: </span><strong>{packet.destination}</strong></div>
            <div><span style={{ color: 'var(--text-muted)' }}>Priority: </span>
              <span style={{ color: packet.priority === 'HIGH' ? 'var(--danger)' : packet.priority === 'MEDIUM' ? 'var(--warning)' : 'var(--success)', fontWeight: 600 }}>
                {packet.priority}
              </span>
            </div>
            <div><span style={{ color: 'var(--text-muted)' }}>Size: </span>{packet.size} bytes</div>
            <div><span style={{ color: 'var(--text-muted)' }}>Status: </span>
              <span style={{
                color: packet.status === 'Delivered' ? 'var(--success)' : packet.status === 'Dropped' ? 'var(--danger)' : 'var(--warning)',
                fontWeight: 600
              }}>
                {packet.status}
              </span>
            </div>
            {packet.dropReason && (
              <div style={{
                background: 'rgba(239, 68, 68, 0.12)', border: '1px solid rgba(239, 68, 68, 0.3)',
                padding: '6px 10px', borderRadius: '6px', color: '#fca5a5', fontSize: '0.82rem'
              }}>
                <strong>Drop Reason:</strong> {packet.dropReason}
                {packet.failedNextHop && (
                  <div style={{ marginTop: '2px', fontSize: '0.78rem' }}>
                    <strong>Failed Next Hop:</strong> {packet.failedNextHop}
                  </div>
                )}
              </div>
            )}
            <div><span style={{ color: 'var(--text-muted)' }}>Last Valid Position: </span>
              <strong>{packet.lastValidNode || packet.currentNode || packet.source}</strong>
            </div>
            <div><span style={{ color: 'var(--text-muted)' }}>Delay: </span>{packet.delay > 0 ? `${packet.delay}ms` : '—'}</div>
            {packet.rerouteCount > 0 && (
              <div><span style={{ color: 'var(--warning)' }}>Reroutes: </span>{packet.rerouteCount}</div>
            )}
          </div>
        </div>

        {/* Route History & Stack */}
        <div>
          <h4 style={{ color: 'var(--text-muted)', fontSize: '0.8rem', marginBottom: '12px' }}>ROUTE EXECUTION</h4>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', fontSize: '0.82rem' }}>
            <div>
              <span style={{ color: 'var(--text-muted)', display: 'block', marginBottom: '2px' }}>Planned Route:</span>
              <span style={{ fontFamily: 'monospace', color: 'var(--accent-cyan)' }}>
                {packet.initialRoute || packet.route || '—'}
              </span>
            </div>
            <div>
              <span style={{ color: 'var(--text-muted)', display: 'block', marginBottom: '2px' }}>Actual Traversed Path:</span>
              <span style={{
                fontFamily: 'monospace',
                color: packet.status === 'Delivered' ? 'var(--success)' : packet.status === 'Dropped' ? 'var(--warning)' : '#e2e8f0'
              }}>
                {packet.traversedNodes && packet.traversedNodes.length > 0
                  ? packet.traversedNodes.join(' → ')
                  : (packet.currentNode || packet.source)}
              </span>
            </div>
            {packet.status === 'Dropped' && packet.failedNextHop && (
              <div>
                <span style={{ color: 'var(--danger)', display: 'block', marginBottom: '2px' }}>Blocked Before Reaching:</span>
                <span style={{ fontFamily: 'monospace', color: 'var(--danger)' }}>
                  ✗ {packet.failedNextHop}
                </span>
              </div>
            )}
          </div>

          <h4 style={{ color: 'var(--text-muted)', fontSize: '0.8rem', marginTop: '16px', marginBottom: '8px' }}>CURRENT ROUTE STACK</h4>
          {routeStack.length > 0 ? (
            <div style={{ display: 'flex', flexDirection: 'column', gap: '3px', alignItems: 'flex-start' }}>
              <span style={{ fontSize: '0.7rem', color: 'var(--accent-purple)', fontWeight: 600 }}>TOP</span>
              {routeStack.map((hop, i) => (
                <div key={i} style={{
                  padding: '4px 14px', borderRadius: '4px', width: '90px', textAlign: 'center',
                  border: '1px solid var(--accent-purple)', background: 'rgba(139,92,246,0.15)', fontSize: '0.82rem'
                }}>{hop}</div>
              ))}
            </div>
          ) : <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>No stack data</span>}
        </div>

        {/* Processing Timeline */}
        <div>
          <h4 style={{ color: 'var(--text-muted)', fontSize: '0.8rem', marginBottom: '12px' }}>PROCESSING TIMELINE</h4>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '6px', maxHeight: '250px', overflowY: 'auto' }}>
            {timeline.length > 0 ? timeline.map((step, i) => (
              <div key={i} style={{ display: 'flex', alignItems: 'flex-start', gap: '8px', fontSize: '0.82rem' }}>
                <span style={{ color: step.includes('DROPPED') ? 'var(--danger)' : step.includes('REROUTED') ? 'var(--warning)' : 'var(--success)' }}>
                  {step.includes('DROPPED') ? '✕' : step.includes('REROUTED') ? '↻' : '✓'}
                </span>
                <span style={{ color: step.includes('DROPPED') ? '#fca5a5' : step.includes('REROUTED') ? '#fcd34d' : '#e2e8f0' }}>
                  {step}
                </span>
              </div>
            )) : <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>No timeline events recorded.</span>}
          </div>
        </div>
      </div>
    </div>
  );
};

export default PacketDetails;
