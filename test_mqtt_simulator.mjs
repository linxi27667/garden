import mqtt from 'mqtt';
import { EventEmitter } from 'events';

const BROKER_URL = 'mqtt://8.134.167.240:1883';  // 阿里云服务器 IP

// 设备定义
const DEVICES = [
  {
    name: '1F_Device',
    displayName: '一楼设备',
    deviceId: 1,
    mac: 'AA0000000001',
    gpioCount: 9,
    gpioNames: ['hall_light', 'light2', 'light3', 'relay0', 'relay1', 'relay2', 'door_servo', 'servo1', 'servo2'],
    gpioStates: new Array(9).fill(0),
  },
  {
    name: '2F_Device',
    displayName: '二楼设备',
    deviceId: 2,
    mac: 'AA0000000002',
    gpioCount: 9,
    gpioNames: ['master_light', 'living_light', 'bath_light', 'fan_relay', 'relay1', 'relay2', 'rack_servo', 'servo1', 'servo2'],
    gpioStates: new Array(9).fill(0),
  },
  {
    name: '3F_Device',
    displayName: '三楼设备',
    deviceId: 3,
    mac: 'AA0000000003',
    gpioCount: 9,
    gpioNames: ['balcony_light', 'light2', 'light3', 'relay0', 'relay1', 'relay2', 'left_window', 'right_window', 'rack_servo'],
    gpioStates: new Array(9).fill(0),
    sensors: { smoke_mv: 1200, rain_mv: 2800, fire_status: 0, rain_status: 0, help_status: 0 },
  },
  {
    name: 'Garden_Device',
    displayName: '智慧庭院',
    deviceId: 10,
    mac: 'AA0000000010',
    gpioCount: 10,
    gpioNames: ['courtyard_west_light', 'strip_light', 'corridor_long_light', 'fan', 'water_pump', 'atomizer', 'safety_switch', 'clothes_servo', 'fire_sensor', 'help_button'],
    gpioStates: new Array(10).fill(0),
    sensors: { soil_mv: 1450, light_mv: 2800, fire_status: 0, rain_status: 0, help_status: 0 },
  },
];

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

function buildResponsePacket(cmd, deviceId, gpioIndex, value) {
  const buf = Buffer.alloc(8);
  buf.writeUInt8(cmd, 0);
  buf.writeUInt8(deviceId, 1);
  buf.writeUInt8(gpioIndex, 2);
  buf.writeUInt8(value, 3);
  buf.writeUInt32LE(0, 4);
  return buf;
}

function buildAnnounceV2Packet(deviceName, gpioCount = 9) {
  const buf = Buffer.alloc(28);
  buf.writeUInt8(CMD.ANNOUNCE_V2, 0);
  buf.writeUInt8(0, 1);
  buf.writeUInt8(gpioCount, 2);
  buf.writeUInt8(0, 3);
  // device_name at offset 4, 24 bytes - use ASCII only to avoid UTF-8 issues
  const asciiName = deviceName.replace(/[^\x00-\x7F]/g, '');
  buf.write(asciiName, 4, 24, 'utf8');
  return buf;
}

function buildSensorPacket(deviceId, sensorType, value) {
  const buf = Buffer.alloc(8);
  buf.writeUInt8(CMD.SENSOR_REPORT, 0);
  buf.writeUInt8(deviceId, 1);
  buf.writeUInt8(sensorType, 2);
  buf.writeUInt8(value & 0xFF, 3);
  buf.writeUInt8((value >> 8) & 0xFF, 4);  // reserved[0] = high byte
  buf.writeUInt16LE(0, 5);
  return buf;
}

function hexStr(buf) {
  return Array.from(buf).map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ');
}

async function runSimulator() {
  console.log('='.repeat(60));
  console.log('  ESP32 设备模拟器');
  console.log('='.repeat(60));
  console.log('模拟设备:');
  for (const d of DEVICES) {
    console.log(`  [${d.deviceId}] ${d.displayName} (MAC: ${d.mac}, GPIOs: ${d.gpioCount})`);
  }
  console.log(`Broker: ${BROKER_URL}`);
  console.log('='.repeat(60));
  console.log('');

  // 每个设备一个 MQTT 客户端
  const clients = [];

  for (const device of DEVICES) {
    const client = mqtt.connect(BROKER_URL, {
      clientId: `esp32_sim_${device.mac}`,
      reconnectPeriod: 0,
    });

    clients.push({ client, device });

    await new Promise((resolve, reject) => {
      client.on('connect', () => {
        console.log(`  [${device.displayName}] 连接 broker 成功`);
        resolve(null);
      });
      client.on('error', (err) => reject(err));
    });

    // 发送 ANNOUNCE_V2
    const announceBuf = buildAnnounceV2Packet(device.name, device.gpioCount);
    client.publish(`xiaozhi/iot/announce/${device.mac}`, announceBuf, { qos: 1 });
    console.log(`  [${device.displayName}] 已发送 ANNOUNCE_V2`);

    // 上报传感器数据（三楼）
    if (device.sensors) {
      const sensorTypes = [0, 1, 2, 3, 4];
      const sensorValues = [
        device.sensors.smoke_mv ?? device.sensors.soil_mv ?? 0,
        device.sensors.rain_mv ?? device.sensors.light_mv ?? 0,
        device.sensors.fire_status,
        device.sensors.rain_status,
        device.sensors.help_status
      ];
      for (let i = 0; i < sensorTypes.length; i++) {
        const sensorBuf = buildSensorPacket(device.deviceId, sensorTypes[i], sensorValues[i]);
        client.publish(`xiaozhi/iot/sensor/${device.mac}`, sensorBuf, { qos: 0 });
      }
      console.log(`  [${device.displayName}] 已上报传感器数据`);
    }

    // 订阅命令 topic
    await new Promise((resolve) => {
      client.subscribe([
        `xiaozhi/iot/cmd/${device.deviceId}`,
        'xiaozhi/iot/cmd/broadcast',
      ], () => {
        console.log(`  [${device.displayName}] 已订阅 xiaozhi/iot/cmd/${device.deviceId} 和 broadcast`);
        resolve();
      });
    });

    // 处理命令
    client.on('message', (topic, message) => {
      if (message.length < 8) return;
      const cmd = message.readUInt8(0);
      const msgDeviceId = message.readUInt8(1);
      const gpioIndex = message.readUInt8(2);
      const value = message.readUInt8(3);

      // 广播命令：所有设备都响应
      const isBroadcast = topic.includes('broadcast');
      // 单播命令：只处理自己的
      if (!isBroadcast && msgDeviceId !== device.deviceId) return;

      console.log(`  [${device.displayName}] <-- 收到命令: cmd=0x${cmd.toString(16).toUpperCase()} gpio=${gpioIndex} val=${value} (topic=${topic})`);

      let responseCmd = CMD.RESPONSE;
      let respGpio = gpioIndex;
      let respVal = value;

      switch (cmd) {
        case CMD.DISCOVER:
          // 重新发送 announce
          client.publish(`xiaozhi/iot/announce/${device.mac}`, announceBuf, { qos: 1 });
          console.log(`      -> 重发 ANNOUNCE_V2`);
          return;

        case CMD.SET_GPIO:
          device.gpioStates[gpioIndex] = value;
          console.log(`      -> GPIO[${gpioIndex}] = ${value}`);
          break;

        case CMD.SET_LIGHT:
          // light_index maps to gpio 0-2
          device.gpioStates[gpioIndex] = value;
          console.log(`      -> LIGHT[${gpioIndex}] = ${value}`);
          break;

        case CMD.SET_RELAY:
          // relay_index maps to gpio 3-5
          device.gpioStates[gpioIndex] = value;
          console.log(`      -> RELAY[${gpioIndex}] = ${value}`);
          break;

        case CMD.SET_SERVO:
          // servo_index maps to gpio 6-8
          device.gpioStates[gpioIndex] = value;
          console.log(`      -> SERVO[${gpioIndex}] = ${value}°`);
          break;

        case CMD.BROADCAST_ALL_OFF:
          device.gpioStates.fill(0);
          console.log(`      -> ALL OFF (所有GPIO=0)`);
          break;

        case CMD.BROADCAST_ALL_ON:
          device.gpioStates.fill(1);
          console.log(`      -> ALL ON (所有GPIO=1)`);
          break;

        case CMD.BROADCAST_LIGHTS_OFF:
          // lights are gpio 0-2
          for (let i = 0; i < 3; i++) device.gpioStates[i] = 0;
          console.log(`      -> LIGHTS OFF (GPIO 0-2 = 0)`);
          break;

        case CMD.BROADCAST_LIGHTS_ON:
          for (let i = 0; i < 3; i++) device.gpioStates[i] = 1;
          console.log(`      -> LIGHTS ON (GPIO 0-2 = 1)`);
          break;

        case CMD.EMERGENCY:
          console.log(`      -> EMERGENCY! (value=${value})`);
          break;

        case CMD.SET_AUTO_MODE:
          device.autoMode = value === 1;
          console.log(`      -> AUTO_MODE = ${device.autoMode ? 'ON' : 'OFF'}`);
          break;

        case CMD.FIRE_RESET:
          if (device.sensors) device.sensors.fire_status = 0;
          console.log(`      -> FIRE_RESET`);
          break;

        case CMD.HEARTBEAT:
          // 发送心跳响应
          const hbBuf = buildResponsePacket(CMD.HEARTBEAT, device.deviceId, 0, 0);
          client.publish(`xiaozhi/iot/heartbeat/${device.mac}`, hbBuf);
          console.log(`      -> 心跳已回复`);
          return;

        case CMD.ANNOUNCE_V2:
          // 重新发送 announce
          client.publish(`xiaozhi/iot/announce/${device.mac}`, announceBuf, { qos: 1 });
          console.log(`      -> 重发 ANNOUNCE_V2`);
          return;

        default:
          console.log(`      -> 未知命令 0x${cmd.toString(16).toUpperCase()}`);
          break;
      }

      // 发送响应
      const respBuf = buildResponsePacket(responseCmd, device.deviceId, respGpio, respVal);
      client.publish(`xiaozhi/iot/resp/${device.mac}`, respBuf, { qos: 1 });
    });

    // 启动心跳定时器（30秒）
    setInterval(() => {
      const hbBuf = buildResponsePacket(CMD.HEARTBEAT, device.deviceId, 0, 0);
      client.publish(`xiaozhi/iot/heartbeat/${device.mac}`, hbBuf);
    }, 30000);
  }

  console.log('');
  console.log('所有设备已就绪，等待命令...');
  console.log('按 Ctrl+C 退出');

  // 优雅退出
  process.on('SIGINT', () => {
    console.log('\n模拟器退出');
    for (const c of clients) {
      c.client.end();
    }
    process.exit(0);
  });
}

runSimulator().catch((err) => {
  console.error(`模拟器错误: ${err.message}`);
  process.exit(1);
});
