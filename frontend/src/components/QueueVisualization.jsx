import React from 'react';

const PRIORITY_COLORS = {
  HIGH: { bg: 'rgba(239, 68, 68, 0.2)', border: '#ef4444', text: '#ef4444' },
  MEDIUM: { bg: 'rgba(245, 158, 11, 0.2)', border: '#f59e0b', text: '#f59e0b' },
  LOW: { bg: 'rgba(16, 185, 129, 0.2)', border: '#10b981', text: '#10b981' },
};

const QueueVisualization = ({ queues, dsState }) => {
  const pqItems = dsState?.priorityQueue || [];
  const nodeQueues = queues?.nodeQueues || {};

  // Group priority queue items by priority
  const highItems = pqItems.filter(p => p.priority === 'HIGH');
  const medItems = pqItems.filter(p => p.priority === 'MEDIUM');
  const lowItems = pqItems.filter(p => p.priority === 'LOW');

  // Separate router queues and endpoint queues
  const routerQueues = Object.entries(nodeQueues).filter(([id]) => id.startsWith('R'));
  const endpointQueues = Object.entries(nodeQueues).filter(([id]) => !id.startsWith('R'));

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '20px', color: 'var(--text-main)' }}>
      {/* 1. Router Queues */}
      <div>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '10px' }}>
          <h4 style={{ color: '#38bdf8', fontSize: '0.85rem', letterSpacing: '1px', textTransform: 'uppercase', margin: 0 }}>
            Router Queues (Priority Processed)
          </h4>
          <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>
            High priority dequeued first
          </span>
        </div>

        {routerQueues.length === 0 && (
          <div style={{ color: 'var(--text-muted)', fontSize: '0.85rem' }}>No routers generated yet.</div>
        )}

        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
          {routerQueues.map(([id, nq]) => (
            <div key={id} style={{
              background: 'rgba(15, 23, 42, 0.6)',
              border: '1px solid rgba(56, 189, 248, 0.2)',
              borderRadius: '8px',
              padding: '10px 14px'
            }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '6px' }}>
                <span style={{ fontWeight: 'bold', color: '#38bdf8', fontSize: '0.9rem' }}>{id}</span>
                <span style={{
                  fontSize: '0.8rem',
                  fontWeight: 600,
                  color: nq.size > 0 ? '#f59e0b' : '#94a3b8'
                }}>
                  Queue: {nq.size} / {nq.capacity}
                </span>
              </div>

              {/* Progress bar */}
              <div style={{ height: '6px', backgroundColor: 'rgba(255,255,255,0.08)', borderRadius: '3px', overflow: 'hidden', marginBottom: '8px' }}>
                <div style={{
                  width: `${Math.min(100, (nq.size / (nq.capacity || 20)) * 100)}%`,
                  height: '100%',
                  borderRadius: '3px',
                  backgroundColor: (nq.size / (nq.capacity || 20)) >= 0.85 ? 'var(--danger)' : (nq.size / (nq.capacity || 20)) >= 0.5 ? 'var(--warning)' : '#38bdf8',
                  transition: 'width 0.4s ease'
                }} />
              </div>

              {/* Queued Packets badges */}
              <div style={{ display: 'flex', gap: '6px', flexWrap: 'wrap', alignItems: 'center', minHeight: '26px' }}>
                {nq.packets && nq.packets.length > 0 ? (
                  nq.packets.map((p, i) => {
                    const col = PRIORITY_COLORS[p.priority] || PRIORITY_COLORS.LOW;
                    return (
                      <span key={i} style={{
                        backgroundColor: col.bg,
                        border: `1px solid ${col.border}`,
                        color: col.text,
                        padding: '2px 8px',
                        borderRadius: '4px',
                        fontSize: '0.75rem',
                        fontWeight: 600,
                        display: 'inline-flex',
                        alignItems: 'center',
                        gap: '4px'
                      }}>
                        {p.id} <span style={{ fontSize: '0.65rem', opacity: 0.8 }}>({p.priority?.[0]})</span>
                      </span>
                    );
                  })
                ) : (
                  <span style={{ color: 'var(--text-muted)', fontSize: '0.75rem', fontStyle: 'italic' }}>Queue idle (0 packets)</span>
                )}
              </div>
            </div>
          ))}
        </div>
      </div>

      {/* 2. Priority Queue (Transit Dispatch) */}
      <div>
        <h4 style={{ color: 'var(--text-muted)', marginBottom: '10px', fontSize: '0.85rem', letterSpacing: '1px', textTransform: 'uppercase' }}>
          Transit Dispatch Priority Queue
        </h4>
        {[
          { label: 'HIGH', items: highItems, colors: PRIORITY_COLORS.HIGH },
          { label: 'MEDIUM', items: medItems, colors: PRIORITY_COLORS.MEDIUM },
          { label: 'LOW', items: lowItems, colors: PRIORITY_COLORS.LOW },
        ].map(({ label, items, colors }) => (
          <div key={label} style={{ display: 'flex', alignItems: 'center', gap: '8px', marginBottom: '8px' }}>
            <span style={{ width: '70px', color: colors.text, fontSize: '0.8rem', fontWeight: 600 }}>{label}</span>
            <div style={{ display: 'flex', gap: '6px', flexWrap: 'wrap' }}>
              {items.length > 0 ? items.map((p, i) => (
                <div key={i} style={{
                  backgroundColor: colors.bg, border: `1px solid ${colors.border}`,
                  padding: '3px 10px', borderRadius: '4px', fontSize: '0.8rem'
                }}>{p.id}</div>
              )) : <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>—</span>}
            </div>
          </div>
        ))}
      </div>

      {/* 3. Endpoint FIFO Queues */}
      {endpointQueues.some(([, nq]) => nq.size > 0) && (
        <div>
          <h4 style={{ color: 'var(--text-muted)', marginBottom: '10px', fontSize: '0.85rem', letterSpacing: '1px', textTransform: 'uppercase' }}>
            Active Endpoint Queues
          </h4>
          <div style={{ display: 'flex', flexWrap: 'wrap', gap: '8px' }}>
            {endpointQueues.filter(([, nq]) => nq.size > 0).map(([id, nq]) => (
              <div key={id} style={{
                background: 'rgba(6, 182, 212, 0.1)',
                border: '1px solid rgba(6, 182, 212, 0.3)',
                padding: '4px 10px',
                borderRadius: '6px',
                fontSize: '0.8rem'
              }}>
                <strong style={{ color: '#06b6d4' }}>{id}:</strong> {nq.size} packets
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
};

export default QueueVisualization;
