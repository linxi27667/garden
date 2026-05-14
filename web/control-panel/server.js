const mqtt = require('mqtt');
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const PASSWORD = process.env.PANEL_PASSWORD || 'ljc12345';
const SESSION_MAX_AGE_MS = 24 * 60 * 60 * 1000;
const SESSIONS = new Map();

const IOT_CMD_SET_GPIO = 0x01;
const IOT_CMD_HEARTBEAT = 0x04;
const IOT_CMD_DISCOVER = 0x06;
const IOT_CMD_ANNOUNCE_V2 = 0x08;
const IOT_CMD_SET_SERVO = 0x10;
const IOT_CMD_SET_LIGHT = 0x11;
const IOT_CMD_SET_RELAY = 0x12;
const IOT_CMD_SENSOR_REPORT = 0x13;
const IOT_CMD_RESPONSE = 0x05;

const SENSOR_MAP = { 0: 'soil_mv', 1: 'light_mv', 2: 'fire_status', 3: 'rain_status', 4: 'help_status' };

const MQTT_BROKER = process.env.MQTT_BROKER || process.argv[2] || 'mqtt://127.0.0.1';
const MQTT_PORT = process.env.MQTT_PORT || 1883;
const HTTP_PORT = process.env.HTTP_PORT || 3000;
const DEVICE_TIMEOUT_MS = 65000;

const PUBLIC_DIR = path.resolve(__dirname, 'public');

const devices = {};
const wsClients = new Set();

function parseCmd(buf) {
  if (buf.length < 8) return null;
  return { command: buf.readUInt8(0), device_id: buf.readUInt8(1), gpio_index: buf.readUInt8(2), value: buf.readUInt8(3), reserved: [buf.readUInt8(4), buf.readUInt8(5), buf.readUInt8(6), buf.readUInt8(7)] };
}
function parseAnnounceV2(buf) {
  if (buf.length < 28 || buf.readUInt8(0) !== IOT_CMD_ANNOUNCE_V2) return null;
  const nb = buf.subarray(4, 28); const ni = nb.indexOf(0);
  return { device_id: buf.readUInt8(1), gpio_index: buf.readUInt8(2), device_name: nb.toString('utf8', 0, ni >= 0 ? ni : 24) };
}
function extMac(t) { const m = t.match(/xiaozhi\/iot\/(?:resp|heartbeat|announce|sensor)\/([A-Fa-f0-9]{12})/); return m ? m[1].toUpperCase() : null; }
function broadcast(j) { const d = JSON.stringify(j); for (const w of wsClients) { if (w.readyState === WebSocket.OPEN) w.send(d); } }

// Session with expiry
function genToken() { return crypto.randomBytes(24).toString('hex'); }
function addSession(token) { SESSIONS.set(token, Date.now()); }
function isAuthed(cookies, q) {
  const t = (cookies || '').split(';').map(s => s.trim()).find(c => c.startsWith('token='));
  const qt = q ? q.token : null;
  const token = (t ? t.split('=')[1] : null) || qt || '';
  if (!SESSIONS.has(token)) return false;
  const created = SESSIONS.get(token);
  if (Date.now() - created > SESSION_MAX_AGE_MS) { SESSIONS.delete(token); return false; }
  return true;
}
function cleanExpiredSessions() {
  const now = Date.now();
  for (const [token, created] of SESSIONS) {
    if (now - created > SESSION_MAX_AGE_MS) SESSIONS.delete(token);
  }
}
setInterval(cleanExpiredSessions, 60000);

// MQTT
const mqttClient = mqtt.connect(MQTT_BROKER, { port: Number(MQTT_PORT), clientId: 'garden_panel_' + Math.random().toString(36).substr(2, 6), clean: true, protocolVersion: 4, connectTimeout: 10000, rejectUnauthorized: false });
mqttClient.on('connect', () => {
  console.log('MQTT connected');
  mqttClient.subscribe('xiaozhi/iot/announce/+', { qos: 1 });
  mqttClient.subscribe('xiaozhi/iot/heartbeat/+');
  mqttClient.subscribe('xiaozhi/iot/resp/+');
  mqttClient.subscribe('xiaozhi/iot/sensor/+');
  setTimeout(() => { const b = Buffer.alloc(8, 0); b.writeUInt8(IOT_CMD_DISCOVER, 0); mqttClient.publish('xiaozhi/iot/cmd/broadcast', b, { qos: 1 }); console.log('Discover sent'); }, 2000);
});
mqttClient.on('error', (e) => console.error('MQTT error:', e.message));
mqttClient.on('message', (topic, payload) => {
  const mac = extMac(topic); if (!mac) return;
  if (!devices[mac]) devices[mac] = { mac, name: 'Unknown', device_id: 0, gpio_count: 0, online: true, gpio_states: {}, sensors: {}, last_seen: Date.now() };
  const dev = devices[mac]; dev.last_seen = Date.now(); dev.online = true;
  const cmd = parseCmd(payload); if (!cmd) return;
  if (cmd.command === IOT_CMD_ANNOUNCE_V2) { const a = parseAnnounceV2(payload); if (a) { dev.name = a.device_name; dev.device_id = a.device_id; dev.gpio_count = a.gpio_index; console.log('Device:', dev.name, dev.device_id, dev.gpio_count); } }
  else if (cmd.command === IOT_CMD_SENSOR_REPORT) { const v16 = cmd.value | (cmd.reserved[0] << 8); const k = SENSOR_MAP[cmd.gpio_index]; if (k) { dev.sensors[k] = v16; dev.sensors._ts = Date.now(); } }
  else if (cmd.command === IOT_CMD_RESPONSE) { if (cmd.gpio_index < dev.gpio_count) dev.gpio_states[cmd.gpio_index] = cmd.value; }
  broadcast({ type: 'device', mac, device: dev });
});
setInterval(() => { const n = Date.now(); for (const m in devices) { if (devices[m].online && (n - devices[m].last_seen) > DEVICE_TIMEOUT_MS) { devices[m].online = false; broadcast({ type: 'device', mac: m, device: devices[m] }); } } }, 10000);

// HTTP
const MIME = { '.html': 'text/html;charset=utf-8', '.js': 'text/javascript', '.css': 'text/css' };

function sanitizePath(requestPath) {
  const resolved = path.resolve(PUBLIC_DIR, '.' + requestPath);
  if (!resolved.startsWith(PUBLIC_DIR + path.sep) && resolved !== PUBLIC_DIR) return null;
  return resolved;
}

function sendFile(res, filePath) {
  fs.readFile(filePath, (err, data) => {
    if (err) { res.writeHead(404); res.end('Not found'); return; }
    res.writeHead(200, { 'Content-Type': MIME[path.extname(filePath)] || 'text/plain' });
    res.end(data);
  });
}

function parseCookies(r) {
  const c = {}; (r.headers.cookie || '').split(';').forEach(s => { const p = s.trim().indexOf('='); if (p > 0) { c[s.trim().substring(0, p)] = s.trim().substring(p + 1); } }); return c;
}

const server = http.createServer((req, res) => {
  const url = new URL(req.url, 'http://localhost');
  const cookies = parseCookies(req);

  if (req.method === 'POST' && url.pathname === '/api/login') {
    let body = '';
    req.on('data', c => { if (body.length < 4096) body += c; });
    req.on('end', () => {
      try {
        const { password } = JSON.parse(body);
        if (password === PASSWORD) {
          const token = genToken();
          addSession(token);
          res.writeHead(200, { 'Content-Type': 'application/json', 'Set-Cookie': 'token=' + token + '; Path=/; Max-Age=86400; HttpOnly; SameSite=Lax' });
          res.end(JSON.stringify({ success: true, token }));
        } else {
          res.writeHead(401, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ success: false, message: '密码错误' }));
        }
      } catch (e) { res.writeHead(400); res.end('Bad request'); }
    });
    return;
  }

  if (url.pathname === '/' || url.pathname === '/login.html') {
    return sendFile(res, path.join(PUBLIC_DIR, 'login.html'));
  }

  if (!isAuthed(req.headers.cookie || '', Object.fromEntries(url.searchParams))) {
    if (url.pathname.startsWith('/api/')) {
      res.writeHead(401, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ success: false, message: 'Unauthorized' }));
    } else {
      res.writeHead(302, { 'Location': '/' });
      res.end();
    }
    return;
  }

  const safePath = sanitizePath(url.pathname);
  if (!safePath) {
    res.writeHead(403);
    res.end('Forbidden');
    return;
  }
  sendFile(res, safePath);
});

// WebSocket
const wss = new WebSocket.Server({ server });
wss.on('connection', (ws, req) => {
  let authed = false;
  const q = new URL(req.url, 'http://localhost').searchParams;
  if (isAuthed(req.headers.cookie || '', Object.fromEntries(q))) authed = true;

  ws.on('message', (data) => {
    try {
      const msg = JSON.parse(data);

      if (msg.type === 'auth') {
        if (SESSIONS.has(msg.token) && isAuthed('', { token: msg.token })) authed = true;
        else { ws.close(4001, 'Unauthorized'); return; }
        return;
      }

      if (!authed) { ws.close(4001); return; }

      if (msg.type === 'init') {
        for (const mac in devices) {
          ws.send(JSON.stringify({ type: 'device', mac, device: devices[mac] }));
        }
        return;
      }

      switch (msg.type) {
        case 'discover': { const b = Buffer.alloc(8, 0); b.writeUInt8(IOT_CMD_DISCOVER, 0); mqttClient.publish('xiaozhi/iot/cmd/broadcast', b, { qos: 1 }); break; }
        case 'set_light': { const b = Buffer.alloc(8, 0); b.writeUInt8(IOT_CMD_SET_LIGHT, 0); b.writeUInt8(msg.device_id, 1); b.writeUInt8(msg.index, 2); b.writeUInt8(msg.value ? 1 : 0, 3); mqttClient.publish('xiaozhi/iot/cmd/' + msg.device_id, b, { qos: 1 }); break; }
        case 'set_relay': { const b = Buffer.alloc(8, 0); b.writeUInt8(IOT_CMD_SET_GPIO, 0); b.writeUInt8(msg.device_id, 1); b.writeUInt8(3 + msg.index, 2); b.writeUInt8(msg.value ? 1 : 0, 3); mqttClient.publish('xiaozhi/iot/cmd/' + msg.device_id, b, { qos: 1 }); break; }
        case 'set_servo': { const b = Buffer.alloc(8, 0); b.writeUInt8(IOT_CMD_SET_SERVO, 0); b.writeUInt8(msg.device_id, 1); b.writeUInt8(6 + msg.index, 2); b.writeUInt8(msg.angle, 3); mqttClient.publish('xiaozhi/iot/cmd/' + msg.device_id, b, { qos: 1 }); break; }
      }
    } catch (e) { console.error('WS error:', e); }
  });
});

server.listen(HTTP_PORT, '0.0.0.0', () => console.log('Panel: http://0.0.0.0:' + HTTP_PORT));
