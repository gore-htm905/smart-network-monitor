import React from 'react';

const SEVERITY_STYLES = {
  info:     { icon: '✓', bg: 'rgba(16, 185, 129, 0.1)', border: '#10b981', color: '#10b981' },
  warning:  { icon: '⚠', bg: 'rgba(245, 158, 11, 0.1)', border: '#f59e0b', color: '#f59e0b' },
  critical: { icon: '🔴', bg: 'rgba(239, 68, 68, 0.1)', border: '#ef4444', color: '#ef4444' },
};

const Alerts = ({ alerts = [] }) => {
  const reversed = [...alerts].reverse(); // newest first

  return (
    <div style={{ color: 'var(--text-main)' }}>
      <h3 style={{ color: 'var(--text-muted)', fontSize: '0.9rem', letterSpacing: '1px', textTransform: 'uppercase', marginBottom: '15px' }}>
        Alerts ({alerts.length})
      </h3>
      <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', maxHeight: '350px', overflowY: 'auto' }}>
        {reversed.length === 0 && <span style={{ color: 'var(--text-muted)' }}>No alerts.</span>}
        {reversed.map((a, i) => {
          const style = SEVERITY_STYLES[a.severity] || SEVERITY_STYLES.info;
          return (
            <div key={i} style={{
              display: 'flex', alignItems: 'center', gap: '10px',
              padding: '8px 12px', borderRadius: '6px',
              backgroundColor: style.bg, borderLeft: `3px solid ${style.border}`,
              fontSize: '0.82rem'
            }}>
              <span>{style.icon}</span>
              <span style={{ color: style.color, flex: 1 }}>{a.message}</span>
            </div>
          );
        })}
      </div>
    </div>
  );
};

export default Alerts;
