import mqtt from 'mqtt';
import { EventEmitter } from 'events';

// ================= 配置区 =================
const BROKER_URL = 'mqtt://8.134.167.240:1883';  // 阿里云服务器 IP
const TEST_TIMEOUT_MS = 3000;  // 每条命令等待响应的超时时间

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
  SET_AUTO_MODE: 0x35,
  FIRE_RESET: 0x36,
};

// ================= 测试用例定义 =================
const TEST_CASES = [
  // === 1. 设备发现 ===
  { name: '设备发现(DISCOVER)', topic: 'xiaozhi/iot/cmd/broadcast', cmd: CMD.DISCOVER, deviceId: 0, gpioIndex: 0, value: 0, expectResponse: true },

  // === 2. 广播命令 ===
  { name: '广播-全开(BROADCAST_ALL_ON)', topic: 'xiaozhi/iot/cmd/broadcast', cmd: CMD.BROADCAST_ALL_ON, deviceId: 0, gpioIndex: 0, value: 0, expectResponse: true },
  { name: '广播-全关(BROADCAST_ALL_OFF)', topic: 'xiaozhi/iot/cmd/broadcast', cmd: CMD.BROADCAST_ALL_OFF, deviceId: 0, gpioIndex: 0, value: 0, expectResponse: true },
  { name: '广播-全灯开(BROADCAST_LIGHTS_ON)', topic: 'xiaozhi/iot/cmd/broadcast', cmd: CMD.BROADCAST_LIGHTS_ON, deviceId: 0, gpioIndex: 0, value: 0, expectResponse: true },
  { name: '广播-全灯关(BROADCAST_LIGHTS_OFF)', topic: 'xiaozhi/iot/cmd/broadcast', cmd: CMD.BROADCAST_LIGHTS_OFF, deviceId: 0, gpioIndex: 0, value: 0, expectResponse: true },

  // === 3. 一楼命令 ===
  { name: '一楼-大厅灯开(SET_GPIO gpio0=1)', topic: 'xiaozhi/iot/cmd/1', cmd: CMD.SET_GPIO, deviceId: 1, gpioIndex: 0, value: 1, expectResponse: true },
  { name: '一楼-大厅灯关(SET_GPIO gpio0=0)', topic: 'xiaozhi/iot/cmd/1', cmd: CMD.SET_GPIO, deviceId: 1, gpioIndex: 0, value: 0, expectResponse: true },
  { name: '一楼-大门开(SET_SERVO servo0=180)', topic: 'xiaozhi/iot/cmd/1', cmd: CMD.SET_SERVO, deviceId: 1, gpioIndex: 6, value: 180, expectResponse: true },
  { name: '一楼-大门关(SET_SERVO servo0=0)', topic: 'xiaozhi/iot/cmd/1', cmd: CMD.SET_SERVO, deviceId: 1, gpioIndex: 6, value: 0, expectResponse: true },

  // === 4. 二楼命令 ===
  { name: '二楼-主卧灯开(SET_GPIO gpio0=1)', topic: 'xiaozhi/iot/cmd/2', cmd: CMD.SET_GPIO, deviceId: 2, gpioIndex: 0, value: 1, expectResponse: true },
  { name: '二楼-主卧灯关(SET_GPIO gpio0=0)', topic: 'xiaozhi/iot/cmd/2', cmd: CMD.SET_GPIO, deviceId: 2, gpioIndex: 0, value: 0, expectResponse: true },
  { name: '二楼-客厅灯开(SET_GPIO gpio1=1)', topic: 'xiaozhi/iot/cmd/2', cmd: CMD.SET_GPIO, deviceId: 2, gpioIndex: 1, value: 1, expectResponse: true },
  { name: '二楼-客厅灯关(SET_GPIO gpio1=0)', topic: 'xiaozhi/iot/cmd/2', cmd: CMD.SET_GPIO, deviceId: 2, gpioIndex: 1, value: 0, expectResponse: true },
  { name: '二楼-厕所灯开(SET_GPIO gpio2=1)', topic: 'xiaozhi/iot/cmd/2', cmd: CMD.SET_GPIO, deviceId: 2, gpioIndex: 2, value: 1, expectResponse: true },
  { name: '二楼-厕所灯关(SET_GPIO gpio2=0)', topic: 'xiaozhi/iot/cmd/2', cmd: CMD.SET_GPIO, deviceId: 2, gpioIndex: 2, value: 0, expectResponse: true },
  { name: '二楼-风扇开(SET_RELAY relay0=1)', topic: 'xiaozhi/iot/cmd/2', cmd: CMD.SET_RELAY, deviceId: 2, gpioIndex: 3, value: 1, expectResponse: true },
  { name: '二楼-风扇关(SET_RELAY relay0=0)', topic: 'xiaozhi/iot/cmd/2', cmd: CMD.SET_RELAY, deviceId: 2, gpioIndex: 3, value: 0, expectResponse: true },
  { name: '二楼-晾衣架收起(SET_SERVO servo0=0)', topic: 'xiaozhi/iot/cmd/2', cmd: CMD.SET_SERVO, deviceId: 2, gpioIndex: 6, value: 0, expectResponse: true },
  { name: '二楼-晾衣架展开(SET_SERVO servo0=90)', topic: 'xiaozhi/iot/cmd/2', cmd: CMD.SET_SERVO, deviceId: 2, gpioIndex: 6, value: 90, expectResponse: true },

  // === 5. 三楼命令 ===
  { name: '三楼-阳台灯开(SET_GPIO gpio0=1)', topic: 'xiaozhi/iot/cmd/3', cmd: CMD.SET_GPIO, deviceId: 3, gpioIndex: 0, value: 1, expectResponse: true },
  { name: '三楼-阳台灯关(SET_GPIO gpio0=0)', topic: 'xiaozhi/iot/cmd/3', cmd: CMD.SET_GPIO, deviceId: 3, gpioIndex: 0, value: 0, expectResponse: true },
  { name: '三楼-左天窗开(SET_SERVO servo0=90)', topic: 'xiaozhi/iot/cmd/3', cmd: CMD.SET_SERVO, deviceId: 3, gpioIndex: 6, value: 90, expectResponse: true },
  { name: '三楼-左天窗关(SET_SERVO servo0=0)', topic: 'xiaozhi/iot/cmd/3', cmd: CMD.SET_SERVO, deviceId: 3, gpioIndex: 6, value: 0, expectResponse: true },
  { name: '三楼-右天窗开(SET_SERVO servo1=90)', topic: 'xiaozhi/iot/cmd/3', cmd: CMD.SET_SERVO, deviceId: 3, gpioIndex: 7, value: 90, expectResponse: true },
  { name: '三楼-右天窗关(SET_SERVO servo1=0)', topic: 'xiaozhi/iot/cmd/3', cmd: CMD.SET_SERVO, deviceId: 3, gpioIndex: 7, value: 0, expectResponse: true },
  { name: '三楼-晾衣架收起(SET_SERVO servo2=0)', topic: 'xiaozhi/iot/cmd/3', cmd: CMD.SET_SERVO, deviceId: 3, gpioIndex: 8, value: 0, expectResponse: true },
  { name: '三楼-晾衣架展开(SET_SERVO servo2=90)', topic: 'xiaozhi/iot/cmd/3', cmd: CMD.SET_SERVO, deviceId: 3, gpioIndex: 8, value: 90, expectResponse: true },

  // === 6. 场景命令 ===
  { name: '庭院-西庭灯开(SET_LIGHT light0=1)', topic: 'xiaozhi/iot/cmd/10', cmd: CMD.SET_LIGHT, deviceId: 10, gpioIndex: 0, value: 1, expectResponse: true },
  { name: '庭院-西庭灯关(SET_LIGHT light0=0)', topic: 'xiaozhi/iot/cmd/10', cmd: CMD.SET_LIGHT, deviceId: 10, gpioIndex: 0, value: 0, expectResponse: true },
  { name: '庭院-长廊灯开(SET_LIGHT light2=1)', topic: 'xiaozhi/iot/cmd/10', cmd: CMD.SET_LIGHT, deviceId: 10, gpioIndex: 2, value: 1, expectResponse: true },
  { name: '庭院-水泵开(SET_RELAY relay1=1)', topic: 'xiaozhi/iot/cmd/10', cmd: CMD.SET_RELAY, deviceId: 10, gpioIndex: 1, value: 1, expectResponse: true },
  { name: '庭院-水泵关(SET_RELAY relay1=0)', topic: 'xiaozhi/iot/cmd/10', cmd: CMD.SET_RELAY, deviceId: 10, gpioIndex: 1, value: 0, expectResponse: true },
  { name: '庭院-晾衣架展开(SET_SERVO servo0=180)', topic: 'xiaozhi/iot/cmd/10', cmd: CMD.SET_SERVO, deviceId: 10, gpioIndex: 6, value: 180, expectResponse: true },
  { name: '庭院-晾衣架收回(SET_SERVO servo0=0)', topic: 'xiaozhi/iot/cmd/10', cmd: CMD.SET_SERVO, deviceId: 10, gpioIndex: 6, value: 0, expectResponse: true },
  { name: '庭院-自动模式开(SET_AUTO_MODE)', topic: 'xiaozhi/iot/cmd/10', cmd: CMD.SET_AUTO_MODE, deviceId: 10, gpioIndex: 0, value: 1, expectResponse: true },
  { name: '庭院-火灾复位(FIRE_RESET)', topic: 'xiaozhi/iot/cmd/10', cmd: CMD.FIRE_RESET, deviceId: 10, gpioIndex: 0, value: 0, expectResponse: true },

  // === 7. 场景命令 ===
  { name: '场景-回家(ANNOUNCE_V2)', topic: 'xiaozhi/iot/cmd/broadcast', cmd: CMD.ANNOUNCE_V2, deviceId: 0, gpioIndex: 0, value: 0, expectResponse: true },
  { name: '场景-紧急(EMERGENCY)', topic: 'xiaozhi/iot/cmd/broadcast', cmd: CMD.EMERGENCY, deviceId: 0, gpioIndex: 0, value: 1, expectResponse: true },

  // === 8. 心跳 ===
  { name: '心跳(HEARTBEAT)', topic: 'xiaozhi/iot/cmd/broadcast', cmd: CMD.HEARTBEAT, deviceId: 0, gpioIndex: 0, value: 0, expectResponse: false },
];

// ================= 工具函数 =================
function buildPacket(cmd, deviceId, gpioIndex, value) {
  const buf = Buffer.alloc(8);
  buf.writeUInt8(cmd, 0);
  buf.writeUInt8(deviceId, 1);
  buf.writeUInt8(gpioIndex, 2);
  buf.writeUInt8(value, 3);
  buf.writeUInt32LE(0, 4);
  return buf;
}

function cmdToHex(cmd) {
  return '0x' + cmd.toString(16).toUpperCase().padStart(2, '0');
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// ================= 测试主流程 =================
async function runTests() {
  console.log('='.repeat(70));
  console.log('  IoT MQTT 自动化测试套件');
  console.log('='.repeat(70));
  console.log(`Broker: ${BROKER_URL}`);
  console.log(`测试用例: ${TEST_CASES.length} 条`);
  console.log(`响应超时: ${TEST_TIMEOUT_MS}ms`);
  console.log('='.repeat(70));
  console.log('');

  const responseBuffer = new Map(); // mac -> { command, gpioIndex, value }
  const emitter = new EventEmitter();

  // 连接 MQTT
  console.log('正在连接 MQTT broker...');
  const client = mqtt.connect(BROKER_URL, {
    clientId: `test_client_${Date.now()}`,
    reconnectPeriod: 0,
    connectTimeout: 5000,
  });

  await new Promise((resolve, reject) => {
    client.on('connect', () => {
      console.log('  MQTT 连接成功!\n');
      resolve(null);
    });
    client.on('error', (err) => reject(err));
    client.on('offline', () => reject(new Error('MQTT 连接断开')));
  });

  // 订阅所有响应 topic
  await new Promise((resolve) => {
    client.subscribe([
      'xiaozhi/iot/resp/+',
      'xiaozhi/iot/heartbeat/+',
      'xiaozhi/iot/announce/+',
      'xiaozhi/iot/sensor/+',
    ], () => {
      console.log('  已订阅 resp/+, heartbeat/+, announce/+, sensor/+');
      console.log('');
      resolve();
    });
  });

  // 监听消息
  client.on('message', (topic, message) => {
    const mac = topic.replace(/.*\//, '');
    const cmd = message.readUInt8(0);
    const gpioIndex = message.readUInt8(2);
    const value = message.readUInt8(3);
    const deviceId = message.readUInt8(1);

    console.log(`  <-- 响应 [${mac}] cmd=${cmdToHex(cmd)} dev=${deviceId} gpio=${gpioIndex} val=${value}`);
    responseBuffer.set(mac, { command: cmd, gpioIndex, value, deviceId });
    emitter.emit('response', mac);
  });

  // 逐条发送测试
  let passed = 0;
  let failed = 0;
  const results = [];

  for (let i = 0; i < TEST_CASES.length; i++) {
    const tc = TEST_CASES[i];
    const num = String(i + 1).padStart(2, ' ');

    // 构建并发送数据包
    const packet = buildPacket(tc.cmd, tc.deviceId, tc.gpioIndex, tc.value);
    console.log(`[${num}] ${tc.name}`);
    console.log(`      topic=${tc.topic}, packet=[${Array.from(packet).map(b => b.toString(16).padStart(2, '0')).join(' ')}]`);

    client.publish(tc.topic, packet, { qos: 1 });

    if (tc.expectResponse) {
      // 等待响应
      const result = await Promise.race([
        new Promise((resolve) => {
          const handler = (mac) => {
            emitter.off('response', handler);
            const resp = responseBuffer.get(mac);
            resolve({ mac, resp });
          };
          emitter.on('response', handler);
        }),
        new Promise((resolve) => setTimeout(() => resolve(null), TEST_TIMEOUT_MS)),
      ]);

      if (result) {
        console.log(`      [PASS] 收到来自 ${result.mac} 的响应: cmd=${cmdToHex(result.resp.command)}`);
        passed++;
        results.push({ name: tc.name, status: 'PASS', mac: result.mac });
      } else {
        console.log(`      [FAIL] 超时 ${TEST_TIMEOUT_MS}ms 未收到响应`);
        failed++;
        results.push({ name: tc.name, status: 'FAIL', mac: null });
      }
    } else {
      // 不需要响应的命令(如心跳)，等待短暂时间看看是否有意外响应
      await sleep(500);
      console.log(`      [INFO] 单向命令，不等待响应`);
      passed++;
      results.push({ name: tc.name, status: 'PASS (no response expected)', mac: null });
    }

    console.log('');
    await sleep(300); // 命令间隔，避免设备过载
  }

  // ================= 测试汇总 =================
  console.log('='.repeat(70));
  console.log('  测试结果汇总');
  console.log('='.repeat(70));

  const total = passed + failed;
  const passRate = total > 0 ? ((passed / total) * 100).toFixed(1) : 0;

  console.log(`  总计: ${total} 条`);
  console.log(`  通过: ${passed} 条 (${passRate}%)`);
  console.log(`  失败: ${failed} 条`);
  console.log('');

  if (failed > 0) {
    console.log('  失败列表:');
    results.filter(r => r.status === 'FAIL').forEach(r => {
      console.log(`    - ${r.name}`);
    });
    console.log('');
  }

  if (failed === 0) {
    console.log('  全部通过! 项目 MQTT 控制功能接近完美!');
  } else if (failed <= 2) {
    console.log('  仅有少量失败，项目基本可用。');
  } else {
    console.log('  多项测试失败，请检查:');
    console.log('    1. ESP32 设备是否在线并连接到同一 broker');
    console.log('    2. broker IP 是否正确 (当前: 192.168.1.100)');
    console.log('    3. 设备固件是否正常响应 MQTT 命令');
  }

  console.log('='.repeat(70));

  // 清理
  client.end();
  process.exit(failed > 0 ? 1 : 0);
}

// 启动
runTests().catch((err) => {
  console.error(`错误: ${err.message}`);
  console.error('请检查 MQTT broker 是否运行在 ' + BROKER_URL);
  process.exit(1);
});
