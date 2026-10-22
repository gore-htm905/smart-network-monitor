import React, { useState } from 'react';

const TABS = ['Priority Queue', 'Doubly Linked List', 'Circular Linked List', 'Stack'];

const DataStructures = ({ dsState, _history }) => {
  const [activeTab, setActiveTab] = useState('Priority Queue');

  const pq = dsState?.priorityQueue || [];
  const cll = dsState?.circularLinkedList || [];
  const stacks = dsState?.stacks || {};
  const dll = dsState?.doublyLinkedList || [];

  return (
    <div style={{ color: 'var(--text-main)' }}>
      {/* Tabs */}
      <div style={{ display: 'flex', gap: '4px', marginBottom: '20px', borderBottom: '1px solid var(--border-color)', paddingBottom: '10px' }}>
        {TABS.map(tab => (
          <button key={tab} onClick={() => setActiveTab(tab)} style={{
            padding: '8px 16px', borderRadius: '6px 6px 0 0', cursor: 'pointer', fontSize: '0.85rem', fontWeight: 500,
            border: 'none',
            background: activeTab === tab ? 'rgba(6, 182, 212, 0.15)' : 'transparent',
            color: activeTab === tab ? 'var(--accent-cyan)' : 'var(--text-muted)',
            borderBottom: activeTab === tab ? '2px solid var(--accent-cyan)' : '2px solid transparent',
          }}>{tab}</button>
        ))}
      </div>

      {/* Priority Queue */}
      {activeTab === 'Priority Queue' && (
        <div>
          <h4 style={{ color: 'var(--text-muted)', marginBottom: '15px' }}>Priority Queue (Max Heap)</h4>
          {pq.length === 0 && <p style={{ color: 'var(--text-muted)' }}>Priority Queue is empty.</p>}
          <div style={{ display: 'flex', gap: '10px', flexWrap: 'wrap' }}>
            {pq.map((p, i) => (
              <div key={i} style={{
                padding: '10px 16px', borderRadius: '8px', textAlign: 'center',
                background: p.priority === 'HIGH' ? 'rgba(239,68,68,0.15)' : p.priority === 'MEDIUM' ? 'rgba(245,158,11,0.15)' : 'rgba(16,185,129,0.15)',
                border: `1px solid ${p.priority === 'HIGH' ? '#ef4444' : p.priority === 'MEDIUM' ? '#f59e0b' : '#10b981'}`,
              }}>
                <div style={{ fontWeight: 'bold', fontSize: '0.9rem' }}>{p.id}</div>
                <div style={{ fontSize: '0.7rem', color: 'var(--text-muted)' }}>{p.priority}</div>
              </div>
            ))}
          </div>
        </div>
      )}

      {/* Doubly Linked List */}
      {activeTab === 'Doubly Linked List' && (
        <div>
          <h4 style={{ color: 'var(--text-muted)', marginBottom: '15px' }}>Doubly Linked List (Packet History)</h4>
          <div style={{ display: 'flex', alignItems: 'center', gap: '4px', overflowX: 'auto', padding: '10px 0' }}>
            <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem', marginRight: '8px' }}>NULL ←</span>
            {dll.map((p, i) => (
              <React.Fragment key={i}>
                <div style={{
                  padding: '8px 14px', borderRadius: '6px', border: '1px solid var(--accent-cyan)',
                  background: 'rgba(6, 182, 212, 0.1)', fontSize: '0.85rem', fontWeight: 500, whiteSpace: 'nowrap'
                }}>
                  {p.id}
                </div>
                {i < dll.length - 1 && <span style={{ color: 'var(--accent-cyan)', fontSize: '0.9rem' }}>⇄</span>}
              </React.Fragment>
            ))}
            {dll.length === 0 && <span style={{ color: 'var(--text-muted)' }}>History empty</span>}
            <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem', marginLeft: '8px' }}>→ NULL</span>
          </div>
        </div>
      )}

      {/* Circular Linked List */}
      {activeTab === 'Circular Linked List' && (
        <div>
          <h4 style={{ color: 'var(--text-muted)', marginBottom: '15px' }}>Circular Linked List (Round-Robin Scheduler)</h4>
          {cll.length === 0 && <p style={{ color: 'var(--text-muted)' }}>Scheduler is empty.</p>}
          <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', minHeight: '200px' }}>
            <svg width="300" height="300" viewBox="0 0 300 300">
              {cll.map((node, i) => {
                const angle = (i / cll.length) * 2 * Math.PI - Math.PI / 2;
                const x = 150 + 100 * Math.cos(angle);
                const y = 150 + 100 * Math.sin(angle);
                const nextAngle = ((i + 1) % cll.length / cll.length) * 2 * Math.PI - Math.PI / 2;
                const nx = 150 + 100 * Math.cos(nextAngle);
                const ny = 150 + 100 * Math.sin(nextAngle);

                return (
                  <g key={i}>
                    <line x1={x} y1={y} x2={nx} y2={ny} stroke={node.isActive ? '#06b6d4' : '#ef4444'} strokeWidth="2" opacity="0.5" />
                    <circle cx={x} cy={y} r="22" fill={node.isActive ? 'rgba(6,182,212,0.2)' : 'rgba(239,68,68,0.2)'} stroke={node.isActive ? '#06b6d4' : '#ef4444'} strokeWidth="2" />
                    <text x={x} y={y + 4} fill="white" textAnchor="middle" fontSize="12px" fontWeight="bold">{node.nodeId}</text>
                  </g>
                );
              })}
              <text x="150" y="155" fill="var(--text-muted)" textAnchor="middle" fontSize="10px">Round-Robin</text>
              <text x="150" y="170" fill="var(--text-muted)" textAnchor="middle" fontSize="10px">Scheduler</text>
            </svg>
          </div>
        </div>
      )}

      {/* Stack */}
      {activeTab === 'Stack' && (() => {
        const stackEntries = Object.entries(stacks).sort((a, b) =>
          a[0].localeCompare(b[0], undefined, { numeric: true })
        );

        return (
          <div>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '15px' }}>
              <h4 style={{ color: 'var(--text-muted)', margin: 0 }}>
                Packet Route Stacks ({stackEntries.length})
              </h4>
              <span style={{ fontSize: '0.8rem', color: 'var(--text-muted)' }}>
                Showing full planned/remaining route stacks for all packets
              </span>
            </div>
            {stackEntries.length === 0 && <p style={{ color: 'var(--text-muted)' }}>No route stacks yet.</p>}
            <div style={{
              display: 'flex',
              gap: '20px',
              flexWrap: 'wrap',
              maxHeight: '600px',
              overflowY: 'auto',
              padding: '10px 4px'
            }}>
              {stackEntries.map(([packetId, stack]) => (
                <div key={packetId} style={{
                  textAlign: 'center',
                  background: 'rgba(255, 255, 255, 0.02)',
                  padding: '12px 14px',
                  borderRadius: '8px',
                  border: '1px solid rgba(255, 255, 255, 0.06)',
                  minWidth: '90px'
                }}>
                  <div style={{ fontSize: '0.85rem', color: 'var(--accent-cyan)', marginBottom: '8px', fontWeight: 700 }}>
                    {packetId}
                  </div>
                  <div style={{ display: 'flex', flexDirection: 'column', gap: '4px', alignItems: 'center' }}>
                    <span style={{ fontSize: '0.7rem', color: 'var(--text-muted)', fontWeight: 600 }}>TOP</span>
                    {stack.map((hop, i) => (
                      <div key={i} style={{
                        padding: '6px 16px', borderRadius: '4px',
                        border: '1px solid var(--accent-purple)', background: 'rgba(139,92,246,0.15)',
                        fontSize: '0.85rem', fontWeight: 600, color: '#e2e8f0', width: '100%', boxSizing: 'border-box'
                      }}>{hop}</div>
                    ))}
                  </div>
                </div>
              ))}
            </div>
          </div>
        );
      })()}
    </div>
  );
};

export default DataStructures;
