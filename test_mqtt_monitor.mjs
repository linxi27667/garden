import mqtt from 'mqtt';

// ================= 配置区 =================
const BROKER_URL = 'mqtt://8.134.167.240:1883';  // 阿里云服务器 IP

// ================= 命令码定义 =================
const CMD = {
  SET_GPIO: 0x01,
  GET_GPIO: 0x02,
  GET_ALL_GPIO: 0x03,
  HEARTBEAT: 0x04,
  RESPONSE: 0x05,
  DISCOVER: 0x06,
  ANNOUNCE: 0x07,
  ANNOUNCE_V2: 0x08,
  SET_SERVO: 0x10,
  SET_LIGHT: 0x11,
  SET_RELAY: 0x12,
  SENSOR_REPORT: 0x13,
  BROADCAST_ALL_OFF: 0x30,
  BROADCAST_ALL_ON: 0x31,
  BROADCAST_LIGHTS_OFF: 0x32,
  BROADCAST_LIGHTS_ON: 0x33,
  EMERGENCY: 0x34,
};

const CMD_NAMES = {
  0x01: 'SET_GPIO',
  0x02: 'GET_GPIO',
  0x03: 'GET_ALL_GPIO',
  0x04: 'HEARTBEAT',
  0x05: 'RESPONSE',
  0x06: 'DISCOVER',
  0x07: 'ANNOUNCE',
  0x08: 'ANNOUNCE_V2',
  0x10: 'SET_SERVO',
  0x11: 'SET_LIGHT',
  0x12: 'SET_RELAY',
  0x13: 'SENSOR_REPORT',
  0x30: 'BROADCAST_ALL_OFF',
  0x31: 'BROADCAST_ALL_ON',
  0x32: 'BROADCAST_LIGHTS_OFF',
  0x33: 'BROADCAST_LIGHTS_ON',
  0x34: 'EMERGENCY',
};

// ================= 解析数据包 =================
function parsePacket(buf) {
  if (buf.length < 8) return null;
  
  const cmd = buf.readUInt8(0);
  const deviceId = buf.readUInt8(1);
  const gpioIndex = buf.readUInt8(2);
  const value = buf.readUInt8(3);
  
  return { cmd, deviceId, gpioIndex, value };
}

// ================= 格式化输出 =================
function formatMessage(topic, message) {
  const timestamp = new Date().toLocaleTimeString('zh-CN', { hour12: false });
  const hex = Array.from(message).map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ');
  
  console.log(`\n[${timestamp}] 📨 ${topic}`);
  console.log(`   HEX: ${hex}`);
  
  // 尝试解析为命令包
  const parsed = parsePacket(message);
  if (parsed) {
    const cmdName = CMD_NAMES[parsed.cmd] || `UNKNOWN(0x${parsed.cmd.toString(16).toUpperCase()})`;
    console.log(`   CMD: ${cmdName}`);
    console.log(`   Device: ${parsed.deviceId}, GPIO: ${parsed.gpioIndex}, Value: ${parsed.value}`);
    
    // 特殊命令的友好提示
    if (parsed.cmd === CMD.SET_GPIO) {
      console.log(`   → 设置 GPIO[${parsed.gpioIndex}] = ${parsed.value}`);
    } else if (parsed.cmd === CMD.SET_SERVO) {
      console.log(`   → 设置舵机[${parsed.gpioIndex}] = ${parsed.value}°`);
    } else if (parsed.cmd === CMD.SET_LIGHT) {
      console.log(`   → 设置灯光[${parsed.gpioIndex}] = ${parsed.value}`);
    } else if (parsed.cmd === CMD.SET_RELAY) {
      console.log(`   → 设置继电器[${parsed.gpioIndex}] = ${parsed.value}`);
    } else if (parsed.cmd === CMD.BROADCAST_ALL_ON) {
      console.log(`   → 广播：全部开启`);
    } else if (parsed.cmd === CMD.BROADCAST_ALL_OFF) {
      console.log(`   → 广播：全部关闭`);
    } else if (parsed.cmd === CMD.EMERGENCY) {
      console.log(`   → ⚠️ 紧急命令！`);
    }
  }
  
  console.log('─'.repeat(60));
}

// ================= 主流程 =================
async function runMonitor() {
  console.log('='.repeat(60));
  console.log('   MQTT 实时监控器');
  console.log('='.repeat(60));
  console.log(`Broker: ${BROKER_URL}`);
  console.log('监控所有 IoT 设备消息...');
  console.log('按 Ctrl+C 退出');
  console.log('='.repeat(60));
  console.log('');
  
  const client = mqtt.connect(BROKER_URL, {
    clientId: `mqtt_monitor_${Date.now()}`,
    reconnectPeriod: 5000,
  });
  
  client.on('connect', () => {
    console.log('✅ 已连接到 MQTT Broker');
    
    // 订阅所有相关 topic
    const topics = [
      'xiaozhi/iot/cmd/#',      // 所有命令
      'xiaozhi/iot/resp/#',     // 所有响应
      'xiaozhi/iot/announce/#', // 设备宣告
      'xiaozhi/iot/heartbeat/#',// 心跳
      'xiaozhi/iot/sensor/#',   // 传感器数据
    ];
    
    client.subscribe(topics, (err) => {
      if (err) {
        console.error('❌ 订阅失败:', err);
      } else {
        console.log('📋 已订阅主题:');
        topics.forEach(t => console.log(`   - ${t}`));
        console.log('');
        console.log('等待消息...');
        console.log('');
      }
    });
  });
  
  client.on('message', (topic, message) => {
    formatMessage(topic, message);
  });
  
  client.on('error', (err) => {
    console.error('❌ MQTT 错误:', err.message);
  });
  
  client.on('close', () => {
    console.log('⚠️  连接已断开，正在重连...');
  });
  
  client.on('reconnect', () => {
    console.log('🔄 正在重连...');
  });
}

runMonitor().catch((err) => {
  console.error('❌ 启动失败:', err);
  process.exit(1);
});
