import React, { useState, useMemo } from 'react';

const PRIORITY_COLOR = { HIGH: 'var(--danger)', MEDIUM: 'var(--warning)', LOW: 'var(--success)' };
const STATUS_COLOR = {
  Delivered: '#10b981',
  'In Transit': '#38bdf8',
  Queued: '#f59e0b',
  Created: '#94a3b8',
  Dropped: '#ef4444',
  Rerouted: '#fb923c',
  Lost: '#ef4444',
  Waiting: '#f59e0b',
  Transmitting: '#38bdf8'
};

const PacketHistory = ({ history = [], onSearch, onSort, onSelectPacket, searchMode = false }) => {
  const [searchId, setSearchId] = useState('');
  const [appliedSearch, setAppliedSearch] = useState('');
  const [sortField, setSortField] = useState('id');
  const [sortOrder, setSortOrder] = useState('asc');

  const handleSearch = () => {
    const trimmed = searchId.trim();
    setAppliedSearch(trimmed);
    if (trimmed && onSearch) onSearch(trimmed);
  };

  const handleClearSearch = () => {
    setSearchId('');
    setAppliedSearch('');
  };

  const handleSort = (field) => {
    const newOrder = (sortField === field && sortOrder === 'asc') ? 'desc' : 'asc';
    setSortField(field);
    setSortOrder(newOrder);
    if (onSort) onSort(field, newOrder);
  };

  // Derive filtered and sorted packets from authoritative collection
  const displayedPackets = useMemo(() => {
    let list = [...history];

    // 1. Search Filter (exact packet ID matching with case-insensitivity)
    if (appliedSearch) {
      const q = appliedSearch.toUpperCase();
      list = list.filter(p => {
        const id = (p.id || p.packetId || '').toUpperCase();
        return id === q;
      });
    }

    // 2. Sorting
    list.sort((a, b) => {
      let cmp = 0;
      switch (sortField) {
        case 'id': {
          const numA = parseInt((a.id || a.packetId || '').replace(/\D/g, ''), 10) || 0;
          const numB = parseInt((b.id || b.packetId || '').replace(/\D/g, ''), 10) || 0;
          cmp = numA - numB;
          if (cmp === 0) cmp = (a.id || '').localeCompare(b.id || '');
          break;
        }
        case 'priority': {
          const prioRank = { HIGH: 3, MEDIUM: 2, LOW: 1 };
          const pA = prioRank[a.priority] || 0;
          const pB = prioRank[b.priority] || 0;
          cmp = pA - pB;
          break;
        }
        case 'delay': {
          const dA = typeof a.delay === 'number' ? a.delay : 0;
          const dB = typeof b.delay === 'number' ? b.delay : 0;
          cmp = dA - dB;
          break;
        }
        case 'size': {
          const sA = typeof a.size === 'number' ? a.size : 0;
          const sB = typeof b.size === 'number' ? b.size : 0;
          cmp = sA - sB;
          break;
        }
        case 'timestamp': {
          const tA = a.timestamp || a.generationTime || 0;
          const tB = b.timestamp || b.generationTime || 0;
          cmp = tA - tB;
          break;
        }
        case 'status': {
          const statusRank = {
            Created: 1,
            Queued: 2,
            'In Transit': 3,
            Rerouted: 4,
            Delivered: 5,
            Dropped: 6
          };
          const rA = statusRank[a.status] || 99;
          const rB = statusRank[b.status] || 99;
          cmp = rA - rB;
          break;
        }
        default:
          cmp = 0;
      }

      // Tie-breaker for stability (ID natural order)
      if (cmp === 0) {
        const numA = parseInt((a.id || a.packetId || '').replace(/\D/g, ''), 10) || 0;
        const numB = parseInt((b.id || b.packetId || '').replace(/\D/g, ''), 10) || 0;
        cmp = numA - numB;
      }

      return sortOrder === 'asc' ? cmp : -cmp;
    });

    return list;
  }, [history, appliedSearch, sortField, sortOrder]);

  return (
    <div style={{ width: '100%', color: 'var(--text-main)' }}>
      {/* Search & Sort Controls Header */}
      <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '15px', flexWrap: 'wrap', gap: '10px', alignItems: 'center' }}>
        <h3 style={{ color: 'var(--text-muted)', fontSize: '0.9rem', letterSpacing: '1px', textTransform: 'uppercase', margin: 0 }}>
          {searchMode ? 'Search & Sort' : 'Packet History'} ({displayedPackets.length} of {history.length})
        </h3>
        <div style={{ display: 'flex', gap: '8px', alignItems: 'center' }}>
          <input
            type="text"
            placeholder="Search Packet ID (e.g. P101)"
            value={searchId}
            onChange={e => {
              setSearchId(e.target.value);
              if (!e.target.value.trim() && appliedSearch) {
                setAppliedSearch('');
              }
            }}
            onKeyDown={e => e.key === 'Enter' && handleSearch()}
            style={{
              padding: '6px 12px', borderRadius: '6px', border: '1px solid var(--border-color)',
              background: 'var(--bg-color)', color: 'white', fontSize: '0.85rem', width: '220px'
            }}
          />
          <button onClick={handleSearch} style={{
            padding: '6px 14px', borderRadius: '6px', border: '1px solid var(--accent-cyan)',
            background: 'rgba(6, 182, 212, 0.2)', color: 'var(--accent-cyan)', cursor: 'pointer', fontWeight: 600, fontSize: '0.8rem'
          }}>SEARCH</button>
          {appliedSearch && (
            <button onClick={handleClearSearch} style={{
              padding: '6px 12px', borderRadius: '6px', border: '1px solid var(--border-color)',
              background: 'rgba(255, 255, 255, 0.08)', color: 'var(--text-muted)', cursor: 'pointer', fontWeight: 500, fontSize: '0.8rem'
            }}>CLEAR</button>
          )}
        </div>
      </div>

      {/* Sort Buttons Bar */}
      <div style={{ display: 'flex', gap: '8px', marginBottom: '15px', flexWrap: 'wrap', alignItems: 'center' }}>
        <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem', fontWeight: 500 }}>Sort By:</span>
        {['id', 'priority', 'delay', 'size', 'timestamp', 'status'].map(f => {
          const isActive = sortField === f;
          return (
            <button key={f} onClick={() => handleSort(f)} style={{
              padding: '4px 10px', borderRadius: '4px', fontSize: '0.75rem', cursor: 'pointer', fontWeight: 600,
              border: isActive ? '1px solid var(--accent-cyan)' : '1px solid var(--border-color)',
              background: isActive ? 'rgba(6, 182, 212, 0.2)' : 'transparent',
              color: isActive ? 'var(--accent-cyan)' : 'var(--text-muted)',
              textTransform: 'capitalize', display: 'flex', alignItems: 'center', gap: '4px'
            }}>
              {f}
              {isActive && (
                <span style={{ fontSize: '0.8rem', fontWeight: 'bold' }}>
                  {sortOrder === 'asc' ? '↑' : '↓'}
                </span>
              )}
            </button>
          );
        })}
      </div>

      {/* Table */}
      <div style={{ overflowX: 'auto' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left' }}>
          <thead>
            <tr style={{ borderBottom: '1px solid var(--border-color)' }}>
              {[
                { key: 'id', label: 'ID' },
                { key: null, label: 'Source' },
                { key: null, label: 'Dest' },
                { key: 'priority', label: 'Priority' },
                { key: null, label: 'Route' },
                { key: 'status', label: 'Status' },
                { key: 'delay', label: 'Delay' },
              ].map((col, idx) => (
                <th
                  key={idx}
                  onClick={() => col.key && handleSort(col.key)}
                  style={{
                    padding: '10px', color: sortField === col.key ? 'var(--accent-cyan)' : 'var(--text-muted)',
                    fontSize: '0.8rem', fontWeight: 600, textTransform: 'uppercase', letterSpacing: '0.5px',
                    cursor: col.key ? 'pointer' : 'default', userSelect: 'none'
                  }}
                >
                  {col.label} {col.key && sortField === col.key ? (sortOrder === 'asc' ? '↑' : '↓') : ''}
                </th>
              ))}
            </tr>
          </thead>
          <tbody>
            {history.length === 0 ? (
              <tr><td colSpan={7} style={{ padding: '30px', textAlign: 'center', color: 'var(--text-muted)' }}>No packets yet. Generate some packets to see history.</td></tr>
            ) : displayedPackets.length === 0 ? (
              <tr>
                <td colSpan={7} style={{ padding: '30px', textAlign: 'center', color: '#ef4444', fontWeight: 600, fontSize: '0.9rem' }}>
                  Packet not found
                  <div style={{ marginTop: '6px', fontSize: '0.8rem', color: 'var(--text-muted)', fontWeight: 400 }}>
                    No record matches ID "{appliedSearch}". <button onClick={handleClearSearch} style={{ background: 'none', border: 'none', color: 'var(--accent-cyan)', cursor: 'pointer', textDecoration: 'underline' }}>View all packets</button>
                  </div>
                </td>
              </tr>
            ) : (
              displayedPackets.map((p, i) => (
                <tr key={p.id || i}
                  onClick={() => onSelectPacket && onSelectPacket(p)}
                  style={{ borderBottom: '1px solid rgba(255,255,255,0.04)', cursor: 'pointer', transition: 'background 0.2s' }}
                  onMouseEnter={e => e.currentTarget.style.background = 'rgba(6, 182, 212, 0.05)'}
                  onMouseLeave={e => e.currentTarget.style.background = 'transparent'}
                >
                  <td style={{ padding: '10px', fontWeight: 'bold', fontSize: '0.85rem' }}>{p.id}</td>
                  <td style={{ padding: '10px', fontSize: '0.85rem' }}>{p.source}</td>
                  <td style={{ padding: '10px', fontSize: '0.85rem' }}>{p.destination}</td>
                  <td style={{ padding: '10px' }}>
                    <span style={{ color: PRIORITY_COLOR[p.priority] || 'var(--text-muted)', fontWeight: 600, fontSize: '0.8rem' }}>{p.priority}</span>
                  </td>
                  <td style={{ padding: '10px', fontFamily: 'monospace', color: 'var(--accent-cyan)', fontSize: '0.8rem' }}>{p.route || '—'}</td>
                  <td style={{ padding: '10px' }}>
                    <span style={{ color: STATUS_COLOR[p.status] || 'var(--text-muted)', fontWeight: 600, fontSize: '0.8rem' }}>{p.status}</span>
                  </td>
                  <td style={{ padding: '10px', fontSize: '0.85rem' }}>{p.delay > 0 ? `${p.delay}ms` : '—'}</td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
};

export default PacketHistory;
