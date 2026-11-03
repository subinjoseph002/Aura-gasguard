#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

// =====================================================================================
// ESP32 Gas Sentinel - Ultra Premium Responsive Real-Time Web Dashboard
// Embedded HTML5/CSS3/JavaScript stored in PROGMEM flash memory
// =====================================================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Aura GasGuard™ • ESP32 IoT Gas Leakage Sentinel</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500;600;700&family=Plus+Jakarta+Sans:wght@300;400;500;600;700;800&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-base: #060913;
      --bg-surface: rgba(15, 23, 42, 0.75);
      --bg-surface-elevated: rgba(30, 41, 59, 0.85);
      --border-subtle: rgba(255, 255, 255, 0.08);
      --border-focus: rgba(99, 102, 241, 0.5);
      
      --color-safe: #10b981;
      --color-safe-glow: rgba(16, 185, 129, 0.25);
      --color-warn: #f59e0b;
      --color-warn-glow: rgba(245, 158, 11, 0.35);
      --color-danger: #ef4444;
      --color-danger-glow: rgba(239, 68, 68, 0.45);
      --color-cyan: #06b6d4;
      --color-indigo: #6366f1;

      --text-primary: #f8fafc;
      --text-secondary: #94a3b8;
      --text-muted: #64748b;
      
      --font-main: 'Plus Jakarta Sans', -apple-system, BlinkMacSystemFont, sans-serif;
      --font-mono: 'JetBrains Mono', monospace;
      --radius-sm: 8px;
      --radius-md: 14px;
      --radius-lg: 20px;
      --radius-full: 9999px;
    }

    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      -webkit-tap-highlight-color: transparent;
    }

    body {
      background-color: var(--bg-base);
      background-image: 
        radial-gradient(circle at 50% 0%, rgba(99, 102, 241, 0.12) 0%, transparent 60%),
        radial-gradient(circle at 100% 100%, rgba(6, 182, 212, 0.08) 0%, transparent 50%),
        radial-gradient(circle at 0% 100%, rgba(239, 68, 68, 0.05) 0%, transparent 50%);
      background-attachment: fixed;
      color: var(--text-primary);
      font-family: var(--font-main);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 1.25rem;
      overflow-x: hidden;
    }

    .dashboard-wrapper {
      width: 100%;
      max-width: 1140px;
      display: flex;
      flex-direction: column;
      gap: 1.25rem;
    }

    /* Ambient Header */
    header {
      background: var(--bg-surface);
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
      border: 1px solid var(--border-subtle);
      border-radius: var(--radius-lg);
      padding: 1rem 1.5rem;
      display: flex;
      justify-content: space-between;
      align-items: center;
      flex-wrap: wrap;
      gap: 1rem;
      box-shadow: 0 10px 30px -10px rgba(0, 0, 0, 0.5);
    }

    .brand-group {
      display: flex;
      align-items: center;
      gap: 1rem;
    }

    .brand-icon-halo {
      width: 48px;
      height: 48px;
      border-radius: var(--radius-md);
      background: linear-gradient(135deg, rgba(99, 102, 241, 0.2), rgba(6, 182, 212, 0.2));
      border: 1px solid rgba(99, 102, 241, 0.3);
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 1.5rem;
      box-shadow: 0 0 20px rgba(99, 102, 241, 0.25);
    }

    .brand-text h1 {
      font-size: 1.25rem;
      font-weight: 800;
      letter-spacing: -0.02em;
      background: linear-gradient(135deg, #fff 40%, #94a3b8);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }

    .brand-text p {
      font-size: 0.8rem;
      color: var(--text-secondary);
      display: flex;
      align-items: center;
      gap: 0.4rem;
    }

    .header-pills {
      display: flex;
      align-items: center;
      gap: 0.6rem;
      flex-wrap: wrap;
    }

    .pill {
      font-size: 0.75rem;
      font-weight: 600;
      font-family: var(--font-mono);
      padding: 0.35rem 0.85rem;
      border-radius: var(--radius-full);
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid var(--border-subtle);
      display: flex;
      align-items: center;
      gap: 0.45rem;
      transition: all 0.2s ease;
    }

    .pill-live {
      color: var(--color-safe);
      border-color: rgba(16, 185, 129, 0.3);
      background: rgba(16, 185, 129, 0.1);
    }

    .pill-live .dot {
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: var(--color-safe);
      box-shadow: 0 0 10px var(--color-safe);
      animation: pulse-dot 1.8s infinite;
    }

    .pill-warn {
      color: var(--color-warn);
      border-color: rgba(245, 158, 11, 0.3);
      background: rgba(245, 158, 11, 0.1);
    }

    .pill-warn .dot {
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: var(--color-warn);
      box-shadow: 0 0 10px var(--color-warn);
    }

    .pill-danger {
      color: var(--color-danger);
      border-color: rgba(239, 68, 68, 0.3);
      background: rgba(239, 68, 68, 0.1);
    }

    @keyframes pulse-dot {
      0%, 100% { transform: scale(1); opacity: 1; }
      50% { transform: scale(1.4); opacity: 0.5; }
    }

    .toggle-chip {
      cursor: pointer;
      user-select: none;
    }
    .toggle-chip:hover {
      background: rgba(255, 255, 255, 0.08);
      border-color: rgba(255, 255, 255, 0.2);
    }

    .audio-toggle-btn {
      background: none;
      border: 1px solid var(--border-subtle);
      color: var(--text-secondary);
      border-radius: var(--radius-full);
      padding: 0.35rem 0.85rem;
      font-size: 0.75rem;
      font-weight: 600;
      font-family: var(--font-mono);
      cursor: pointer;
      display: flex;
      align-items: center;
      gap: 0.45rem;
      transition: all 0.2s ease;
    }
    .audio-toggle-btn:hover {
      color: var(--text-primary);
      border-color: rgba(255, 255, 255, 0.25);
    }

    /* Primary Hazard Banner */
    .hazard-banner {
      background: var(--bg-surface);
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
      border: 2px solid var(--color-safe);
      border-radius: var(--radius-lg);
      padding: 1.25rem 1.75rem;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 1.25rem;
      box-shadow: 0 0 30px var(--color-safe-glow);
      transition: all 0.4s cubic-bezier(0.16, 1, 0.3, 1);
      position: relative;
      overflow: hidden;
    }

    .hazard-banner::before {
      content: '';
      position: absolute;
      top: 0; left: -100%;
      width: 50%;
      height: 100%;
      background: linear-gradient(90deg, transparent, rgba(255,255,255,0.06), transparent);
      transform: skewX(-25deg);
      animation: shimmer 6s infinite;
    }

    @keyframes shimmer {
      100% { left: 200%; }
    }

    .hazard-banner.safe {
      border-color: var(--color-safe);
      box-shadow: 0 0 35px var(--color-safe-glow);
    }
    .hazard-banner.warning {
      border-color: var(--color-warn);
      box-shadow: 0 0 45px var(--color-warn-glow);
    }
    .hazard-banner.danger {
      border-color: var(--color-danger);
      box-shadow: 0 0 60px var(--color-danger-glow);
      animation: danger-pulse 1.2s infinite alternate;
    }

    @keyframes danger-pulse {
      0% { transform: scale(1); filter: brightness(1); }
      100% { transform: scale(1.008); filter: brightness(1.18); }
    }

    .hazard-info h2 {
      font-size: 1.35rem;
      font-weight: 800;
      letter-spacing: -0.01em;
      display: flex;
      align-items: center;
      gap: 0.6rem;
    }

    .hazard-info p {
      color: var(--text-secondary);
      font-size: 0.9rem;
      margin-top: 0.35rem;
      line-height: 1.4;
    }

    .hazard-badge {
      font-size: 2.75rem;
      line-height: 1;
      filter: drop-shadow(0 0 15px rgba(255, 255, 255, 0.2));
    }

    /* Main Grid Layout */
    .dashboard-grid {
      display: grid;
      grid-template-columns: 1.25fr 0.85fr;
      gap: 1.25rem;
    }

    @media (max-width: 900px) {
      .dashboard-grid {
        grid-template-columns: 1fr;
      }
    }

    /* Glass Cards */
    .card {
      background: var(--bg-surface);
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
      border: 1px solid var(--border-subtle);
      border-radius: var(--radius-lg);
      padding: 1.35rem;
      display: flex;
      flex-direction: column;
      gap: 1.15rem;
      box-shadow: 0 15px 35px -10px rgba(0, 0, 0, 0.4);
      transition: transform 0.2s ease, border-color 0.2s ease;
    }

    .card:hover {
      border-color: rgba(255, 255, 255, 0.14);
    }

    .card-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      flex-wrap: wrap;
      gap: 0.5rem;
    }

    .card-title {
      font-size: 0.82rem;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.08em;
      color: var(--text-secondary);
      display: flex;
      align-items: center;
      gap: 0.5rem;
    }

    /* Radial Circular Dial Gauge */
    .gauge-container {
      display: flex;
      flex-direction: column;
      align-items: center;
      position: relative;
      margin: 0.25rem 0;
    }

    .dial-svg {
      width: 270px;
      height: 165px;
      overflow: visible;
    }

    .dial-bg {
      fill: none;
      stroke: #1e293b;
      stroke-width: 18;
      stroke-linecap: round;
    }

    .dial-fill {
      fill: none;
      stroke: var(--color-safe);
      stroke-width: 18;
      stroke-linecap: round;
      stroke-dasharray: 235.6;
      stroke-dashoffset: 235.6;
      transition: stroke-dashoffset 0.5s cubic-bezier(0.16, 1, 0.3, 1), stroke 0.4s ease;
      filter: drop-shadow(0 0 12px var(--color-safe-glow));
    }

    .dial-readout {
      position: absolute;
      top: 68px;
      left: 50%;
      transform: translateX(-50%);
      text-align: center;
      pointer-events: none;
    }

    .dial-value {
      font-size: 3.25rem;
      font-weight: 800;
      letter-spacing: -0.04em;
      font-family: var(--font-mono);
      line-height: 1;
    }

    .dial-unit {
      font-size: 0.85rem;
      font-weight: 600;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 0.05em;
      margin-top: 0.25rem;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 0.35rem;
    }

    .dial-stats-row {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 0.75rem;
      width: 100%;
      padding-top: 1rem;
      border-top: 1px solid var(--border-subtle);
    }

    .stat-item {
      text-align: center;
      background: rgba(255, 255, 255, 0.02);
      padding: 0.5rem;
      border-radius: var(--radius-sm);
      border: 1px solid var(--border-subtle);
    }
    .stat-label {
      font-size: 0.7rem;
      color: var(--text-muted);
      text-transform: uppercase;
      font-weight: 600;
      letter-spacing: 0.05em;
    }
    .stat-num {
      font-size: 1.15rem;
      font-weight: 700;
      font-family: var(--font-mono);
      color: var(--text-primary);
      margin-top: 0.15rem;
    }

    /* Gas Calibrated Selector */
    .gas-types-bar {
      display: flex;
      gap: 0.4rem;
      overflow-x: auto;
      padding: 0.2rem 0;
    }

    .gas-tag {
      font-size: 0.7rem;
      font-weight: 600;
      padding: 0.25rem 0.6rem;
      border-radius: var(--radius-full);
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid var(--border-subtle);
      color: var(--text-secondary);
      cursor: pointer;
      white-space: nowrap;
      transition: all 0.2s ease;
    }

    .gas-tag.active {
      background: rgba(99, 102, 241, 0.2);
      border-color: var(--color-indigo);
      color: #fff;
    }

    /* Live Rolling Sparkline Chart */
    .chart-box {
      width: 100%;
      height: 125px;
      position: relative;
      background: rgba(0, 0, 0, 0.35);
      border-radius: var(--radius-sm);
      border: 1px solid var(--border-subtle);
      overflow: hidden;
      margin-top: 0.35rem;
    }

    canvas#trend-chart {
      width: 100%;
      height: 100%;
      display: block;
    }

    /* Controls & Actuators Grid */
    .actuators-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0.85rem;
    }

    @media (max-width: 540px) {
      .actuators-grid {
        grid-template-columns: 1fr;
      }
    }

    .actuator-card {
      background: var(--bg-surface-elevated);
      border: 1px solid var(--border-subtle);
      border-radius: var(--radius-md);
      padding: 1rem;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      gap: 0.75rem;
      transition: all 0.2s ease;
    }

    .actuator-top {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
    }

    .actuator-name {
      font-size: 0.85rem;
      font-weight: 700;
      color: var(--text-primary);
    }
    .actuator-sub {
      font-size: 0.72rem;
      color: var(--text-muted);
    }

    .actuator-state-badge {
      font-size: 0.68rem;
      font-weight: 700;
      font-family: var(--font-mono);
      padding: 0.2rem 0.5rem;
      border-radius: var(--radius-full);
      background: rgba(255, 255, 255, 0.05);
      color: var(--text-muted);
      border: 1px solid var(--border-subtle);
    }

    .actuator-state-badge.active {
      background: rgba(16, 185, 129, 0.15);
      color: var(--color-safe);
      border-color: rgba(16, 185, 129, 0.3);
    }

    .actuator-state-badge.danger {
      background: rgba(239, 68, 68, 0.15);
      color: var(--color-danger);
      border-color: rgba(239, 68, 68, 0.3);
      animation: pulse-dot 1s infinite;
    }

    .actuator-btn {
      width: 100%;
      background: linear-gradient(135deg, #3b82f6, #2563eb);
      color: white;
      border: none;
      padding: 0.65rem 1rem;
      border-radius: var(--radius-sm);
      font-size: 0.8rem;
      font-weight: 600;
      font-family: var(--font-main);
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 0.45rem;
      transition: all 0.2s ease;
      box-shadow: 0 4px 12px rgba(37, 99, 235, 0.3);
    }

    .actuator-btn:hover {
      transform: translateY(-1px);
      box-shadow: 0 6px 16px rgba(37, 99, 235, 0.4);
    }

    .actuator-btn:active {
      transform: translateY(1px);
    }

    .actuator-btn.btn-mute {
      background: #334155;
      box-shadow: none;
    }
    .actuator-btn.btn-mute:hover {
      background: #475569;
    }

    .actuator-btn.btn-alert {
      background: linear-gradient(135deg, #ef4444, #dc2626);
      box-shadow: 0 4px 12px rgba(239, 68, 68, 0.3);
    }
    .actuator-btn.btn-alert:hover {
      box-shadow: 0 6px 16px rgba(239, 68, 68, 0.4);
    }

    /* Fan spinning animation */
    .icon-fan {
      display: inline-block;
      transition: transform 0.2s linear;
    }
    .icon-fan.spinning {
      animation: spin 0.6s linear infinite;
    }
    @keyframes spin {
      100% { transform: rotate(360deg); }
    }

    /* Slider Card */
    .slider-box {
      display: flex;
      flex-direction: column;
      gap: 0.65rem;
    }

    .slider-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .slider-val-tag {
      font-family: var(--font-mono);
      font-weight: 700;
      color: var(--color-cyan);
      font-size: 1.05rem;
    }

    input[type=range] {
      -webkit-appearance: none;
      width: 100%;
      height: 8px;
      border-radius: var(--radius-full);
      background: #1e293b;
      outline: none;
      cursor: pointer;
    }

    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--color-cyan);
      box-shadow: 0 0 12px var(--color-cyan);
      cursor: pointer;
      transition: transform 0.15s ease;
    }

    input[type=range]::-webkit-slider-thumb:hover {
      transform: scale(1.15);
    }

    /* Safety Checklist Card (Visible in Hazard state) */
    .safety-actions {
      display: none;
      background: rgba(239, 68, 68, 0.08);
      border: 1px dashed rgba(239, 68, 68, 0.4);
      border-radius: var(--radius-md);
      padding: 1rem;
      margin-top: 0.5rem;
    }

    .safety-actions.active {
      display: block;
      animation: fadeIn 0.4s ease;
    }

    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(5px); }
      to { opacity: 1; transform: translateY(0); }
    }

    .safety-title {
      font-size: 0.82rem;
      font-weight: 700;
      color: var(--color-danger);
      margin-bottom: 0.5rem;
      text-transform: uppercase;
      letter-spacing: 0.05em;
      display: flex;
      align-items: center;
      gap: 0.4rem;
    }

    .safety-list {
      list-style: none;
      display: flex;
      flex-direction: column;
      gap: 0.35rem;
      font-size: 0.8rem;
      color: var(--text-primary);
    }

    .safety-list li {
      display: flex;
      align-items: center;
      gap: 0.5rem;
    }

    /* Simulation Control Drawer */
    .sim-bar {
      background: rgba(6, 182, 212, 0.05);
      border: 1px solid rgba(6, 182, 212, 0.2);
      border-radius: var(--radius-md);
      padding: 0.85rem;
      display: flex;
      flex-direction: column;
      gap: 0.75rem;
    }

    .sim-bar-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 0.75rem;
      font-weight: 700;
      color: var(--color-cyan);
      text-transform: uppercase;
      letter-spacing: 0.06em;
    }

    .sim-buttons {
      display: flex;
      gap: 0.5rem;
      flex-wrap: wrap;
    }

    .sim-btn {
      flex: 1;
      min-width: 100px;
      padding: 0.45rem 0.75rem;
      font-size: 0.72rem;
      font-weight: 600;
      font-family: var(--font-mono);
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid var(--border-subtle);
      border-radius: var(--radius-sm);
      color: var(--text-primary);
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 0.35rem;
      transition: all 0.2s ease;
    }

    .sim-btn:hover {
      background: rgba(255, 255, 255, 0.1);
      border-color: rgba(255, 255, 255, 0.2);
    }

    .sim-btn.leak {
      color: var(--color-danger);
      border-color: rgba(239, 68, 68, 0.3);
      background: rgba(239, 68, 68, 0.1);
    }
    .sim-btn.leak:hover {
      background: rgba(239, 68, 68, 0.2);
    }

    .sim-btn.clear {
      color: var(--color-safe);
      border-color: rgba(16, 185, 129, 0.3);
      background: rgba(16, 185, 129, 0.1);
    }
    .sim-btn.clear:hover {
      background: rgba(16, 185, 129, 0.2);
    }

    /* Event Log Table */
    .events-box {
      max-height: 130px;
      overflow-y: auto;
      font-family: var(--font-mono);
      font-size: 0.72rem;
      display: flex;
      flex-direction: column;
      gap: 0.3rem;
      padding-right: 0.25rem;
    }

    .event-row {
      display: flex;
      justify-content: space-between;
      padding: 0.25rem 0.4rem;
      background: rgba(255, 255, 255, 0.02);
      border-radius: 4px;
      border-left: 3px solid var(--border-subtle);
    }

    .event-row.alarm {
      border-left-color: var(--color-danger);
      background: rgba(239, 68, 68, 0.05);
    }
    .event-row.safe {
      border-left-color: var(--color-safe);
    }
    .event-row.info {
      border-left-color: var(--color-cyan);
    }

    /* Telemetry Info Footer */
    footer {
      display: flex;
      justify-content: space-between;
      align-items: center;
      flex-wrap: wrap;
      gap: 0.75rem;
      padding: 0.85rem 1.5rem;
      background: var(--bg-surface);
      border: 1px solid var(--border-subtle);
      border-radius: var(--radius-lg);
      font-size: 0.75rem;
      color: var(--text-muted);
      font-family: var(--font-mono);
    }

    .footer-left {
      display: flex;
      align-items: center;
      gap: 0.75rem;
    }

    /* Custom Scrollbar */
    ::-webkit-scrollbar {
      width: 5px;
      height: 5px;
    }
    ::-webkit-scrollbar-track {
      background: rgba(0, 0, 0, 0.2);
    }
    ::-webkit-scrollbar-thumb {
      background: rgba(255, 255, 255, 0.15);
      border-radius: 4px;
    }
    ::-webkit-scrollbar-thumb:hover {
      background: rgba(255, 255, 255, 0.3);
    }
  </style>
</head>
<body>
  <div class="dashboard-wrapper">
    
    <!-- Top Ambient Navigation Header -->
    <header>
      <div class="brand-group">
        <div class="brand-icon-halo">🛡ï¸</div>
        <div class="brand-text">
          <h1>AURA GASGUARD™</h1>
          <p>ESP32 30-Pin Sentinel • Real-Time Safety Matrix</p>
        </div>
      </div>
      <div class="header-pills">
        <div class="pill pill-live" id="network-pill">
          <span class="dot"></span>
          <span id="network-label">CHECKING HARDWARE...</span>
        </div>
        <div class="pill toggle-chip" id="demo-chip" onclick="toggleDemoMode()" title="Toggle simulated live gas drift">
          <span>🎮 Demo Simulation</span>
        </div>
        <button class="audio-toggle-btn" id="audio-btn" onclick="toggleBrowserAudio()" title="Sound audible siren through browser speakers">
          <span id="audio-icon">🔇</span> Web Siren: Off
        </button>
      </div>
    </header>

    <!-- Emergency Dynamic Status Banner -->
    <div class="hazard-banner safe" id="banner">
      <div class="hazard-info">
        <h2 id="banner-title">🟢 STATUS: AIR QUALITY NORMAL</h2>
        <p id="banner-desc">Atmospheric gas concentration is within clean room baseline. No flammable hydrocarbons or combustion smoke detected.</p>
      </div>
      <div class="hazard-badge" id="banner-emoji">✅</div>
    </div>

    <!-- Main Dynamic Content Grid -->
    <div class="dashboard-grid">
      
      <!-- Primary Gas Sensor Column -->
      <div class="card">
        <div class="card-header">
          <div class="card-title">🔥 Gas Concentration Matrix (MQ-2)</div>
          <span class="pill" id="sensor-mode-pill">GPIO 34 • ADC1</span>
        </div>

        <!-- Gas Type Calibration Switcher -->
        <div class="gas-types-bar">
          <div class="gas-tag active" onclick="setGasType('LPG', this)">LPG / Butane</div>
          <div class="gas-tag" onclick="setGasType('METHANE', this)">Methane / CNG</div>
          <div class="gas-tag" onclick="setGasType('SMOKE', this)">Smoke / CO</div>
          <div class="gas-tag" onclick="setGasType('PROPANE', this)">Propane</div>
          <div class="gas-tag" onclick="setGasType('HYDROGEN', this)">Hydrogen (H2)</div>
        </div>

        <!-- Radial Arc Gauge -->
        <div class="gauge-container">
          <svg class="dial-svg" viewBox="0 0 200 125">
            <!-- Background Arc: Radius 75, Center (100, 100), Angles 180 to 0 deg -->
            <path class="dial-bg" d="M 25 100 A 75 75 0 0 1 175 100" />
            <!-- Fill Arc -->
            <path class="dial-fill" id="dial-path" d="M 25 100 A 75 75 0 0 1 175 100" stroke-dasharray="235.6" stroke-dashoffset="235.6" />
          </svg>
          <div class="dial-readout">
            <div class="dial-value" id="dial-percentage">0.0%</div>
            <div class="dial-unit" id="gas-level-label">Atmosphere Clear</div>
          </div>
        </div>

        <!-- Metrics Row -->
        <div class="dial-stats-row">
          <div class="stat-item">
            <div class="stat-label">Raw ADC (12-bit)</div>
            <div class="stat-num" id="stat-raw">0</div>
          </div>
          <div class="stat-item">
            <div class="stat-label">Alarm Threshold</div>
            <div class="stat-num" id="stat-threshold">10%</div>
          </div>
          <div class="stat-item">
            <div class="stat-label">Estimated PPM</div>
            <div class="stat-num" id="stat-ppm">~15 ppm</div>
          </div>
        </div>

        <!-- Real-Time Rolling History Graph -->
        <div class="slider-box" style="margin-top: 0.25rem;">
          <div class="slider-header">
            <div class="card-title" style="font-size: 0.75rem;">📈 60-Second Real-Time Trend Wave</div>
            <span style="font-size: 0.72rem; color: var(--text-muted); font-family: var(--font-mono);" id="trend-status">Live 1Hz</span>
          </div>
          <div class="chart-box">
            <canvas id="trend-chart"></canvas>
          </div>
        </div>

        <!-- Safety Checklist (Revealed during alarm) -->
        <div class="safety-actions" id="safety-box">
          <div class="safety-title">⚠ï¸ Mandatory Safety Protocols:</div>
          <ul class="safety-list">
            <li>🚨 1. Automated exhaust ventilation active • Do NOT touch light switches or sparks</li>
            <li>🔥 2. Immediately close main LPG cylinder manual shutoff valve</li>
            <li>💨 3. Open exterior windows and doors for rapid natural cross-ventilation</li>
            <li>🏃 4. Evacuate building immediately and call Emergency Fire Services (101 / 911)</li>
          </ul>
        </div>
      </div>

      <!-- Controls, Actuators & Telemetry Column -->
      <div style="display: flex; flex-direction: column; gap: 1.25rem;">
        
        <!-- Actuators Card -->
        <div class="card">
          <div class="card-title">⚡ Hardware Actuators & Relays</div>
          
          <div class="actuators-grid">
            <!-- Exhaust Fan / Solenoid Relay -->
            <div class="actuator-card">
              <div class="actuator-top">
                <div>
                  <div class="actuator-name">Exhaust Fan Relay</div>
                  <div class="actuator-sub">GPIO 14 • 5V Coil</div>
                </div>
                <div class="actuator-state-badge" id="fan-badge">STANDBY</div>
              </div>
              <button class="actuator-btn" id="fan-btn" onclick="toggleFan()">
                <span class="icon-fan" id="fan-icon">🌀</span>
                <span id="fan-btn-text">Turn Fan ON</span>
              </button>
            </div>

            <!-- Siren Buzzer -->
            <div class="actuator-card">
              <div class="actuator-top">
                <div>
                  <div class="actuator-name">Acoustic Siren</div>
                  <div class="actuator-sub">GPIO 25 • Intermittent</div>
                </div>
                <div class="actuator-state-badge" id="buzzer-badge">MUTED</div>
              </div>
              <button class="actuator-btn btn-mute" id="mute-btn" onclick="toggleMute()">
                <span id="mute-icon">🔇</span>
                <span id="mute-btn-text">Mute Buzzer</span>
              </button>
            </div>
          </div>

          <!-- Alarm Setpoint Adjuster -->
          <div class="slider-box" style="margin-top: 0.25rem;">
            <div class="slider-header">
              <span class="card-title">🎯 Trigger Setpoint Level</span>
              <span class="slider-val-tag" id="slider-display">40%</span>
            </div>
            <input type="range" id="threshold-slider" min="5" max="90" value="10" oninput="onSliderMove(this.value)" onchange="sendThreshold(this.value)">
            <p style="font-size: 0.72rem; color: var(--text-muted);">Hysteresis deadband: Automatically silences 3% below trigger point to eliminate flutter.</p>
          </div>

          <!-- Diagnostic Test Alarm -->
          <button class="actuator-btn btn-alert" onclick="triggerTestAlarm()">
            🚨 Trigger 4-Second Emergency System Drill
          </button>
        </div>

        <!-- Interactive Simulation Drawer (for Demos & Testing) -->
        <div class="sim-bar" id="sim-controls">
          <div class="sim-bar-header">
            <span>🛠ï¸ Interactive Gas Simulator</span>
            <span style="font-size: 0.68rem; color: var(--text-muted);" id="sim-state-text">Active for Presentation</span>
          </div>
          <div class="sim-buttons">
            <button class="sim-btn leak" onclick="simTriggerLeak()">💥 Simulate Gas Leak</button>
            <button class="sim-btn clear" onclick="simClearAtmosphere()">🍃 Clear Air</button>
            <button class="sim-btn" onclick="simStepGas(10)">➕ Gas +10%</button>
            <button class="sim-btn" onclick="simStepGas(-10)">➖ Gas -10%</button>
          </div>
        </div>

        <!-- Telemetry & Event Log Card -->
        <div class="card">
          <div class="card-header">
            <div class="card-title">📡 Live System Telemetry</div>
            <button class="gas-tag" onclick="exportEventLog()" style="padding: 0.2rem 0.5rem; font-size: 0.65rem;">💾 Export CSV</button>
          </div>

          <div style="display: flex; flex-direction: column; gap: 0.45rem; font-family: var(--font-mono); font-size: 0.76rem;">
            <div style="display: flex; justify-content: space-between; border-bottom: 1px solid var(--border-subtle); padding-bottom: 0.25rem;">
              <span style="color: var(--text-muted);">Uptime Counter:</span>
              <span id="telem-uptime">00:00:00</span>
            </div>
            <div style="display: flex; justify-content: space-between; border-bottom: 1px solid var(--border-subtle); padding-bottom: 0.25rem;">
              <span style="color: var(--text-muted);">MCU Processor:</span>
              <span>ESP32 240MHz Xtensa LX6</span>
            </div>
            <div style="display: flex; justify-content: space-between; border-bottom: 1px solid var(--border-subtle); padding-bottom: 0.25rem;">
              <span style="color: var(--text-muted);">Wi-Fi RSSI Signal:</span>
              <span id="telem-wifi" style="color: var(--color-safe);">-52 dBm (Excellent)</span>
            </div>
            <div style="display: flex; justify-content: space-between;">
              <span style="color: var(--text-muted);">Sensor Analog Filter:</span>
              <span style="color: var(--color-cyan);">10-Sample Moving Avg</span>
            </div>
          </div>

          <div style="margin-top: 0.4rem;">
            <div class="card-title" style="font-size: 0.7rem; margin-bottom: 0.35rem;">📜 Event Timeline Log</div>
            <div class="events-box" id="events-log">
              <!-- Dynamically populated event rows -->
            </div>
          </div>
        </div>

      </div>
    </div>

    <!-- Telemetry System Footer -->
    <footer>
      <div class="footer-left">
        <span>⚡ ESP32 GasGuard Firmware v2.5</span>
        <span>•</span>
        <span>ADC1 Continuous Sampling</span>
      </div>
      <div>
        <span>Embedded Web Server Running on FreeRTOS Core 1</span>
      </div>
    </footer>

  </div>

  <script>
    // System Global State
    let isDemoMode = false; // Auto-detects on first fetch
    let isWebAudioEnabled = false;
    let audioCtx = null;
    let isMuted = false;
    let currentThreshold = 40;
    let selectedGasType = 'LPG';
    let historyPoints = new Array(60).fill(12);
    let eventLog = [];
    let consecutiveFetchErrors = 0;

    // Gas Curve PPM Constants for MQ-2
    const GAS_CURVES = {
      'LPG':      { mult: 1200, base: 15, exp: 1.15 },
      'METHANE':  { mult: 2500, base: 50, exp: 1.25 },
      'SMOKE':    { mult: 800,  base: 20, exp: 1.05 },
      'PROPANE':  { mult: 1400, base: 18, exp: 1.18 },
      'HYDROGEN': { mult: 1800, base: 30, exp: 1.22 }
    };

    function setGasType(type, el) {
      selectedGasType = type;
      document.querySelectorAll('.gas-tag').forEach(t => t.classList.remove('active'));
      el.classList.add('active');
      logEvent('Gas calibration set to ' + type, 'info');
      if (lastData) applySystemData(lastData);
    }

    // Logger
    function logEvent(text, type = 'info') {
      const now = new Date();
      const timeStr = now.toTimeString().split(' ')[0];
      const item = { time: timeStr, text, type };
      eventLog.unshift(item);
      if (eventLog.length > 50) eventLog.pop();

      const container = document.getElementById('events-log');
      if (!container) return;
      const row = document.createElement('div');
      row.className = 'event-row ' + type;
      row.innerHTML = `<span style="color: var(--text-muted);">${timeStr}</span> <span>${text}</span>`;
      container.insertBefore(row, container.firstChild);
      if (container.children.length > 30) {
        container.removeChild(container.lastChild);
      }
    }

    function exportEventLog() {
      if (eventLog.length === 0) {
        alert('Event log is currently empty.');
        return;
      }
      let csv = 'Timestamp,Event,Type\n';
      eventLog.forEach(e => {
        csv += `"${e.time}","${e.text.replace(/"/g, '""')}","${e.type}"\n`;
      });
      const blob = new Blob([csv], { type: 'text/csv' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `GasGuard_Log_${Date.now()}.csv`;
      a.click();
    }

    // Initialize Canvas Sparkline
    const canvas = document.getElementById('trend-chart');
    const ctx = canvas.getContext('2d');
    function resizeCanvas() {
      canvas.width = canvas.parentElement.clientWidth * (window.devicePixelRatio || 1);
      canvas.height = canvas.parentElement.clientHeight * (window.devicePixelRatio || 1);
      ctx.scale(window.devicePixelRatio || 1, window.devicePixelRatio || 1);
    }
    window.addEventListener('resize', resizeCanvas);
    resizeCanvas();

    function renderTrendChart() {
      const dpr = window.devicePixelRatio || 1;
      const w = canvas.width / dpr;
      const h = canvas.height / dpr;
      ctx.clearRect(0, 0, w, h);

      // Grid line for threshold
      const threshY = h - (currentThreshold / 100 * h);
      ctx.strokeStyle = 'rgba(239, 68, 68, 0.4)';
      ctx.setLineDash([4, 4]);
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(0, threshY);
      ctx.lineTo(w, threshY);
      ctx.stroke();
      ctx.setLineDash([]);

      // Label threshold
      ctx.fillStyle = 'rgba(239, 68, 68, 0.7)';
      ctx.font = '10px JetBrains Mono';
      ctx.fillText('LIMIT ' + currentThreshold + '%', 8, Math.max(12, threshY - 4));

      // Draw Wave Path
      const step = w / (historyPoints.length - 1);
      ctx.beginPath();
      ctx.moveTo(0, h - (historyPoints[0] / 100 * h));

      for (let i = 1; i < historyPoints.length; i++) {
        const x = i * step;
        const y = h - (historyPoints[i] / 100 * h);
        ctx.lineTo(x, y);
      }

      // Stroke
      ctx.lineWidth = 2.5;
      const latestVal = historyPoints[historyPoints.length - 1];
      ctx.strokeStyle = latestVal >= currentThreshold ? '#ef4444' : (latestVal >= currentThreshold * 0.75 ? '#f59e0b' : '#10b981');
      ctx.stroke();

      // Area Fill
      ctx.lineTo(w, h);
      ctx.lineTo(0, h);
      ctx.closePath();
      const grad = ctx.createLinearGradient(0, 0, 0, h);
      if (latestVal >= currentThreshold) {
        grad.addColorStop(0, 'rgba(239, 68, 68, 0.35)');
      } else if (latestVal >= currentThreshold * 0.75) {
        grad.addColorStop(0, 'rgba(245, 158, 11, 0.28)');
      } else {
        grad.addColorStop(0, 'rgba(16, 185, 129, 0.22)');
      }
      grad.addColorStop(1, 'rgba(16, 185, 129, 0.0)');
      ctx.fillStyle = grad;
      ctx.fill();
    }

    // Dial Gauge Updater (Arc length: 235.6)
    function updateDialGauge(pct) {
      const totalArc = 235.6;
      const clamped = Math.max(0, Math.min(100, pct));
      const offset = totalArc - (clamped / 100 * totalArc);
      const path = document.getElementById('dial-path');
      path.style.strokeDashoffset = offset;

      let color = 'var(--color-safe)';
      if (clamped >= currentThreshold) {
        color = 'var(--color-danger)';
      } else if (clamped >= currentThreshold * 0.75) {
        color = 'var(--color-warn)';
      }
      path.style.stroke = color;

      document.getElementById('dial-percentage').innerText = clamped.toFixed(1) + '%';
      
      const label = document.getElementById('gas-level-label');
      if (clamped >= currentThreshold) {
        label.innerText = '🔥 CRITICAL GAS LEAK';
        label.style.color = 'var(--color-danger)';
      } else if (clamped >= currentThreshold * 0.75) {
        label.innerText = '⚠ï¸ ELEVATED LEVEL';
        label.style.color = 'var(--color-warn)';
      } else {
        label.innerText = 'ATMOSPHERE NOMINAL';
        label.style.color = 'var(--text-muted)';
      }
    }

    // Web Audio Synthesizer (for in-browser acoustic warning)
    function playBeep(freq, duration) {
      if (!isWebAudioEnabled || isMuted) return;
      try {
        if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
        if (audioCtx.state === 'suspended') audioCtx.resume();
        const osc = audioCtx.createOscillator();
        const gain = audioCtx.createGain();
        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(freq, audioCtx.currentTime);
        // Frequency sweep siren effect
        osc.frequency.exponentialRampToValueAtTime(freq * 1.4, audioCtx.currentTime + duration * 0.5);
        gain.gain.setValueAtTime(0.09, audioCtx.currentTime);
        gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + duration);
        osc.connect(gain);
        gain.connect(audioCtx.destination);
        osc.start();
        osc.stop(audioCtx.currentTime + duration);
      } catch (e) {}
    }

    function toggleBrowserAudio() {
      isWebAudioEnabled = !isWebAudioEnabled;
      const btn = document.getElementById('audio-btn');
      if (isWebAudioEnabled) {
        btn.innerHTML = '<span id="audio-icon">🔊</span> Web Siren: ON';
        btn.style.color = 'var(--color-safe)';
        btn.style.borderColor = 'rgba(16, 185, 129, 0.4)';
        playBeep(880, 0.15);
        logEvent('Browser acoustic siren enabled', 'info');
      } else {
        btn.innerHTML = '<span id="audio-icon">🔇</span> Web Siren: Off';
        btn.style.color = 'var(--text-secondary)';
        btn.style.borderColor = 'var(--border-subtle)';
        logEvent('Browser acoustic siren disabled', 'info');
      }
    }

    // Live Server Telemetry Polling
    let lastData = null;
    let prevStatus = 'SAFE';

    async function fetchStatus() {
      if (isDemoMode) {
        runDemoCycle();
        return;
      }

      try {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 1500);
        const res = await fetch('/api/status', { signal: controller.signal });
        clearTimeout(timeoutId);

        if (!res.ok) throw new Error('HTTP ' + res.status);
        const data = await res.json();
        consecutiveFetchErrors = 0;
        lastData = data;
        applySystemData(data);

        const pill = document.getElementById('network-pill');
        pill.className = 'pill pill-live';
        document.getElementById('network-label').innerText = 'LIVE ESP32 HARDWARE';
      } catch (err) {
        consecutiveFetchErrors++;
        // If 2 failed attempts, gracefully suggest or switch to simulation
        if (consecutiveFetchErrors >= 2 && !isDemoMode) {
          const pill = document.getElementById('network-pill');
          pill.className = 'pill pill-warn';
          document.getElementById('network-label').innerText = 'ESP32 OFFLINE (SIMULATING)';
          runDemoCycle();
        }
      }
    }

    function applySystemData(data) {
      currentThreshold = data.threshold;
      document.getElementById('stat-threshold').innerText = data.threshold + '%';
      document.getElementById('slider-display').innerText = data.threshold + '%';
      document.getElementById('threshold-slider').value = data.threshold;
      document.getElementById('stat-raw').innerText = data.gasRaw;

      // Estimated PPM mapping based on selected gas profile
      const curve = GAS_CURVES[selectedGasType] || GAS_CURVES['LPG'];
      const rawPct = Math.max(0, data.gasPercent);
      const estimatedPpm = Math.round(Math.pow(rawPct / 100, curve.exp) * curve.mult + curve.base);
      document.getElementById('stat-ppm').innerText = '~' + estimatedPpm + ' ppm';

      // Update Gauge & History Array
      updateDialGauge(data.gasPercent);
      historyPoints.shift();
      historyPoints.push(data.gasPercent);
      renderTrendChart();

      // Uptime Formatter
      const sec = data.uptime || 0;
      const h = String(Math.floor(sec / 3600)).padStart(2, '0');
      const m = String(Math.floor((sec % 3600) / 60)).padStart(2, '0');
      const s = String(sec % 60).padStart(2, '0');
      document.getElementById('telem-uptime').innerText = `${h}:${m}:${s}`;

      // Actuators
      const fanBadge = document.getElementById('fan-badge');
      const fanIcon = document.getElementById('fan-icon');
      const fanBtnText = document.getElementById('fan-btn-text');
      if (data.fan) {
        fanBadge.className = 'actuator-state-badge active';
        fanBadge.innerText = 'VENTILATING';
        fanIcon.className = 'icon-fan spinning';
        fanBtnText.innerText = 'Stop Fan (Active)';
      } else {
        fanBadge.className = 'actuator-state-badge';
        fanBadge.innerText = 'STANDBY';
        fanIcon.className = 'icon-fan';
        fanBtnText.innerText = 'Turn Fan ON';
      }

      const buzzerBadge = document.getElementById('buzzer-badge');
      if (data.buzzer) {
        buzzerBadge.className = 'actuator-state-badge danger';
        buzzerBadge.innerText = 'PULSING SIREN';
        playBeep(920, 0.2);
      } else {
        buzzerBadge.className = 'actuator-state-badge';
        buzzerBadge.innerText = isMuted ? 'MUTED' : 'IDLE';
      }

      // Primary Status Banner
      const banner = document.getElementById('banner');
      const title = document.getElementById('banner-title');
      const desc = document.getElementById('banner-desc');
      const emoji = document.getElementById('banner-emoji');
      const safety = document.getElementById('safety-box');

      if (data.status === 'FAULT') {
        banner.className = 'hazard-banner warning';
        title.innerHTML = '\u26a0\ufe0f SENSOR FAULT: CHECK MQ-2 WIRING';
        desc.innerText = 'The MQ-2 analog signal is lost (AO wire open or sensor unpowered). Gas detection is NOT active.';
        emoji.innerText = '\u26a0\ufe0f';
        safety.className = 'safety-actions';
        prevStatus = data.status;
        return;
      }
      banner.className = 'hazard-banner ' + data.status.toLowerCase();
      if (data.status === 'DANGER') {
        title.innerHTML = '🔴 DANGER: GAS LEAKAGE DETECTED!';
        desc.innerText = 'Critical gas concentration detected! Siren sounding and automated exhaust ventilation active.';
        emoji.innerText = '🔥';
        safety.className = 'safety-actions active';
        if (prevStatus !== 'DANGER') {
          logEvent(`🚨 CRITICAL ALARM TRIGGERED (${data.gasPercent.toFixed(1)}%)`, 'alarm');
        }
      } else if (data.status === 'WARNING') {
        title.innerHTML = '🟡 WARNING: ELEVATED GAS CONCENTRATION';
        desc.innerText = 'Sensor level is approaching critical threshold. Monitor environment and maintain airflow.';
        emoji.innerText = '⚠ï¸';
        safety.className = 'safety-actions';
        if (prevStatus !== 'WARNING' && prevStatus !== 'DANGER') {
          logEvent(`⚠ï¸ Elevated Gas Warning (${data.gasPercent.toFixed(1)}%)`, 'info');
        }
      } else {
        title.innerHTML = '🟢 STATUS: AIR QUALITY NORMAL';
        desc.innerText = 'Atmospheric gas concentration is within clean room baseline. No flammable gas or combustion smoke detected.';
        emoji.innerText = '✅';
        safety.className = 'safety-actions';
        if (prevStatus === 'DANGER' || prevStatus === 'WARNING') {
          logEvent('✅ Air cleared: Atmosphere returned to Normal', 'safe');
        }
      }
      prevStatus = data.status;
    }

    // Demo Simulation Engine
    let demoSec = 45;
    let demoGas = 14.0;
    let demoPhase = 0;
    let demoFan = false;
    let demoManualHold = false;

    function toggleDemoMode() {
      isDemoMode = !isDemoMode;
      const chip = document.getElementById('demo-chip');
      const pill = document.getElementById('network-pill');
      if (isDemoMode) {
        chip.style.background = 'rgba(6, 182, 212, 0.2)';
        chip.style.borderColor = 'var(--color-cyan)';
        chip.style.color = 'var(--color-cyan)';
        pill.className = 'pill pill-live';
        document.getElementById('network-label').innerText = 'DEMO SIMULATION ACTIVE';
        logEvent('Interactive Demo Simulation engaged', 'info');
      } else {
        chip.style.background = 'rgba(255, 255, 255, 0.04)';
        chip.style.borderColor = 'var(--border-subtle)';
        chip.style.color = 'var(--text-primary)';
        pill.className = 'pill';
        document.getElementById('network-label').innerText = 'CONNECTING HARDWARE...';
        logEvent('Switched to live hardware polling', 'info');
        consecutiveFetchErrors = 0;
      }
    }

    function runDemoCycle() {
      demoSec++;
      if (!demoManualHold) {
        demoPhase += 0.07;
        // Realistic ambient atmospheric drift (10% to 35%)
        demoGas = 16 + Math.sin(demoPhase) * 10 + (Math.random() * 3 - 1.5);
      } else {
        // Natural dissipation if fan is running
        if (demoFan && demoGas > 15) {
          demoGas -= 4.5;
          if (demoGas <= 15) {
            demoManualHold = false;
            demoGas = 15;
          }
        }
      }
      demoGas = Math.max(5, Math.min(95, demoGas));

      const isDanger = demoGas >= currentThreshold;
      const isWarn = demoGas >= (currentThreshold * 0.75);

      applySystemData({
        gasRaw: Math.round((demoGas / 100) * 4095),
        gasPercent: demoGas,
        threshold: currentThreshold,
        status: isDanger ? 'DANGER' : (isWarn ? 'WARNING' : 'SAFE'),
        fan: isDanger || demoFan,
        buzzer: isDanger && !isMuted,
        uptime: demoSec
      });
    }

    // Simulation Interactive Controls
    function simTriggerLeak() {
      demoManualHold = true;
      demoGas = 76.5;
      logEvent('Simulated instant LPG gas burst (76.5%)', 'alarm');
      runDemoCycle();
    }

    function simClearAtmosphere() {
      demoGas = 12.0;
      demoManualHold = false;
      logEvent('Atmosphere cleared back to baseline (12%)', 'safe');
      runDemoCycle();
    }

    function simStepGas(delta) {
      demoManualHold = true;
      demoGas = Math.max(5, Math.min(95, demoGas + delta));
      logEvent(`Simulated gas step: ${delta > 0 ? '+' : ''}${delta}% (Now: ${demoGas.toFixed(1)}%)`, 'info');
      runDemoCycle();
    }

    // Actuator Hardware Controls
    function onSliderMove(val) {
      document.getElementById('slider-display').innerText = val + '%';
      currentThreshold = parseInt(val);
      renderTrendChart();
    }

    async function sendThreshold(val) {
      logEvent('Alarm threshold set to ' + val + '%', 'info');
      if (isDemoMode) return;
      try {
        await fetch('/api/config?threshold=' + val, { method: 'POST' });
      } catch (e) {}
    }

    async function toggleFan() {
      if (isDemoMode) {
        demoFan = !demoFan;
        logEvent('Exhaust fan manually toggled: ' + (demoFan ? 'ON' : 'OFF'), 'info');
        runDemoCycle();
        return;
      }
      try {
        await fetch('/api/fan/toggle', { method: 'POST' });
        fetchStatus();
      } catch (e) {}
    }

    async function toggleMute() {
      isMuted = !isMuted;
      const btn = document.getElementById('mute-btn');
      const text = document.getElementById('mute-btn-text');
      const icon = document.getElementById('mute-icon');

      if (isMuted) {
        btn.className = 'actuator-btn btn-mute';
        btn.style.background = '#475569';
        text.innerText = 'Unmute Buzzer';
        icon.innerText = '🔊';
        logEvent('Audible siren buzzer MUTED', 'info');
      } else {
        btn.className = 'actuator-btn btn-mute';
        btn.style.background = '#334155';
        text.innerText = 'Mute Buzzer';
        icon.innerText = '🔇';
        logEvent('Audible siren buzzer UNMUTED', 'info');
      }

      if (!isDemoMode) {
        try {
          await fetch('/api/mute?state=' + (isMuted ? '1' : '0'), { method: 'POST' });
        } catch (e) {}
      }
    }

    async function triggerTestAlarm() {
      logEvent('🚨 Manual 4-Second Emergency Drill initiated', 'alarm');
      if (isDemoMode) {
        demoManualHold = true;
        demoGas = 82.0;
        runDemoCycle();
        setTimeout(() => {
          demoManualHold = false;
          logEvent('Emergency Drill completed', 'safe');
        }, 4000);
        return;
      }
      try {
        await fetch('/api/test', { method: 'POST' });
        fetchStatus();
      } catch (e) {}
    }

    // Initial system boot
    logEvent('System dashboard loaded • Ready', 'info');
    fetchStatus();
    setInterval(fetchStatus, 1000);
  </script>
</body>
</html>

)rawliteral";

#endif
