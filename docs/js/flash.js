'use strict';

// ========== CONSTANTS ==========
const BAUDRATE = 38400;
const GITHUB_REPO = 'EthanYan6/uv-k1-k5v3-firmware-custom';

const MSG_DEV_INFO_REQ     = 0x0514;
const MSG_DEV_INFO_RESP    = 0x0515;
const MSG_NOTIFY_DEV_INFO  = 0x0518;
const MSG_PROG_FW          = 0x0519;
const MSG_PROG_FW_RESP     = 0x051A;
const MSG_READ_EEPROM      = 0x051B;
const MSG_READ_EEPROM_RESP = 0x051C;
const MSG_WRITE_EEPROM     = 0x051D;
const MSG_WRITE_EEPROM_RESP= 0x051E;
const MSG_SPI_FLASH_READ   = 0x051F;
const MSG_SPI_FLASH_READ_RESP  = 0x0520;
const MSG_SPI_FLASH_WRITE  = 0x0521;
const MSG_SPI_FLASH_WRITE_RESP = 0x0522;
const MSG_NOTIFY_BL_VER    = 0x0530;
const MSG_REBOOT           = 0x05DD;

const OBFUS_TBL = new Uint8Array([
  0x16, 0x6c, 0x14, 0xe6, 0x2e, 0x91, 0x0d, 0x40,
  0x21, 0x35, 0xd5, 0x40, 0x13, 0x03, 0xe9, 0x80
]);

const CN_FONT_FLASH_BASE  = 0x010200;
const CN_FONT_VERSION_OFFSET = 20825;
const CN_FONT_VERSION     = 2;
const SPI_CHUNK_SIZE      = 48;
const CALIB_SIZE          = 512;
const CALIB_CHUNK         = 16;

// ========== STATE ==========
let port = null, reader = null, writer = null;
let firmwareData = null, fontData = null, calibData = null;
let readBuffer = [], isReading = false;
let isFlashing = false, isFontFlashing = false, isDumping = false, isRestoring = false;

// ========== UI ==========
const $ = id => document.getElementById(id);

// ========== TABS ==========
document.querySelectorAll('.tab').forEach(tab => {
  tab.addEventListener('click', () => {
    document.querySelectorAll('.tab').forEach(t => { t.classList.remove('active'); t.setAttribute('aria-selected','false'); });
    document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
    tab.classList.add('active');
    tab.setAttribute('aria-selected','true');
    $(tab.dataset.tab + '-content').classList.add('active');
  });
});

// ========== LOG ==========
function log(msg, type='') {
  const el = document.createElement('div');
  el.className = 'log-entry ' + type;
  el.textContent = `[${new Date().toLocaleTimeString()}] ${msg}`;
  const logDiv = $('log');
  logDiv.appendChild(el);
  logDiv.scrollTop = logDiv.scrollHeight;
  console.log(msg);
}

$('logToggle').addEventListener('click', () => {
  const logDiv = $('log');
  logDiv.classList.toggle('visible');
  $('logToggle').textContent = logDiv.classList.contains('visible') ? '隐藏日志' : '显示日志';
});

function updateProgress(pct) {
  const r = Math.round(pct);
  $('progressFill').style.width = r + '%';
  $('progressLabel').textContent = r + '%';
}

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

// ========== PROTOCOL ==========
function createMessage(msgType, dataLen) {
  const msg = new Uint8Array(4 + dataLen);
  new DataView(msg.buffer).setUint16(0, msgType, true);
  new DataView(msg.buffer).setUint16(2, dataLen, true);
  return msg;
}

async function sendMessage(msg) {
  await writer.write(makePacket(msg));
}

function makePacket(msg) {
  let msgLen = msg.length;
  if (msgLen % 2) msgLen++;
  const buf = new Uint8Array(8 + msgLen);
  const v = new DataView(buf.buffer);
  v.setUint16(0, 0xCDAB, true);
  v.setUint16(2, msgLen, true);
  for (let i = 0; i < msg.length; i++) buf[4 + i] = msg[i];
  v.setUint16(4 + msgLen, calcCRC(buf, 4, msgLen), true);
  v.setUint16(6 + msgLen, 0xBADC, true);
  obfuscate(buf, 4, 2 + msgLen);
  return buf;
}

function fetchMessage(buf) {
  if (buf.length < 8) return null;
  let pb = -1;
  for (let i = 0; i < buf.length - 1; i++) {
    if (buf[i] === 0xab && buf[i+1] === 0xcd) { pb = i; break; }
  }
  if (pb === -1) { buf.length = 0; return null; }
  if (buf.length - pb < 8) return null;
  const msgLen = (buf[pb+3] << 8) | buf[pb+2];
  const pe = pb + 6 + msgLen;
  if (buf.length < pe + 2) return null;
  if (buf[pe] !== 0xdc || buf[pe+1] !== 0xba) { buf.splice(0, pb+2); return null; }
  const msgBuf = new Uint8Array(msgLen + 2);
  for (let i = 0; i < msgLen + 2; i++) msgBuf[i] = buf[pb + 4 + i];
  obfuscate(msgBuf, 0, msgLen + 2);
  const msgType = new DataView(msgBuf.buffer).getUint16(0, true);
  buf.splice(0, pe + 2);
  return { msgType, data: msgBuf.slice(4) };
}

function obfuscate(buf, off, size) {
  for (let i = 0; i < size; i++) buf[off+i] ^= OBFUS_TBL[i % 16];
}

function calcCRC(buf, off, size) {
  let CRC = 0;
  for (let i = 0; i < size; i++) {
    CRC ^= (buf[off+i] & 0xff) << 8;
    for (let j = 0; j < 8; j++)
      CRC = (CRC & 0x8000) ? ((CRC << 1) ^ 0x1021) & 0xffff : (CRC << 1) & 0xffff;
  }
  return CRC;
}

function hex(arr) { return Array.from(arr).map(b => b.toString(16).padStart(2,'0')).join(' '); }

// ========== SERIAL ==========
async function connect() {
  log('请求串口...', 'info');
  port = await navigator.serial.requestPort();
  await port.open({ baudRate: BAUDRATE });
  reader = port.readable.getReader();
  writer = port.writable.getWriter();
  isReading = true;
  readLoop();
  await sleep(500);
  log('已连接', 'success');
}

async function disconnect() {
  isReading = false;
  if (reader) { try { await reader.cancel(); } catch{} reader.releaseLock(); reader = null; }
  if (writer) { try { await writer.close(); } catch{} writer = null; }
  if (port) { try { await port.close(); } catch{} port = null; }
  log('已断开', 'info');
}

async function readLoop() {
  try {
    while (isReading && reader) {
      const { value, done } = await reader.read();
      if (done) break;
      if (value?.length) readBuffer.push(...value);
    }
  } catch(e) { if (isReading) log('读取错误: ' + e.message, 'error'); }
}

async function waitForMsg(msgType, timeout=300) {
  for (let i = 0; i < timeout; i++) {
    await sleep(10);
    const msg = fetchMessage(readBuffer);
    if (!msg) continue;
    if (msg.msgType === MSG_NOTIFY_DEV_INFO) continue;
    if (msg.msgType === msgType) return msg;
  }
  return null;
}

async function waitForDeviceInfo() {
  let acc = 0, lastTs = 0;
  log('等待设备...', 'info');
  for (let t = 0; t < 500; t++) {
    await sleep(10);
    const msg = fetchMessage(readBuffer);
    if (!msg || msg.msgType !== MSG_NOTIFY_DEV_INFO) continue;
    const now = Date.now();
    if (lastTs > 0) {
      const dt = now - lastTs;
      if (dt >= 5 && dt <= 1000) { acc++; if (acc >= 5) {
        const uid = msg.data.slice(0, 16);
        let blEnd = -1;
        for (let i = 16; i < 32; i++) { if (msg.data[i] === 0) { blEnd = i; break; } }
        if (blEnd === -1) blEnd = 32;
        const blVer = new TextDecoder().decode(msg.data.slice(16, blEnd));
        log('UID: ' + hex(uid), 'info');
        log('Bootloader: ' + blVer, 'info');
        return { uid, blVersion: blVer };
      }} else { acc = 0; }
    }
    lastTs = now;
  }
  throw new Error('超时：未检测到设备');
}

async function handshake(blVersion) {
  let acc = 0;
  while (acc < 3) {
    await sleep(50);
    const msg = fetchMessage(readBuffer);
    if (msg && msg.msgType === MSG_NOTIFY_DEV_INFO) {
      const m = createMessage(MSG_NOTIFY_BL_VER, 4);
      const b = new TextEncoder().encode(blVersion.substring(0, 4));
      for (let i = 0; i < Math.min(b.length, 4); i++) m[4+i] = b[i];
      await sendMessage(m);
      acc++;
      await sleep(50);
    }
  }
  await sleep(200);
  readBuffer = [];
}

// ========== FIRMWARE FLASH ==========
$('firmwareFile').addEventListener('change', e => {
  const file = e.target.files?.[0];
  if (!file) return;
  const fr = new FileReader();
  fr.onload = ev => {
    firmwareData = new Uint8Array(ev.target.result);
    $('fileName').textContent = file.name + ' (' + firmwareData.length + ' bytes)';
    $('fileName').classList.add('has-file');
    $('fileLabel').classList.add('has-file');
    log('固件已加载: ' + file.name + ' (' + firmwareData.length + ' bytes)', 'success');
    $('flashBtn').disabled = false;
  };
  fr.readAsArrayBuffer(file);
});

$('flashBtn').addEventListener('click', async () => {
  if (!firmwareData || isFlashing) return;
  isFlashing = true;
  $('flashBtn').disabled = true;
  $('progressContainer').style.display = 'block';
  updateProgress(0);
  try {
    if (!port) await connect();
    readBuffer = [];
    await sleep(1000);
    const dev = await waitForDeviceInfo();
    await handshake(dev.blVersion);
    log('开始刷入固件...', 'info');
    const pageCount = Math.ceil(firmwareData.length / 256);
    const ts = Date.now() & 0xffffffff;
    let page = 0, retry = 0;
    while (page < pageCount) {
      updateProgress((page / pageCount) * 100);
      const msg = createMessage(MSG_PROG_FW, 268);
      const v = new DataView(msg.buffer);
      v.setUint32(4, ts, true);
      v.setUint16(8, page, true);
      v.setUint16(10, pageCount, true);
      const off = page * 256;
      const len = Math.min(256, firmwareData.length - off);
      for (let i = 0; i < len; i++) msg[16+i] = firmwareData[off+i];
      await sendMessage(msg);
      let ok = false;
      for (let i = 0; i < 300 && !ok; i++) {
        await sleep(10);
        const resp = fetchMessage(readBuffer);
        if (!resp || resp.msgType === MSG_NOTIFY_DEV_INFO) continue;
        if (resp.msgType === MSG_PROG_FW_RESP) {
          const rv = new DataView(resp.data.buffer);
          const rp = rv.getUint16(4, true);
          const err = rv.getUint16(6, true);
          if (rp !== page) continue;
          if (err !== 0) { retry++; if (retry > 3) throw new Error('页面 ' + page + ' 错误: ' + err); break; }
          ok = true; retry = 0;
          if ((page+1) % 20 === 0 || page === pageCount-1) log('页面 ' + (page+1) + '/' + pageCount, 'success');
        }
      }
      if (ok) page++;
      else { retry++; if (retry > 3) throw new Error('页面 ' + page + ' 超时'); }
    }
    updateProgress(100);
    log('固件刷入完成！', 'success');
  } catch(e) {
    log('错误: ' + e.message, 'error');
  } finally {
    isFlashing = false;
    $('flashBtn').disabled = !firmwareData;
    if (port) await disconnect();
    setTimeout(() => { $('progressContainer').style.display = 'none'; updateProgress(0); }, 1000);
  }
});

// ========== FETCH LATEST FIRMWARE ==========
$('fetchLatestBtn').addEventListener('click', async () => {
  const btn = $('fetchLatestBtn');
  btn.disabled = true;
  btn.textContent = '正在获取...';
  try {
    const res = await fetch(`https://api.github.com/repos/${GITHUB_REPO}/releases/latest`);
    if (!res.ok) throw new Error('GitHub API: ' + res.status);
    const release = await res.json();
    const binAsset = release.assets.find(a => a.name.endsWith('.bin'));
    if (!binAsset) throw new Error('未找到 .bin 文件');
    $('fwReleaseInfo').style.display = 'block';
    $('fwReleaseInfo').innerHTML =
      `<span class="fw-name">${release.tag_name}</span> &middot; ` +
      `<span class="fw-size">${(binAsset.size/1024).toFixed(1)} KB</span> &middot; ` +
      `<span class="fw-date">${new Date(release.published_at).toLocaleDateString()}</span>`;
    log('正在下载: ' + binAsset.name, 'info');
    const binRes = await fetch(binAsset.browser_download_url);
    if (!binRes.ok) throw new Error('下载失败: ' + binRes.status);
    const buf = await binRes.arrayBuffer();
    firmwareData = new Uint8Array(buf);
    $('fileName').textContent = '✓ ' + binAsset.name + ' (' + firmwareData.length + ' bytes)';
    $('fileName').classList.add('has-file');
    $('fileLabel').classList.add('has-file');
    log('固件已下载: ' + binAsset.name + ' (' + firmwareData.length + ' bytes)', 'success');
    $('flashBtn').disabled = false;
    $('flashBtn').textContent = '刷入固件';
  } catch(e) {
    log('获取失败: ' + e.message, 'error');
  } finally {
    btn.disabled = false;
    btn.textContent = '从 GitHub 拉取最新固件';
  }
});

// ========== FONT FLASH ==========
$('fontFile').addEventListener('change', e => {
  const file = e.target.files?.[0];
  if (!file) return;
  const fr = new FileReader();
  fr.onload = ev => {
    fontData = new Uint8Array(ev.target.result);
    $('fontFileName').textContent = file.name + ' (' + fontData.length + ' bytes)';
    $('fontFileName').classList.add('has-file');
    $('fontFileLabel').classList.add('has-file');
    log('字库已加载: ' + file.name + ' (' + fontData.length + ' bytes)', 'success');
    $('fontFlashBtn').disabled = false;
  };
  fr.readAsArrayBuffer(file);
});

$('fetchFontBtn').addEventListener('click', async () => {
  const btn = $('fetchFontBtn');
  btn.disabled = true;
  btn.textContent = '正在加载...';
  try {
    const res = await fetch('font/cn_font.bin');
    if (!res.ok) throw new Error('HTTP ' + res.status);
    const buf = await res.arrayBuffer();
    fontData = new Uint8Array(buf);
    $('fontInfo').style.display = 'block';
    $('fontInfo').innerHTML = `<span class="fw-name">cn_font.bin</span> &middot; <span class="fw-size">${(fontData.length/1024).toFixed(1)} KB</span> &middot; 648 字符`;
    $('fontFileName').textContent = 'cn_font.bin (' + fontData.length + ' bytes)';
    $('fontFileName').classList.add('has-file');
    $('fontFileLabel').classList.add('has-file');
    log('字库已加载: cn_font.bin (' + fontData.length + ' bytes)', 'success');
    $('fontFlashBtn').disabled = false;
  } catch(e) {
    log('加载失败: ' + e.message, 'error');
  } finally {
    btn.disabled = false;
    btn.textContent = '从仓库加载字库';
  }
});

$('fontFlashBtn').addEventListener('click', async () => {
  if (!fontData || isFontFlashing) return;
  isFontFlashing = true;
  $('fontFlashBtn').disabled = true;
  $('progressContainer').style.display = 'block';
  updateProgress(0);
  try {
    if (!port) await connect();
    readBuffer = [];
    await sleep(1000);

    // Detect device mode: send MSG_DEV_INFO_REQ and check response
    log('检测设备模式...', 'info');
    const ts = Date.now() & 0xffffffff;
    const reqMsg = createMessage(MSG_DEV_INFO_REQ, 4);
    new DataView(reqMsg.buffer).setUint32(4, ts, true);
    await sendMessage(reqMsg);

    let isFirmwareMode = false;
    for (let i = 0; i < 100; i++) {
      await sleep(10);
      const msg = fetchMessage(readBuffer);
      if (!msg) continue;
      if (msg.msgType === MSG_DEV_INFO_RESP) {
        isFirmwareMode = true;
        break;
      }
      if (msg.msgType === MSG_NOTIFY_DEV_INFO) {
        // Bootloader mode - keep waiting for possible DEV_INFO_RESP
      }
    }

    if (!isFirmwareMode) {
      throw new Error('设备处于 BOOT 模式，请先刷入固件并启动后再刷字库');
    }

    log('设备已运行自定义固件，开始刷入字库...', 'success');

    // Write font data in chunks via SPI Flash Write (0x0521)
    const totalChunks = Math.ceil(fontData.length / SPI_CHUNK_SIZE);
    let written = 0;

    for (let i = 0; i < fontData.length; i += SPI_CHUNK_SIZE) {
      const chunkLen = Math.min(SPI_CHUNK_SIZE, fontData.length - i);
      const addr = CN_FONT_FLASH_BASE + i;
      let ok = false;

      for (let retry = 0; retry < 3 && !ok; retry++) {
        if (retry > 0) {
          log('重试 @ 0x' + addr.toString(16) + ' (' + retry + ')', 'info');
          await sleep(200);
        }

        const msg = createMessage(MSG_SPI_FLASH_WRITE, 12 + chunkLen);
        const v = new DataView(msg.buffer);
        v.setUint32(4, addr, true);
        v.setUint16(8, chunkLen, true);
        v.setUint16(10, 0, true); // padding
        v.setUint32(12, ts, true);
        for (let j = 0; j < chunkLen; j++) msg[16+j] = fontData[i+j];

        await sendMessage(msg);
        const resp = await waitForMsg(MSG_SPI_FLASH_WRITE_RESP, 800);
        if (resp) ok = true;
      }

      if (!ok) throw new Error('写入超时 @ 0x' + addr.toString(16));

      written += chunkLen;
      updateProgress((written / fontData.length) * 100);
      if ((i / SPI_CHUNK_SIZE) % 10 === 0)
        log('已写入 ' + written + '/' + fontData.length + ' bytes', 'info');

      // Delay to avoid overwhelming the firmware during SPI Flash erase
      await sleep(50);
    }

    // Write version marker
    const verMsg = createMessage(MSG_SPI_FLASH_WRITE, 12 + 1);
    const vv = new DataView(verMsg.buffer);
    vv.setUint32(4, CN_FONT_FLASH_BASE + CN_FONT_VERSION_OFFSET, true);
    vv.setUint16(8, 1, true);
    vv.setUint16(10, 0, true);
    vv.setUint32(12, ts, true);
    verMsg[16] = CN_FONT_VERSION;
    await sendMessage(verMsg);
    const verResp = await waitForMsg(MSG_SPI_FLASH_WRITE_RESP, 100);
    if (!verResp) log('版本标记写入超时（可能固件不支持 SPI Flash 写入）', 'error');
    else log('版本标记已写入', 'success');

    // Verify: read back first 4 bytes
    const readMsg = createMessage(MSG_SPI_FLASH_READ, 12);
    const rv = new DataView(readMsg.buffer);
    rv.setUint32(4, CN_FONT_FLASH_BASE, true);
    rv.setUint16(8, 4, true);
    rv.setUint16(10, 0, true);
    rv.setUint32(12, ts, true);
    await sendMessage(readMsg);
    const readResp = await waitForMsg(MSG_SPI_FLASH_READ_RESP, 100);
    if (readResp) {
      const probe = new DataView(readResp.data.buffer);
      const w0 = probe.getUint16(8, true);
      const w1 = probe.getUint16(10, true);
      if (w0 === 0x1100 && w1 === 0x2100)
        log('验证通过：字库数据正确', 'success');
      else
        log('验证警告：首字节 0x' + w0.toString(16) + ' 0x' + w1.toString(16) + '（期望 0x1100 0x2100）', 'error');
    } else {
      log('验证跳过：读取超时', 'info');
    }

    updateProgress(100);
    log('字库刷入完成！共 ' + written + ' bytes', 'success');
  } catch(e) {
    log('错误: ' + e.message, 'error');
  } finally {
    isFontFlashing = false;
    $('fontFlashBtn').disabled = !fontData;
    if (port) await disconnect();
    setTimeout(() => { $('progressContainer').style.display = 'none'; updateProgress(0); }, 1000);
  }
});

// ========== DUMP CALIBRATION ==========
$('dumpBtn').addEventListener('click', async () => {
  if (isDumping) return;
  isDumping = true;
  $('dumpBtn').disabled = true;
  $('progressContainer').style.display = 'block';
  updateProgress(0);
  $('dumpDownload').style.display = 'none';
  try {
    if (!port) await connect();
    readBuffer = [];
    await sleep(1000);
    const dev = await waitForDeviceInfo();
    await handshake(dev.blVersion);
    log('导出校准数据...', 'info');
    const data = new Uint8Array(CALIB_SIZE);
    const ts = Date.now() & 0xffffffff;
    let offset = 0x1E00;
    for (let i = 0; i < CALIB_SIZE; i += CALIB_CHUNK) {
      updateProgress((i / CALIB_SIZE) * 100);
      const msg = createMessage(MSG_READ_EEPROM, 8);
      const v = new DataView(msg.buffer);
      v.setUint16(4, offset, true);
      v.setUint16(6, CALIB_CHUNK, true);
      v.setUint32(8, ts, true);
      await sendMessage(msg);
      let ok = false;
      for (let a = 0; a < 300 && !ok; a++) {
        await sleep(10);
        const resp = fetchMessage(readBuffer);
        if (!resp) continue;
        if (resp.msgType === MSG_READ_EEPROM_RESP) {
          const rv = new DataView(resp.data.buffer);
          if (rv.getUint16(0, true) === offset && resp.data[2] === CALIB_CHUNK) {
            for (let j = 0; j < CALIB_CHUNK; j++) data[i+j] = resp.data[4+j];
            ok = true;
            offset += CALIB_CHUNK;
          }
        }
      }
      if (!ok) throw new Error('读取失败 @ 0x' + offset.toString(16));
    }
    updateProgress(100);
    const blob = new Blob([data], { type: 'application/octet-stream' });
    $('dumpLink').href = URL.createObjectURL(blob);
    $('dumpDownload').style.display = 'block';
    log('校准数据导出完成', 'success');
  } catch(e) {
    log('错误: ' + e.message, 'error');
  } finally {
    isDumping = false;
    $('dumpBtn').disabled = false;
    if (port) await disconnect();
    setTimeout(() => { $('progressContainer').style.display = 'none'; updateProgress(0); }, 800);
  }
});

// ========== RESTORE CALIBRATION ==========
$('calibFile').addEventListener('change', e => {
  const file = e.target.files?.[0];
  if (!file) return;
  const fr = new FileReader();
  fr.onload = ev => {
    const buf = new Uint8Array(ev.target.result);
    if (buf.length !== CALIB_SIZE) { log('文件大小错误: ' + buf.length + ' (需要 ' + CALIB_SIZE + ')', 'error'); return; }
    calibData = buf;
    $('calibFileName').textContent = file.name;
    $('calibFileName').classList.add('has-file');
    $('calibFileLabel').classList.add('has-file');
    log('校准文件已加载: ' + file.name, 'success');
    $('restoreBtn').disabled = false;
  };
  fr.readAsArrayBuffer(file);
});

$('restoreBtn').addEventListener('click', async () => {
  if (!calibData || isRestoring) return;
  isRestoring = true;
  $('restoreBtn').disabled = true;
  $('progressContainer').style.display = 'block';
  updateProgress(0);
  try {
    if (!port) await connect();
    readBuffer = [];
    await sleep(1000);
    const dev = await waitForDeviceInfo();
    await handshake(dev.blVersion);
    log('恢复校准数据...', 'info');
    const ts = Date.now() & 0xffffffff;
    let offset = 0x1E00;
    for (let i = 0; i < CALIB_SIZE; i += CALIB_CHUNK) {
      updateProgress((i / CALIB_SIZE) * 100);
      const msg = createMessage(MSG_WRITE_EEPROM, 24);
      const v = new DataView(msg.buffer);
      v.setUint16(4, offset, true);
      v.setUint16(6, CALIB_CHUNK, true);
      msg[7] = 1;
      v.setUint32(8, ts, true);
      for (let j = 0; j < CALIB_CHUNK; j++) msg[12+j] = calibData[i+j];
      await sendMessage(msg);
      let ok = false;
      for (let a = 0; a < 300 && !ok; a++) {
        await sleep(10);
        const resp = fetchMessage(readBuffer);
        if (!resp) continue;
        if (resp.msgType === MSG_WRITE_EEPROM_RESP) {
          if (new DataView(resp.data.buffer).getUint16(0, true) === offset) { ok = true; offset += CALIB_CHUNK; }
        }
      }
      if (!ok) throw new Error('写入失败 @ 0x' + offset.toString(16));
    }
    updateProgress(100);
    log('校准数据恢复完成！正在重启...', 'success');
    await sendMessage(createMessage(MSG_REBOOT, 0));
    await sleep(500);
    log('设备已重启', 'success');
  } catch(e) {
    log('错误: ' + e.message, 'error');
  } finally {
    isRestoring = false;
    $('restoreBtn').disabled = !calibData;
    if (port) await disconnect();
    setTimeout(() => { $('progressContainer').style.display = 'none'; updateProgress(0); }, 800);
  }
});

// ========== CAPABILITY CHECK ==========
if (!('serial' in navigator)) {
  log('浏览器不支持 Web Serial API，请使用 Chrome/Edge/Opera', 'error');
  $('flashBtn').disabled = true;
  $('fontFlashBtn').disabled = true;
  $('dumpBtn').disabled = true;
  $('restoreBtn').disabled = true;
}
