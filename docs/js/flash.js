'use strict';

// ========== CONSTANTS ==========
const BAUDRATE = 38400;
const GITHUB_REPO = 'EthanYan6/Dondji';

/** 若把 Dondji.fusion.bin 放在与页面同源的路径（如 gh-pages 的 firmware/Dondji.fusion.bin），可优先从此加载；留空则仅用下方代理拉取 GitHub */
const FIRMWARE_SAME_ORIGIN_REL = '';

/** GitHub releases/download 响应不带 Access-Control-Allow-Origin，浏览器不能直接 fetch 原始链接，只能走代理或同源文件 */
const CORS_PROXIES = [
  url => 'https://corsproxy.io/?' + encodeURIComponent(url),
  url => 'https://api.allorigins.win/raw?url=' + encodeURIComponent(url),
  url => 'https://ghproxy.net/https://' + url.replace(/^https?:\/\//, ''),
];

function buildFirmwareDownloadCandidates(browserDownloadUrl) {
  const candidates = [];
  const trimmedLocal = typeof FIRMWARE_SAME_ORIGIN_REL === 'string' ? FIRMWARE_SAME_ORIGIN_REL.trim() : '';
  if (trimmedLocal.length > 0) {
    const sameOriginAbsoluteUrl = new URL(trimmedLocal, window.location.href).href;
    candidates.push(sameOriginAbsoluteUrl);
  }
  let proxyIndex = 0;
  for (; proxyIndex < CORS_PROXIES.length; proxyIndex++) {
    const proxyTransform = CORS_PROXIES[proxyIndex];
    const proxiedUrl = proxyTransform(browserDownloadUrl);
    candidates.push(proxiedUrl);
  }
  return candidates;
}

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

/** 校准区在 EEPROM 中的起始地址：与 UVTools2 一致，v5.0.0 起为 0xB000，更早固件为 0x1E00（由导出/恢复时请求设备信息解析） */
let calibEepromBase = 0x1E00;

// ========== STATE ==========
let port = null, reader = null, writer = null;
let firmwareData = null, fontData = null, calibData = null;
let readBuffer = [], isReading = false;
let isFlashing = false, isFontFlashing = false, isDumping = false, isRestoring = false;
let isWritefreqBusy = false;

// ========== UI ==========
const $ = id => document.getElementById(id);

// ========== TABS ==========
function syncWritefreqFullLayoutClass() {
  const activeTab = document.querySelector('.tab.active');
  let isWritefreq = false;
  if (activeTab) {
    const tabId = activeTab.dataset.tab;
    if (tabId === 'writefreq') {
      isWritefreq = true;
    }
  }
  document.body.classList.toggle('writefreq-full', isWritefreq);
}

/**
 * 写频页把日志（及进度条）挂到 #logDockWritefreq，与表格同区显示；
 * 若只移日志、不移进度条，则进度条会留在 main 底部，看起来仍在「表格下方」。
 */
function syncLogDockPlacement() {
  const logSection = $('logContainerSection');
  const progressSection = $('progressContainer');
  const dockWf = $('logDockWritefreq');
  const anchor = $('logAnchorDefault');
  if (!logSection || !anchor) {
    return;
  }
  if (!progressSection) {
    return;
  }
  const activeTab = document.querySelector('.tab.active');
  let isWritefreq = false;
  if (activeTab && activeTab.dataset.tab === 'writefreq') {
    isWritefreq = true;
  }
  if (isWritefreq && dockWf) {
    dockWf.appendChild(logSection);
    dockWf.appendChild(progressSection);
    return;
  }
  anchor.after(logSection);
  logSection.after(progressSection);
}

document.querySelectorAll('.tab').forEach(tab => {
  tab.addEventListener('click', () => {
    document.querySelectorAll('.tab').forEach(t => { t.classList.remove('active'); t.setAttribute('aria-selected','false'); });
    document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
    tab.classList.add('active');
    tab.setAttribute('aria-selected','true');
    $(tab.dataset.tab + '-content').classList.add('active');
    syncWritefreqFullLayoutClass();
    syncLogDockPlacement();
  });
});
syncWritefreqFullLayoutClass();
syncLogDockPlacement();

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

/** 根据 MSG_DEV_INFO_RESP 中的 ASCII 设备字符串解析固件主版本，设置全局 calibEepromBase（对齐 armel UVTools2） */
function applyCalibBaseFromDeviceInfo(deviceInfoPayload) {
  let asciiLine = '';
  let idx = 0;
  for (; idx < deviceInfoPayload.length; idx++) {
    const b = deviceInfoPayload[idx];
    if (b === 0x00 || b === 0xff) {
      break;
    }
    if (b >= 32 && b < 127) {
      asciiLine += String.fromCharCode(b);
    }
  }
  if (asciiLine.length > 0) {
    log('设备信息: ' + asciiLine, 'success');
    const versionMatch = asciiLine.match(/v(\d+\.\d+\.\d+)/);
    if (versionMatch) {
      const verStr = versionMatch[1];
      const parts = verStr.split('.');
      const major = parseInt(parts[0], 10);
      if (major >= 5) {
        calibEepromBase = 0xB000;
        log('固件 v' + verStr + '：校准区基址 0xB000', 'info');
      } else {
        calibEepromBase = 0x1E00;
        log('固件 v' + verStr + '：校准区基址 0x1E00', 'info');
      }
    }
    return;
  }
  let hexLine = '';
  let hi = 0;
  const hexLimit = Math.min(deviceInfoPayload.length, 40);
  for (; hi < hexLimit; hi++) {
    hexLine += deviceInfoPayload[hi].toString(16).padStart(2, '0').toUpperCase() + ' ';
  }
  log('设备信息(hex): ' + hexLine, 'info');
}

/** 导出/恢复校准用：发 DEV_INFO_REQ，等设备应答（运行中的固件协议），不使用 Bootloader 的 NOTIFY 检测 */
async function requestDeviceInfoForCalib() {
  calibEepromBase = 0x1E00;
  log('正在请求设备信息（校准）...', 'info');
  const sessionTimestamp = Date.now() & 0xffffffff;
  const req = createMessage(MSG_DEV_INFO_REQ, 4);
  const reqView = new DataView(req.buffer);
  reqView.setUint32(4, sessionTimestamp, true);
  await sendMessage(req);
  let tick = 0;
  for (; tick < 500; tick++) {
    await sleep(10);
    const resp = fetchMessage(readBuffer);
    if (!resp) {
      continue;
    }
    log('收到消息: 0x' + resp.msgType.toString(16).padStart(4, '0'), 'info');
    if (resp.msgType === MSG_DEV_INFO_RESP) {
      applyCalibBaseFromDeviceInfo(resp.data);
      log('设备已就绪（校准会话）', 'success');
      const out = { timestamp: sessionTimestamp };
      return out;
    }
  }
  throw new Error('超时：未收到设备信息（请开机进入正常工作界面再试，勿停在纯 Bootloader 刷机界面）');
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
    const binAsset = release.assets.find(a => a.name === 'Dondji.fusion.bin');
    if (!binAsset) throw new Error('未找到 Dondji.fusion.bin 文件');
    $('fwReleaseInfo').style.display = 'block';
    $('fwReleaseInfo').innerHTML =
      `<span class="fw-name">${release.tag_name}</span> &middot; ` +
      `<span class="fw-size">${(binAsset.size/1024).toFixed(1)} KB</span> &middot; ` +
      `<span class="fw-date">${new Date(release.published_at).toLocaleDateString()}</span>`;
    log('正在下载: ' + binAsset.name, 'info');
    const candidateUrls = buildFirmwareDownloadCandidates(binAsset.browser_download_url);
    let binRes = null;
    let lastErr = null;
    let attemptIndex = 0;
    for (; attemptIndex < candidateUrls.length; attemptIndex++) {
      const tryUrl = candidateUrls[attemptIndex];
      try {
        const responseTry = await fetch(tryUrl);
        if (responseTry.ok) {
          binRes = responseTry;
          break;
        }
        lastErr = new Error('下载失败: HTTP ' + responseTry.status);
      } catch (fetchErr) {
        lastErr = fetchErr;
        binRes = null;
      }
    }
    if (!binRes || !binRes.ok) throw lastErr || new Error('下载失败（GitHub 发布文件需经代理或同源镜像拉取）');
    const buf = await binRes.arrayBuffer();
    firmwareData = new Uint8Array(buf);
    $('fileName').textContent = '✓ ' + binAsset.name + ' (' + firmwareData.length + ' bytes)';
    $('fileName').classList.add('has-file');
    $('fileLabel').classList.add('has-file');
    log('固件已下载: ' + binAsset.name + ' (' + firmwareData.length + ' bytes)', 'success');
    $('flashBtn').disabled = false;
    $('flashBtn').textContent = '刷入固件';
  } catch(e) {
    log('获取失败: ' + e.message + '（请检查网络，可能需要开启代理/VPN）', 'error');
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
    const calibSession = await requestDeviceInfoForCalib();
    log('导出校准数据...', 'info');
    const data = new Uint8Array(CALIB_SIZE);
    const ts = calibSession.timestamp;
    let offset = calibEepromBase;
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
    const calibSession = await requestDeviceInfoForCalib();
    log('恢复校准数据...', 'info');
    const ts = calibSession.timestamp;
    let offset = calibEepromBase;
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
  const wfRead = $('writefreqReadBtn');
  const wfWrite = $('writefreqWriteBtn');
  if (wfRead) wfRead.disabled = true;
  if (wfWrite) wfWrite.disabled = true;
}

// ========== VERSION TIMELINE ==========
(async function loadTimeline() {
  const container = $('timeline');
  try {
    const resp = await fetch(`https://api.github.com/repos/${GITHUB_REPO}/releases?per_page=20`);
    if (!resp.ok) throw new Error('HTTP ' + resp.status);
    const releases = await resp.json();
    if (!releases.length) { container.innerHTML = '<div class="timeline-loading">暂无发布版本</div>'; return; }

    container.innerHTML = releases.map(r => {
      const date = new Date(r.published_at).toLocaleDateString('zh-CN', { year: 'numeric', month: '2-digit', day: '2-digit' });
      const body = (r.body || '').replace(/<[^>]*>/g, '').replace(/\n/g, '<br>');
      const pre = r.prerelease ? '<span class="timeline-prerelease">Pre-release</span>' : '';
      return `<div class="timeline-item">
        <div class="timeline-dot"></div>
        <div class="timeline-header">
          <a class="timeline-tag" href="${r.html_url}" target="_blank">${r.tag_name}</a>
          ${pre}
          <span class="timeline-date">${date}</span>
        </div>
        ${r.name ? `<div class="timeline-name">${r.name}</div>` : ''}
        ${body ? `<div class="timeline-body">${body}</div>` : ''}
      </div>`;
    }).join('');
  } catch (e) {
    container.innerHTML = `<div class="timeline-loading">加载失败: ${e.message}</div>`;
  }
})();

// ========== WRITE FREQUENCY (MR CHANNELS, SPI FLASH) ==========
// 地址与固件一致：MR 块 channel*16；英文名 0x004000+ch*16；中文名 0x020000+ch*16（settings.c / settings.h）

/** 本工具仅读写设备 MR 的前 N 槽（Flash 下标 0 … N-1，界面 CH1 … CHN）；大于 N 的 MR 槽读写时不触碰 */
const WRITE_FREQ_MR_MAX = 200;
/** 写频表导出文件名的前缀（如 Dondji_channels_export.xlsx） */
const WRITE_FREQ_EXPORT_FILE_PREFIX = 'Dondji';
/** 表格第 1 行对应的 MR 信道号（与 WRITE_FREQ_MR_MAX 一致，当前为 1–200）；默认 1；Excel 导入时取首行「信道号」 */
let writefreqTableBaseChannel = 1;
/** 内存中 N 条信道数据；界面仅渲染一页 WRITE_FREQ_PAGE_SIZE 行 */
const WRITE_FREQ_PAGE_SIZE = 10;
let writefreqPageIndex = 0;
let writefreqRowsData = null;
/** SortableJS 实例（写频表格行拖拽） */
let writefreqSortableInstance = null;
const WRITE_FREQ_EN_NAME_MAX_BYTES = 10;
const WRITE_FREQ_CN_NAME_MAX_BYTES = 10;
const WRITE_FREQ_SPI_MAX_CHUNK = 120;
/** waitForMsg 循环次数，×10ms 为大约最长等待（例 120 ≈ 1.2s） */
const WRITE_FREQ_SPI_READ_WAIT_ITERATIONS = 120;
const WRITE_FREQ_SPI_READ_RETRIES = 5;

const WRITE_FREQ_ADDR_EN_BASE = 0x004000;
const WRITE_FREQ_ADDR_CN_BASE = 0x020000;
/** 固件/擦除区常用 0xFFFFFFFF 表示无有效频率；直接换算成 MHz 会显示 4294.967295，读表应留空 */
const WRITE_FREQ_HZ_UNSET = 0xffffffff;
/** 与固件 VFO / MR 一致：Flash 中 uint32 频率步长为 10 Hz（见 App/frequencies.c frequencyBandTable、radio.c info.Frequency → BK4819） */
const WRITE_FREQ_STORE_STEP_HZ = 10;

// 与 App/dcs.c、App/radio.c 一致
const WF_CTCSS_OPTIONS = [
  670, 693, 719, 744, 770, 797, 825, 854, 885, 915,
  948, 974, 1000, 1035, 1072, 1109, 1148, 1188, 1230, 1273,
  1318, 1365, 1413, 1462, 1514, 1567, 1598, 1622, 1655, 1679,
  1713, 1738, 1773, 1799, 1835, 1862, 1899, 1928, 1966, 1995,
  2035, 2065, 2107, 2181, 2257, 2291, 2336, 2418, 2503, 2541
];

const WF_DCS_OPTIONS = [
  0x0013, 0x0015, 0x0016, 0x0019, 0x001a, 0x001e, 0x0023, 0x0027,
  0x0029, 0x002b, 0x002c, 0x0035, 0x0039, 0x003a, 0x003b, 0x003c,
  0x004c, 0x004d, 0x004e, 0x0052, 0x0055, 0x0059, 0x005a, 0x005c,
  0x0063, 0x0065, 0x006a, 0x006d, 0x006e, 0x0072, 0x0075, 0x007a,
  0x007c, 0x0085, 0x008a, 0x0093, 0x0095, 0x0096, 0x00a3, 0x00a4,
  0x00a5, 0x00a6, 0x00a9, 0x00aa, 0x00ad, 0x00b1, 0x00b3, 0x00b5,
  0x00b6, 0x00b9, 0x00bc, 0x00c6, 0x00c9, 0x00cd, 0x00d5, 0x00d9,
  0x00da, 0x00e3, 0x00e6, 0x00e9, 0x00ee, 0x00f4, 0x00f5, 0x00f9,
  0x0109, 0x010a, 0x010b, 0x0113, 0x0119, 0x011a, 0x0125, 0x0126,
  0x012a, 0x012c, 0x012d, 0x0132, 0x0134, 0x0135, 0x0136, 0x0143,
  0x0146, 0x014e, 0x0153, 0x0156, 0x015a, 0x0166, 0x0175, 0x0186,
  0x018a, 0x0194, 0x0197, 0x0199, 0x019a, 0x01ac, 0x01b2, 0x01b4,
  0x01c3, 0x01ca, 0x01d3, 0x01d9, 0x01da, 0x01dc, 0x01e3, 0x01ec
];

const WF_POWER_LABELS = ['', 'LOW 1', 'LOW 2', 'LOW 3', 'LOW 4', 'LOW 5', 'MID', 'HIGH'];
const WF_MOD_LABELS = ['FM', 'AM', 'USB'];

function wfFormatDcsMenuLabel(isInverted, index) {
  const raw = WF_DCS_OPTIONS[index];
  const oct = (raw & 0x1ff).toString(8).padStart(3, '0');
  if (isInverted) {
    return 'D' + oct + 'I';
  }
  return 'D' + oct + 'N';
}

/** 与下拉框「模拟亚音」选项文本一致，供导出 Excel、导入按文案匹配 */
function wfCtcssIndexToMenuLabel(ci) {
  const hz10 = WF_CTCSS_OPTIONS[ci];
  const whole = Math.floor(hz10 / 10);
  const frac = hz10 % 10;
  const labelText = String(whole) + '.' + String(frac) + 'Hz';
  return labelText;
}

function wfAppendCtcssSelectOptions(selectEl) {
  const z = document.createElement('option');
  z.value = '';
  z.textContent = 'OFF';
  selectEl.appendChild(z);
  let ci = 0;
  for (; ci < WF_CTCSS_OPTIONS.length; ci++) {
    const opt = document.createElement('option');
    opt.value = String(ci);
    opt.textContent = wfCtcssIndexToMenuLabel(ci);
    selectEl.appendChild(opt);
  }
}

function wfAppendDcsSelectOptions(selectEl) {
  const z2 = document.createElement('option');
  z2.value = '';
  z2.textContent = 'OFF';
  selectEl.appendChild(z2);
  let ni = 0;
  for (; ni < WF_DCS_OPTIONS.length; ni++) {
    const optN = document.createElement('option');
    optN.value = 'N:' + ni;
    optN.textContent = wfFormatDcsMenuLabel(false, ni);
    selectEl.appendChild(optN);
  }
  let ii = 0;
  for (; ii < WF_DCS_OPTIONS.length; ii++) {
    const optI = document.createElement('option');
    optI.value = 'I:' + ii;
    optI.textContent = wfFormatDcsMenuLabel(true, ii);
    selectEl.appendChild(optI);
  }
}

function wfDecodeChannelFields(bytes16) {
  const dv = new DataView(bytes16.buffer, bytes16.byteOffset, 16);
  const rxStored = dv.getUint32(0, true);
  const offsetStored = dv.getUint32(4, true);
  const rxCode = bytes16[8];
  const txCode = bytes16[9];
  const rxCt = bytes16[10] & 0x0f;
  const txCt = (bytes16[10] >> 4) & 0x0f;
  let offsetDir = bytes16[11] & 0x0f;
  if (offsetDir > 2) {
    offsetDir = 0;
  }
  let modulation = (bytes16[11] >> 4) & 0x0f;
  if (modulation > 2) {
    modulation = 0;
  }
  const d4 = bytes16[12];
  let power = 0;
  if (d4 !== 0xff) {
    power = (d4 >> 2) & 7;
  }
  const rxIsUnset = rxStored === WRITE_FREQ_HZ_UNSET;
  const offsetIsUnset = offsetStored === WRITE_FREQ_HZ_UNSET;
  let rxMHzStr;
  if (rxIsUnset) {
    rxMHzStr = '';
  } else {
    const rxTrueHz = rxStored * WRITE_FREQ_STORE_STEP_HZ;
    const rxMhz = rxTrueHz / 1e6;
    rxMHzStr = rxMhz.toFixed(6);
  }
  let offsetMHzStr;
  if (offsetIsUnset) {
    offsetMHzStr = '';
  } else {
    const offsetTrueHz = offsetStored * WRITE_FREQ_STORE_STEP_HZ;
    const offsetMhz = offsetTrueHz / 1e6;
    offsetMHzStr = offsetMhz.toFixed(6);
  }
  return {
    rxMHzStr,
    offsetMHzStr,
    offsetDir,
    modulation,
    power,
    rxToneType: rxCt,
    rxToneCode: rxCode,
    txToneType: txCt,
    txToneCode: txCode
  };
}

function wfMergePowerByte(old12, power7) {
  if (old12 === 0xff) {
    const defaultTxLock = 0x40;
    const defaultLow1Bits = 1 << 2;
    const base = defaultTxLock | defaultLow1Bits;
    const cleared = base & ~(7 << 2);
    const merged = cleared | ((power7 & 7) << 2);
    return merged & 0xff;
  }
  const cleared2 = old12 & ~(7 << 2);
  const merged2 = cleared2 | ((power7 & 7) << 2);
  return merged2 & 0xff;
}

function wfParseToneSide(ctcssVal, dcsVal, sideLabel) {
  const ctcssOn = ctcssVal !== '';
  const dcsOn = dcsVal !== '';
  if (ctcssOn && dcsOn) {
    const err = new Error(sideLabel + ' 不能同时选择模拟亚音与数字亚音');
    throw err;
  }
  if (ctcssOn) {
    const idx = Number.parseInt(ctcssVal, 10);
    if (!Number.isFinite(idx) || idx < 0 || idx >= WF_CTCSS_OPTIONS.length) {
      const err = new Error(sideLabel + ' 模拟亚音选项无效');
      throw err;
    }
    const out = { type: 1, code: idx };
    return out;
  }
  if (dcsOn) {
    if (dcsVal.startsWith('N:')) {
      const di = Number.parseInt(dcsVal.slice(2), 10);
      if (!Number.isFinite(di) || di < 0 || di >= WF_DCS_OPTIONS.length) {
        const err = new Error(sideLabel + ' 数字亚音(DCS)索引无效');
        throw err;
      }
      const outN = { type: 2, code: di };
      return outN;
    }
    if (dcsVal.startsWith('I:')) {
      const di2 = Number.parseInt(dcsVal.slice(2), 10);
      if (!Number.isFinite(di2) || di2 < 0 || di2 >= WF_DCS_OPTIONS.length) {
        const err = new Error(sideLabel + ' 数字亚音(DCS)索引无效');
        throw err;
      }
      const outI = { type: 3, code: di2 };
      return outI;
    }
    const err2 = new Error(sideLabel + ' 数字亚音格式无效（应为 OFF 或 N:索引 / I:索引）');
    throw err2;
  }
  const off = { type: 0, code: 0 };
  return off;
}

function wfFillRowFromBlock(tr, block16) {
  const decoded = wfDecodeChannelFields(block16);
  const rxIn = tr.querySelector('.wf-rx');
  if (rxIn) {
    rxIn.value = decoded.rxMHzStr;
  }
  const offIn = tr.querySelector('.wf-offset');
  if (offIn) {
    offIn.value = decoded.offsetMHzStr;
  }
  const sft = tr.querySelector('.wf-sft');
  if (sft) {
    sft.value = String(decoded.offsetDir);
  }
  const mod = tr.querySelector('.wf-mod');
  if (mod) {
    const mv = String(decoded.modulation);
    mod.value = mv;
  }
  const pw = tr.querySelector('.wf-power');
  if (pw) {
    if (decoded.power >= 1 && decoded.power <= 7) {
      pw.value = String(decoded.power);
    } else {
      pw.value = '';
    }
  }
  const rxCtEl = tr.querySelector('.wf-rx-ctcss');
  const rxDcEl = tr.querySelector('.wf-rx-dcs');
  const txCtEl = tr.querySelector('.wf-tx-ctcss');
  const txDcEl = tr.querySelector('.wf-tx-dcs');
  if (decoded.rxToneType === 1 && rxCtEl) {
    rxCtEl.value = String(decoded.rxToneCode);
  } else if (rxCtEl) {
    rxCtEl.value = '';
  }
  if (decoded.rxToneType === 2 && rxDcEl) {
    rxDcEl.value = 'N:' + decoded.rxToneCode;
  } else if (decoded.rxToneType === 3 && rxDcEl) {
    rxDcEl.value = 'I:' + decoded.rxToneCode;
  } else if (rxDcEl) {
    rxDcEl.value = '';
  }
  if (decoded.txToneType === 1 && txCtEl) {
    txCtEl.value = String(decoded.txToneCode);
  } else if (txCtEl) {
    txCtEl.value = '';
  }
  if (decoded.txToneType === 2 && txDcEl) {
    txDcEl.value = 'N:' + decoded.txToneCode;
  } else if (decoded.txToneType === 3 && txDcEl) {
    txDcEl.value = 'I:' + decoded.txToneCode;
  } else if (txDcEl) {
    txDcEl.value = '';
  }
}

/** rxStored/offsetStored：与固件相同的 uint32，单位为 WRITE_FREQ_STORE_STEP_HZ（10 Hz）一步 */
function wfMergeUserIntoBlock(original16, rxStored, offsetStored, offsetDir, modulation, power7, rxTone, txTone) {
  const out = new Uint8Array(original16);
  const dv = new DataView(out.buffer);
  dv.setUint32(0, rxStored >>> 0, true);
  dv.setUint32(4, offsetStored >>> 0, true);
  out[8] = rxTone.code & 0xff;
  out[9] = txTone.code & 0xff;
  const b10 = ((txTone.type & 0x0f) << 4) | (rxTone.type & 0x0f);
  out[10] = b10;
  const b11 = ((modulation & 0x0f) << 4) | (offsetDir & 0x0f);
  out[11] = b11;
  const old12 = original16[12];
  const new12 = wfMergePowerByte(old12, power7);
  out[12] = new12;
  return out;
}

async function spiFlashReadChunk(sessionTs, flashAddress, byteLength) {
  const addr = flashAddress >>> 0;
  const len = byteLength;
  if (len === 0 || len > WRITE_FREQ_SPI_MAX_CHUNK) {
    return null;
  }
  let attempt = 0;
  for (; attempt < WRITE_FREQ_SPI_READ_RETRIES; attempt++) {
    if (attempt > 0) {
      await sleep(80 + attempt * 40);
    }
    const req = createMessage(MSG_SPI_FLASH_READ, 12);
    const reqView = new DataView(req.buffer);
    reqView.setUint32(4, addr, true);
    reqView.setUint16(8, len, true);
    reqView.setUint16(10, 0, true);
    reqView.setUint32(12, sessionTs >>> 0, true);
    await sendMessage(req);
    const resp = await waitForMsg(MSG_SPI_FLASH_READ_RESP, WRITE_FREQ_SPI_READ_WAIT_ITERATIONS);
    if (!resp) {
      continue;
    }
    const respView = new DataView(resp.data.buffer, resp.data.byteOffset, resp.data.byteLength);
    const respAddr = respView.getUint32(0, true);
    const respLen = respView.getUint16(4, true);
    if (respAddr !== addr) {
      continue;
    }
    if (respLen !== len) {
      continue;
    }
    const payloadAvail = resp.data.length - 8;
    const copyLen = payloadAvail < len ? payloadAvail : len;
    const out = new Uint8Array(len);
    let copyIndex = 0;
    for (; copyIndex < copyLen; copyIndex++) {
      out[copyIndex] = resp.data[8 + copyIndex];
    }
    return out;
  }
  return null;
}

async function spiFlashWriteChunk(sessionTs, flashAddress, payload) {
  const addr = flashAddress >>> 0;
  const chunkLen = payload.length;
  if (chunkLen === 0 || chunkLen > WRITE_FREQ_SPI_MAX_CHUNK) {
    return false;
  }
  let retry = 0;
  let ok = false;
  for (; retry < 3 && !ok; retry++) {
    if (retry > 0) {
      await sleep(150);
    }
    const msg = createMessage(MSG_SPI_FLASH_WRITE, 12 + chunkLen);
    const v = new DataView(msg.buffer);
    v.setUint32(4, addr, true);
    v.setUint16(8, chunkLen, true);
    v.setUint16(10, 0, true);
    v.setUint32(12, sessionTs >>> 0, true);
    let bi = 0;
    for (; bi < chunkLen; bi++) {
      msg[16 + bi] = payload[bi];
    }
    await sendMessage(msg);
    const wr = await waitForMsg(MSG_SPI_FLASH_WRITE_RESP, 600);
    if (wr) {
      ok = true;
    }
  }
  return ok;
}

function writefreqTrimAsciiName(bytes) {
  const outChars = [];
  let i = 0;
  for (; i < WRITE_FREQ_EN_NAME_MAX_BYTES && i < bytes.length; i++) {
    const b = bytes[i];
    if (b === 0 || b === 0xff) {
      break;
    }
    if (b >= 32 && b <= 127) {
      outChars.push(String.fromCharCode(b));
    } else {
      break;
    }
  }
  let s = outChars.join('');
  while (s.length > 0 && s.charAt(s.length - 1) === ' ') {
    s = s.slice(0, -1);
  }
  return s;
}

function writefreqDecodeCnNameUtf8(bytes) {
  const len = Math.min(16, bytes.length);
  let end = 0;
  let ei = 0;
  for (; ei < len; ei++) {
    const b = bytes[ei];
    if (b === 0 || b === 0xff) {
      break;
    }
    end = ei + 1;
  }
  if (end === 0) {
    return '';
  }
  const slice = bytes.subarray(0, end);
  try {
    const text = new TextDecoder('utf-8', { fatal: true }).decode(slice);
    return text;
  } catch (e) {
    return '';
  }
}

/** 返回与 Flash 一致的 uint32：步长为 WRITE_FREQ_STORE_STEP_HZ（10 Hz），非标准 Hz */
function writefreqParseMHzOrThrow(label, text) {
  const trimmed = text.trim();
  if (trimmed === '') {
    const err = new Error(label + ' 不能为空');
    throw err;
  }
  const mhz = Number.parseFloat(trimmed);
  if (!Number.isFinite(mhz)) {
    const err = new Error(label + ' 不是有效频率数字');
    throw err;
  }
  const trueHz = mhz * 1e6;
  const stored = Math.round(trueHz / WRITE_FREQ_STORE_STEP_HZ);
  return stored;
}

/**
 * 按 UTF-8 字节截断，不在多字节字符中间切断。
 * 返回截断后字符串、是否截断、原始 UTF-8 字节长度。
 */
function writefreqTruncateUtf8ToMaxBytes(text, maxBytes) {
  const encoder = new TextEncoder();
  const encodedBytes = encoder.encode(text);
  const originalByteLength = encodedBytes.length;
  let resultText = text;
  let wasTruncated = false;
  if (originalByteLength <= maxBytes) {
    const resultOk = {
      text: resultText,
      wasTruncated: wasTruncated,
      originalByteLength: originalByteLength
    };
    return resultOk;
  }
  wasTruncated = true;
  let cutEnd = maxBytes;
  let foundBoundary = false;
  while (!foundBoundary) {
    if (cutEnd <= 0) {
      resultText = '';
      foundBoundary = true;
      break;
    }
    const byteAtCut = encodedBytes[cutEnd - 1];
    const isContinuationByte = (byteAtCut & 0xc0) === 0x80;
    if (!isContinuationByte) {
      const sliceBytes = encodedBytes.subarray(0, cutEnd);
      const decoder = new TextDecoder('utf-8');
      resultText = decoder.decode(sliceBytes);
      foundBoundary = true;
      break;
    }
    cutEnd = cutEnd - 1;
  }
  const resultTrunc = {
    text: resultText,
    wasTruncated: wasTruncated,
    originalByteLength: originalByteLength
  };
  return resultTrunc;
}

/** 有接收频率的信道：英文名、中文名按 10 字节（UTF-8）截断并写回 model */
function writefreqApplyAllChannelNameTruncations() {
  writefreqEnsureModelInit();
  const truncationWarnings = [];
  const startCh = writefreqGetBaseChannel();
  let rowIndex = 0;
  for (; rowIndex < WRITE_FREQ_MR_MAX; rowIndex++) {
    const fields = writefreqRowsData[rowIndex];
    const rxTrimmed = fields.rxText.trim();
    if (rxTrimmed === '') {
      continue;
    }
    const channelNumber = startCh + rowIndex;
    const rowLabel = '第 ' + channelNumber + ' 信道';
    const enResult = writefreqTruncateUtf8ToMaxBytes(fields.enText, WRITE_FREQ_EN_NAME_MAX_BYTES);
    if (enResult.wasTruncated) {
      fields.enText = enResult.text;
      const enMsg = rowLabel + '：英文信道名超过 10 字节（原 ' + enResult.originalByteLength + ' 字节，UTF-8），已截断';
      truncationWarnings.push(enMsg);
    }
    const cnResult = writefreqTruncateUtf8ToMaxBytes(fields.cnText, WRITE_FREQ_CN_NAME_MAX_BYTES);
    if (cnResult.wasTruncated) {
      fields.cnText = cnResult.text;
      const cnMsg = rowLabel + '：中文信道名超过 10 字节（原 ' + cnResult.originalByteLength + ' 字节，UTF-8），已截断';
      truncationWarnings.push(cnMsg);
    }
  }
  return truncationWarnings;
}

function writefreqValidateEnglishName(text) {
  const problems = [];
  let ci = 0;
  for (; ci < text.length; ci++) {
    const code = text.charCodeAt(ci);
    if (code < 32 || code > 127) {
      problems.push('英文信道名仅允许 ASCII 可打印字符（32–127）');
      break;
    }
  }
  return problems;
}

function writefreqValidateCnName(text) {
  const problems = [];
  const encoder = new TextEncoder();
  const encoded = encoder.encode(text);
  const byteCount = encoded.length;
  if (byteCount > WRITE_FREQ_CN_NAME_MAX_BYTES) {
    problems.push('中文信道名 UTF-8 最长 10 字节（当前 ' + byteCount + ' 字节）');
  }
  return problems;
}

function writefreqBuildEnglish16(text) {
  const truncated = writefreqTruncateUtf8ToMaxBytes(text, WRITE_FREQ_EN_NAME_MAX_BYTES);
  const encoder = new TextEncoder();
  const encoded = encoder.encode(truncated.text);
  const buf = new Uint8Array(16);
  let j = 0;
  for (; j < encoded.length; j++) {
    buf[j] = encoded[j];
  }
  return buf;
}

function writefreqBuildCn16(text) {
  const truncated = writefreqTruncateUtf8ToMaxBytes(text, WRITE_FREQ_CN_NAME_MAX_BYTES);
  const encoder = new TextEncoder();
  const encoded = encoder.encode(truncated.text);
  const buf = new Uint8Array(16);
  let j = 0;
  for (; j < encoded.length; j++) {
    buf[j] = encoded[j];
  }
  return buf;
}

/** 与 Flash 擦除态一致：整段 0xFF，表示未使用信道（覆盖写入时清空该 MR 槽） */
function writefreqErasedMrBlock16() {
  const buf = new Uint8Array(16);
  buf.fill(0xff);
  return buf;
}

function writefreqGetRowInputs(tr) {
  const rxEl = tr.querySelector('.wf-rx');
  const offsetEl = tr.querySelector('.wf-offset');
  const enEl = tr.querySelector('.wf-en');
  const cnEl = tr.querySelector('.wf-cn');
  const powerEl = tr.querySelector('.wf-power');
  const rxCtcssEl = tr.querySelector('.wf-rx-ctcss');
  const rxDcsEl = tr.querySelector('.wf-rx-dcs');
  const txCtcssEl = tr.querySelector('.wf-tx-ctcss');
  const txDcsEl = tr.querySelector('.wf-tx-dcs');
  const sftEl = tr.querySelector('.wf-sft');
  const modEl = tr.querySelector('.wf-mod');
  const rxText = rxEl ? rxEl.value : '';
  const offsetText = offsetEl ? offsetEl.value : '';
  const enText = enEl ? enEl.value : '';
  const cnText = cnEl ? cnEl.value : '';
  const powerVal = powerEl ? powerEl.value : '';
  const rxCtcss = rxCtcssEl ? rxCtcssEl.value : '';
  const rxDcs = rxDcsEl ? rxDcsEl.value : '';
  const txCtcss = txCtcssEl ? txCtcssEl.value : '';
  const txDcs = txDcsEl ? txDcsEl.value : '';
  const sftVal = sftEl ? sftEl.value : '';
  const modVal = modEl ? modEl.value : '';
  return {
    rxText,
    offsetText,
    enText,
    cnText,
    powerVal,
    rxCtcss,
    rxDcs,
    txCtcss,
    txDcs,
    sftVal,
    modVal
  };
}

function writefreqEmptyRowFields() {
  return {
    rxText: '',
    offsetText: '',
    enText: '',
    cnText: '',
    powerVal: '',
    rxCtcss: '',
    rxDcs: '',
    txCtcss: '',
    txDcs: '',
    sftVal: '0',
    modVal: '0'
  };
}

function writefreqEnsureModelInit() {
  if (writefreqRowsData !== null) {
    return;
  }
  writefreqRowsData = [];
  let i = 0;
  for (; i < WRITE_FREQ_MR_MAX; i++) {
    writefreqRowsData[i] = writefreqEmptyRowFields();
  }
}

function writefreqGetPageCount() {
  const total = WRITE_FREQ_MR_MAX;
  const pageSize = WRITE_FREQ_PAGE_SIZE;
  const fullPages = Math.floor((total + pageSize - 1) / pageSize);
  return fullPages;
}

function writefreqFlushDomToModel() {
  writefreqEnsureModelInit();
  const rowList = document.querySelectorAll('#writefreqTbody tr');
  let ri = 0;
  for (; ri < rowList.length; ri++) {
    const tr = rowList[ri];
    if (tr.style.display === 'none') {
      continue;
    }
    const chIdxRaw = tr.dataset.writefreqChIdx;
    if (chIdxRaw === undefined || chIdxRaw === '') {
      continue;
    }
    const chIdx = Number.parseInt(chIdxRaw, 10);
    if (!Number.isFinite(chIdx) || chIdx < 0 || chIdx >= WRITE_FREQ_MR_MAX) {
      continue;
    }
    writefreqRowsData[chIdx] = writefreqGetRowInputs(tr);
  }
}

function writefreqApplyFieldsToTr(tr, fields) {
  const rxIn = tr.querySelector('.wf-rx');
  const offsetEl = tr.querySelector('.wf-offset');
  const enEl = tr.querySelector('.wf-en');
  const cnEl = tr.querySelector('.wf-cn');
  const powerEl = tr.querySelector('.wf-power');
  const rxCtcssEl = tr.querySelector('.wf-rx-ctcss');
  const rxDcsEl = tr.querySelector('.wf-rx-dcs');
  const txCtcssEl = tr.querySelector('.wf-tx-ctcss');
  const txDcsEl = tr.querySelector('.wf-tx-dcs');
  const sftEl = tr.querySelector('.wf-sft');
  const modEl = tr.querySelector('.wf-mod');
  if (rxIn) {
    rxIn.value = fields.rxText;
  }
  if (offsetEl) {
    offsetEl.value = fields.offsetText;
  }
  if (enEl) {
    enEl.value = fields.enText;
  }
  if (cnEl) {
    cnEl.value = fields.cnText;
  }
  if (powerEl) {
    powerEl.value = fields.powerVal;
  }
  if (rxCtcssEl) {
    rxCtcssEl.value = fields.rxCtcss;
  }
  if (rxDcsEl) {
    rxDcsEl.value = fields.rxDcs;
  }
  if (txCtcssEl) {
    txCtcssEl.value = fields.txCtcss;
  }
  if (txDcsEl) {
    txDcsEl.value = fields.txDcs;
  }
  if (sftEl) {
    sftEl.value = fields.sftVal;
  }
  if (modEl) {
    modEl.value = fields.modVal;
  }
}

function wfBlock16ToRowFields(block16) {
  const decoded = wfDecodeChannelFields(block16);
  let rxCtcss = '';
  let rxDcs = '';
  if (decoded.rxToneType === 1) {
    rxCtcss = String(decoded.rxToneCode);
  } else if (decoded.rxToneType === 2) {
    rxDcs = 'N:' + decoded.rxToneCode;
  } else if (decoded.rxToneType === 3) {
    rxDcs = 'I:' + decoded.rxToneCode;
  }
  let txCtcss = '';
  let txDcs = '';
  if (decoded.txToneType === 1) {
    txCtcss = String(decoded.txToneCode);
  } else if (decoded.txToneType === 2) {
    txDcs = 'N:' + decoded.txToneCode;
  } else if (decoded.txToneType === 3) {
    txDcs = 'I:' + decoded.txToneCode;
  }
  let powerVal = '';
  if (decoded.power >= 1 && decoded.power <= 7) {
    powerVal = String(decoded.power);
  }
  const row = writefreqEmptyRowFields();
  row.rxText = decoded.rxMHzStr;
  row.offsetText = decoded.offsetMHzStr;
  row.sftVal = String(decoded.offsetDir);
  row.modVal = String(decoded.modulation);
  row.powerVal = powerVal;
  row.rxCtcss = rxCtcss;
  row.rxDcs = rxDcs;
  row.txCtcss = txCtcss;
  row.txDcs = txDcs;
  return row;
}

function writefreqUpdatePaginationUI() {
  const infoEl = $('writefreqPageInfo');
  const prevBtn = $('writefreqPagePrev');
  const nextBtn = $('writefreqPageNext');
  const totalPages = writefreqGetPageCount();
  const cur = writefreqPageIndex + 1;
  if (infoEl) {
    const totalLine =
      '共 ' +
      WRITE_FREQ_MR_MAX +
      ' 条 · 第 ' +
      cur +
      ' / ' +
      totalPages +
      ' 页 · 每页 ' +
      WRITE_FREQ_PAGE_SIZE +
      ' 信道';
    infoEl.textContent = totalLine;
  }
  if (prevBtn) {
    prevBtn.disabled = writefreqPageIndex <= 0;
  }
  if (nextBtn) {
    nextBtn.disabled = writefreqPageIndex >= totalPages - 1;
  }
}

function writefreqShowCurrentPage() {
  writefreqEnsureModelInit();
  const tbody = $('writefreqTbody');
  if (!tbody) {
    return;
  }
  const base = writefreqGetBaseChannel();
  const page = writefreqPageIndex;
  const startSlot = page * WRITE_FREQ_PAGE_SIZE;
  const rowList = tbody.querySelectorAll('tr');
  let slot = 0;
  for (; slot < WRITE_FREQ_PAGE_SIZE; slot++) {
    const tr = rowList[slot];
    const chIdx = startSlot + slot;
    if (chIdx >= WRITE_FREQ_MR_MAX) {
      tr.style.display = 'none';
      tr.removeAttribute('data-writefreq-ch-idx');
      tr.setAttribute('data-sortable-ignore', '1');
      continue;
    }
    tr.style.display = '';
    tr.removeAttribute('data-sortable-ignore');
    tr.dataset.writefreqChIdx = String(chIdx);
    const fields = writefreqRowsData[chIdx];
    writefreqApplyFieldsToTr(tr, fields);
    const cell = tr.querySelector('.ch-num');
    if (cell) {
      const chNum = base + chIdx;
      cell.textContent = String(chNum);
    }
  }
  writefreqUpdatePaginationUI();
  writefreqInitSortable();
}

function writefreqArrayMoveInPlace(arr, fromIndex, toIndex) {
  if (fromIndex === toIndex) {
    return;
  }
  if (fromIndex < 0 || fromIndex >= arr.length) {
    return;
  }
  if (toIndex < 0 || toIndex >= arr.length) {
    return;
  }
  const movedItem = arr[fromIndex];
  arr.splice(fromIndex, 1);
  arr.splice(toIndex, 0, movedItem);
}

function writefreqDestroySortable() {
  if (writefreqSortableInstance) {
    writefreqSortableInstance.destroy();
    writefreqSortableInstance = null;
  }
}

function writefreqInitSortable() {
  writefreqDestroySortable();
  const tbody = $('writefreqTbody');
  if (!tbody || typeof Sortable === 'undefined') {
    return;
  }
  writefreqSortableInstance = Sortable.create(tbody, {
    handle: '.wf-drag-handle',
    animation: 160,
    ghostClass: 'writefreq-sortable-ghost',
    chosenClass: 'writefreq-sortable-chosen',
    dragClass: 'writefreq-sortable-drag',
    filter: '[data-sortable-ignore]',
    preventOnFilter: true,
    onStart: function onSortStart() {
      writefreqFlushDomToModel();
    },
    onEnd: function onSortEnd(evt) {
      const oldIdx = evt.oldIndex;
      const newIdx = evt.newIndex;
      if (oldIdx === newIdx) {
        return;
      }
      if (oldIdx === undefined || newIdx === undefined) {
        return;
      }
      const pageStart = writefreqPageIndex * WRITE_FREQ_PAGE_SIZE;
      const fromGlobal = pageStart + oldIdx;
      const toGlobal = pageStart + newIdx;
      if (fromGlobal < 0 || fromGlobal >= WRITE_FREQ_MR_MAX) {
        return;
      }
      if (toGlobal < 0 || toGlobal >= WRITE_FREQ_MR_MAX) {
        return;
      }
      writefreqArrayMoveInPlace(writefreqRowsData, fromGlobal, toGlobal);
      setTimeout(function deferredWritefreqAfterSort() {
        writefreqShowCurrentPage();
      }, 0);
    }
  });
}

function writefreqPageDelta(delta) {
  const totalPages = writefreqGetPageCount();
  writefreqFlushDomToModel();
  let next = writefreqPageIndex + delta;
  if (next < 0) {
    next = 0;
  }
  if (next > totalPages - 1) {
    next = totalPages - 1;
  }
  writefreqPageIndex = next;
  writefreqShowCurrentPage();
}

function writefreqGetBaseChannel() {
  const baseRaw = writefreqTableBaseChannel;
  const baseClamped = Math.min(WRITE_FREQ_MR_MAX, Math.max(1, baseRaw || 1));
  return baseClamped;
}

function writefreqUpdateLabels() {
  writefreqShowCurrentPage();
}

function writefreqRebuildRows() {
  writefreqEnsureModelInit();
  const tbody = $('writefreqTbody');
  if (!tbody) {
    return;
  }
  tbody.innerHTML = '';
  let r = 0;
  for (; r < WRITE_FREQ_PAGE_SIZE; r++) {
    const tr = document.createElement('tr');
    const tdDrag = document.createElement('td');
    tdDrag.className = 'wf-drag-handle';
    tdDrag.setAttribute('aria-label', '拖动排序');
    tdDrag.title = '拖动排序';
    tdDrag.textContent = '⠿';

    const tdN = document.createElement('td');
    tdN.className = 'ch-num';
    tdN.textContent = '—';

    const tdRx = document.createElement('td');
    const inRx = document.createElement('input');
    inRx.type = 'text';
    inRx.className = 'wf-rx';
    inRx.placeholder = '例 438.500000';
    tdRx.appendChild(inRx);

    const tdPwr = document.createElement('td');
    const selPwr = document.createElement('select');
    selPwr.className = 'wf-power';
    const optP0 = document.createElement('option');
    optP0.value = '';
    optP0.textContent = '请选择功率';
    selPwr.appendChild(optP0);
    let pi = 1;
    for (; pi <= 7; pi++) {
      const op = document.createElement('option');
      op.value = String(pi);
      op.textContent = WF_POWER_LABELS[pi];
      selPwr.appendChild(op);
    }
    tdPwr.appendChild(selPwr);

    const tdRxDcs = document.createElement('td');
    const selRxDcs = document.createElement('select');
    selRxDcs.className = 'wf-rx-dcs';
    wfAppendDcsSelectOptions(selRxDcs);
    tdRxDcs.appendChild(selRxDcs);

    const tdRxCt = document.createElement('td');
    const selRxCt = document.createElement('select');
    selRxCt.className = 'wf-rx-ctcss';
    wfAppendCtcssSelectOptions(selRxCt);
    tdRxCt.appendChild(selRxCt);

    const tdTxDcs = document.createElement('td');
    const selTxDcs = document.createElement('select');
    selTxDcs.className = 'wf-tx-dcs';
    wfAppendDcsSelectOptions(selTxDcs);
    tdTxDcs.appendChild(selTxDcs);

    const tdTxCt = document.createElement('td');
    const selTxCt = document.createElement('select');
    selTxCt.className = 'wf-tx-ctcss';
    wfAppendCtcssSelectOptions(selTxCt);
    tdTxCt.appendChild(selTxCt);

    const tdSft = document.createElement('td');
    const selSft = document.createElement('select');
    selSft.className = 'wf-sft';
    const s0 = document.createElement('option');
    s0.value = '0';
    s0.textContent = '关闭';
    selSft.appendChild(s0);
    const s1 = document.createElement('option');
    s1.value = '1';
    s1.textContent = '+';
    selSft.appendChild(s1);
    const s2 = document.createElement('option');
    s2.value = '2';
    s2.textContent = '−';
    selSft.appendChild(s2);
    tdSft.appendChild(selSft);

    const tdOff = document.createElement('td');
    const inOff = document.createElement('input');
    inOff.type = 'text';
    inOff.className = 'wf-offset';
    inOff.placeholder = '例 5.000000';
    tdOff.appendChild(inOff);

    const tdMod = document.createElement('td');
    const selMod = document.createElement('select');
    selMod.className = 'wf-mod';
    let mi = 0;
    for (; mi < WF_MOD_LABELS.length; mi++) {
      const mo = document.createElement('option');
      mo.value = String(mi);
      mo.textContent = WF_MOD_LABELS[mi];
      selMod.appendChild(mo);
    }
    tdMod.appendChild(selMod);

    const tdEn = document.createElement('td');
    const inEn = document.createElement('input');
    inEn.type = 'text';
    inEn.className = 'wf-en';
    inEn.maxLength = 10;
    tdEn.appendChild(inEn);

    const tdCn = document.createElement('td');
    const inCn = document.createElement('input');
    inCn.type = 'text';
    inCn.className = 'wf-cn';
    tdCn.appendChild(inCn);

    tr.appendChild(tdDrag);
    tr.appendChild(tdN);
    tr.appendChild(tdRx);
    tr.appendChild(tdPwr);
    tr.appendChild(tdRxDcs);
    tr.appendChild(tdRxCt);
    tr.appendChild(tdTxDcs);
    tr.appendChild(tdTxCt);
    tr.appendChild(tdSft);
    tr.appendChild(tdOff);
    tr.appendChild(tdMod);
    tr.appendChild(tdEn);
    tr.appendChild(tdCn);
    tbody.appendChild(tr);
  }
  writefreqShowCurrentPage();
}

function writefreqShowValidation(text, show) {
  const el = $('writefreqValidation');
  if (!el) {
    return;
  }
  if (show) {
    el.style.display = 'block';
    el.textContent = text;
  } else {
    el.style.display = 'none';
    el.textContent = '';
  }
}

async function writefreqReadFromDevice() {
  if (isWritefreqBusy) {
    return;
  }
  isWritefreqBusy = true;
  const readBtn = $('writefreqReadBtn');
  const writeBtn = $('writefreqWriteBtn');
  if (readBtn) readBtn.disabled = true;
  if (writeBtn) writeBtn.disabled = true;
  $('progressContainer').style.display = 'block';
  updateProgress(0);
  try {
    if (!port) {
      await connect();
    }
    readBuffer = [];
    await sleep(800);
    const session = await requestDeviceInfoForCalib();
    const sessionTs = session.timestamp;
    writefreqTableBaseChannel = 1;
    writefreqEnsureModelInit();
    let chIdx = 0;
    for (; chIdx < WRITE_FREQ_MR_MAX; chIdx++) {
      const chIndex0 = chIdx;
      const baseAddr = chIndex0 * 16;
      const block = await spiFlashReadChunk(sessionTs, baseAddr, 16);
      if (!block || block.length !== 16) {
        throw new Error('读取信道数据失败 @ 0x' + baseAddr.toString(16));
      }
      const enAddr = WRITE_FREQ_ADDR_EN_BASE + chIndex0 * 16;
      const cnAddr = WRITE_FREQ_ADDR_CN_BASE + chIndex0 * 16;
      const enRaw = await spiFlashReadChunk(sessionTs, enAddr, 16);
      if (!enRaw) {
        const enHex = enAddr.toString(16);
        throw new Error('读取英文信道名区失败 @ CH ' + (chIndex0 + 1) + '（SPI 0x' + enHex + '）');
      }
      await sleep(25);
      const cnRaw = await spiFlashReadChunk(sessionTs, cnAddr, 16);
      if (!cnRaw) {
        const cnHex = cnAddr.toString(16);
        throw new Error('读取中文信道名区失败 @ CH ' + (chIndex0 + 1) + '（SPI 0x' + cnHex + '）');
      }
      const enText = writefreqTrimAsciiName(enRaw);
      const cnText = writefreqDecodeCnNameUtf8(cnRaw);
      const rowFields = wfBlock16ToRowFields(block);
      rowFields.enText = enText;
      rowFields.cnText = cnText;
      writefreqRowsData[chIdx] = rowFields;
      const pct = ((chIdx + 1) / WRITE_FREQ_MR_MAX) * 100;
      updateProgress(pct);
      await sleep(30);
    }
    writefreqShowCurrentPage();
    updateProgress(100);
    log('已从设备读取 MR CH1–CH' + WRITE_FREQ_MR_MAX + '（共 ' + WRITE_FREQ_MR_MAX + ' 槽）；其余 MR 信道未读取', 'success');
    writefreqShowValidation('', false);
  } catch (e) {
    log('写频读取失败: ' + e.message, 'error');
  } finally {
    isWritefreqBusy = false;
    if (readBtn) readBtn.disabled = false;
    if (writeBtn) writeBtn.disabled = false;
    if (port) {
      await disconnect();
    }
    setTimeout(() => {
      $('progressContainer').style.display = 'none';
      updateProgress(0);
    }, 600);
  }
}

async function writefreqWriteToDevice() {
  if (isWritefreqBusy) {
    return;
  }
  writefreqEnsureModelInit();
  writefreqFlushDomToModel();
  const nameTruncationWarnings = writefreqApplyAllChannelNameTruncations();
  if (nameTruncationWarnings.length > 0) {
    writefreqShowCurrentPage();
    const warnJoined = nameTruncationWarnings.join('\n');
    log('信道名截断提示（≤10 字节 UTF-8）：\n' + warnJoined, 'warning');
  }
  const messages = [];
  const startCh = writefreqGetBaseChannel();
  let validateRow = 0;
  for (; validateRow < WRITE_FREQ_MR_MAX; validateRow++) {
    const fields = writefreqRowsData[validateRow];
    if (fields.rxText.trim() === '') {
      continue;
    }
    const chNum = startCh + validateRow;
    const rowPrefix = '第 ' + chNum + ' 信道：';
    const enProblems = writefreqValidateEnglishName(fields.enText);
    const cnProblems = writefreqValidateCnName(fields.cnText);
    if (enProblems.length > 0) {
      let pi = 0;
      for (; pi < enProblems.length; pi++) {
        messages.push(rowPrefix + enProblems[pi]);
      }
    }
    if (cnProblems.length > 0) {
      let pj = 0;
      for (; pj < cnProblems.length; pj++) {
        messages.push(rowPrefix + cnProblems[pj]);
      }
    }
    try {
      writefreqParseMHzOrThrow(rowPrefix + '接收频率(MHz)', fields.rxText);
      writefreqParseMHzOrThrow(rowPrefix + '频差频率(MHz)', fields.offsetText);
    } catch (err) {
      messages.push(err.message);
    }
    if (fields.powerVal === '') {
      messages.push(rowPrefix + '请选择功率（已排除 USER）');
    }
    const powCheck = Number.parseInt(fields.powerVal, 10);
    if (fields.powerVal !== '') {
      if (!Number.isFinite(powCheck) || powCheck < 1 || powCheck > 7) {
        messages.push(rowPrefix + '功率须为 LOW1–HIGH 之一');
      }
    }
    const sftNum = Number.parseInt(fields.sftVal, 10);
    if (!Number.isFinite(sftNum) || sftNum < 0 || sftNum > 2) {
      messages.push(rowPrefix + '频差方向无效');
    }
    const modCheck = Number.parseInt(fields.modVal, 10);
    if (!Number.isFinite(modCheck) || modCheck < 0 || modCheck > 2) {
      messages.push(rowPrefix + '调制模式须为 FM / AM / USB');
    }
    try {
      wfParseToneSide(fields.rxCtcss, fields.rxDcs, rowPrefix + '接收侧');
      wfParseToneSide(fields.txCtcss, fields.txDcs, rowPrefix + '发射侧');
    } catch (toneErr) {
      messages.push(toneErr.message);
    }
  }
  if (messages.length > 0) {
    const joined = messages.join('\n');
    writefreqShowValidation(joined, true);
    log('校验未通过，未写入设备', 'error');
    return;
  }
  writefreqShowValidation('', false);

  isWritefreqBusy = true;
  const readBtn = $('writefreqReadBtn');
  const writeBtn = $('writefreqWriteBtn');
  if (readBtn) readBtn.disabled = true;
  if (writeBtn) writeBtn.disabled = true;
  $('progressContainer').style.display = 'block';
  updateProgress(0);
  try {
    if (!port) {
      await connect();
    }
    readBuffer = [];
    await sleep(800);
    const session = await requestDeviceInfoForCalib();
    const sessionTs = session.timestamp;
    let rowIdx = 0;
    for (; rowIdx < WRITE_FREQ_MR_MAX; rowIdx++) {
      /** 与读取一致：表格第 i 行对应 Flash MR 槽 i（CH i+1），与起始信道号显示无关，仅覆盖前 200 槽 */
      const chIndex0 = rowIdx;
      const fields = writefreqRowsData[rowIdx];
      const baseAddr = chIndex0 * 16;
      const enAddr = WRITE_FREQ_ADDR_EN_BASE + chIndex0 * 16;
      const cnAddr = WRITE_FREQ_ADDR_CN_BASE + chIndex0 * 16;
      if (fields.rxText.trim() === '') {
        const erasedMain = writefreqErasedMrBlock16();
        const writeEraseOk = await spiFlashWriteChunk(sessionTs, baseAddr, erasedMain);
        if (!writeEraseOk) {
          throw new Error('覆盖写入擦除信道失败 @ CH ' + (chIndex0 + 1));
        }
        const enBufClear = writefreqBuildEnglish16('');
        const enClearOk = await spiFlashWriteChunk(sessionTs, enAddr, enBufClear);
        if (!enClearOk) {
          throw new Error('覆盖写入清空英文信道名失败 @ CH ' + (chIndex0 + 1));
        }
        const cnBufClear = writefreqBuildCn16('');
        const cnClearOk = await spiFlashWriteChunk(sessionTs, cnAddr, cnBufClear);
        if (!cnClearOk) {
          throw new Error('覆盖写入清空中文信道名失败 @ CH ' + (chIndex0 + 1));
        }
        const pctErase = ((rowIdx + 1) / WRITE_FREQ_MR_MAX) * 100;
        updateProgress(pctErase);
        await sleep(40);
        continue;
      }
      const chLabel = 'CH ' + (chIndex0 + 1);
      const rxStored = writefreqParseMHzOrThrow(chLabel + ' 接收频率(MHz)', fields.rxText);
      const offsetStored = writefreqParseMHzOrThrow(chLabel + ' 频差频率(MHz)', fields.offsetText);
      const offsetDir = Number.parseInt(fields.sftVal, 10);
      const modNum = Number.parseInt(fields.modVal, 10);
      const pow7 = Number.parseInt(fields.powerVal, 10);
      if (!Number.isFinite(offsetDir) || offsetDir < 0 || offsetDir > 2) {
        throw new Error(chLabel + ' 频差方向无效');
      }
      if (!Number.isFinite(modNum) || modNum < 0 || modNum > 2) {
        throw new Error(chLabel + ' 调制模式无效');
      }
      if (!Number.isFinite(pow7) || pow7 < 1 || pow7 > 7) {
        throw new Error(chLabel + ' 功率无效');
      }
      const rxTone = wfParseToneSide(fields.rxCtcss, fields.rxDcs, chLabel + ' 接收');
      const txTone = wfParseToneSide(fields.txCtcss, fields.txDcs, chLabel + ' 发射');
      const original = await spiFlashReadChunk(sessionTs, baseAddr, 16);
      if (!original || original.length !== 16) {
        throw new Error('读取原信道块失败 @ CH ' + (chIndex0 + 1));
      }
      const merged = wfMergeUserIntoBlock(original, rxStored, offsetStored, offsetDir, modNum, pow7, rxTone, txTone);
      const writeMainOk = await spiFlashWriteChunk(sessionTs, baseAddr, merged);
      if (!writeMainOk) {
        throw new Error('写入信道数据失败 @ CH ' + (chIndex0 + 1));
      }
      const enBuf = writefreqBuildEnglish16(fields.enText);
      const enOk = await spiFlashWriteChunk(sessionTs, enAddr, enBuf);
      if (!enOk) {
        throw new Error('写入英文信道名失败 @ CH ' + (chIndex0 + 1));
      }
      const cnBuf = writefreqBuildCn16(fields.cnText);
      const cnOk = await spiFlashWriteChunk(sessionTs, cnAddr, cnBuf);
      if (!cnOk) {
        throw new Error('写入中文信道名失败 @ CH ' + (chIndex0 + 1));
      }
      const pct = ((rowIdx + 1) / WRITE_FREQ_MR_MAX) * 100;
      updateProgress(pct);
      await sleep(40);
    }
    updateProgress(100);
    log('已按表格覆盖设备 MR CH1–CH' + WRITE_FREQ_MR_MAX + '（共 ' + WRITE_FREQ_MR_MAX + ' 槽；无接收频率的槽已擦除含中英文名）。第 ' + (WRITE_FREQ_MR_MAX + 1) + ' 个及以后 MR 未修改。请先确认固件与备份。', 'success');
  } catch (e) {
    log('写频写入失败: ' + e.message, 'error');
  } finally {
    isWritefreqBusy = false;
    if (readBtn) readBtn.disabled = false;
    if (writeBtn) writeBtn.disabled = false;
    if (port) {
      await disconnect();
    }
    setTimeout(() => {
      $('progressContainer').style.display = 'none';
      updateProgress(0);
    }, 600);
  }
}

function writefreqNormalizeCellCompact(text) {
  return String(text).replace(/\s+/g, '').toLowerCase();
}

function writefreqPowerValToSheetLabel(powerVal) {
  const t = String(powerVal).trim();
  if (t === '') {
    return '请选择功率';
  }
  const n = Number.parseInt(t, 10);
  if (!Number.isFinite(n) || n < 1 || n > 7) {
    return t;
  }
  const labelOut = WF_POWER_LABELS[n];
  return labelOut;
}

function writefreqSheetLabelToPowerVal(cellText) {
  const t = String(cellText).trim();
  if (t === '' || t === '请选择功率') {
    return '';
  }
  const tLower = t.toLowerCase();
  let pi = 1;
  for (; pi <= 7; pi++) {
    if (WF_POWER_LABELS[pi].toLowerCase() === tLower) {
      return String(pi);
    }
  }
  const legacyNum = Number.parseInt(t, 10);
  if (Number.isFinite(legacyNum) && legacyNum >= 1 && legacyNum <= 7) {
    return String(legacyNum);
  }
  return '';
}

function writefreqModValToSheetLabel(modVal) {
  const t = String(modVal).trim();
  const idx = Number.parseInt(t, 10);
  if (!Number.isFinite(idx) || idx < 0 || idx >= WF_MOD_LABELS.length) {
    return t;
  }
  const modLabel = WF_MOD_LABELS[idx];
  return modLabel;
}

function writefreqSheetLabelToModVal(cellText) {
  const raw = String(cellText).trim();
  if (raw === '') {
    return '0';
  }
  let mi = 0;
  for (; mi < WF_MOD_LABELS.length; mi++) {
    if (WF_MOD_LABELS[mi] === raw) {
      return String(mi);
    }
  }
  const rawLower = raw.toLowerCase();
  let mj = 0;
  for (; mj < WF_MOD_LABELS.length; mj++) {
    if (WF_MOD_LABELS[mj].toLowerCase() === rawLower) {
      return String(mj);
    }
  }
  const legacyNum = Number.parseInt(raw, 10);
  if (Number.isFinite(legacyNum) && legacyNum >= 0 && legacyNum < WF_MOD_LABELS.length) {
    return String(legacyNum);
  }
  return '0';
}

function writefreqSftValToSheetLabel(sftVal) {
  const s = String(sftVal).trim();
  if (s === '0') {
    return '关闭';
  }
  if (s === '1') {
    return '+';
  }
  if (s === '2') {
    return '−';
  }
  return s;
}

function writefreqSheetLabelToSftVal(cellText) {
  const raw = String(cellText).trim();
  if (raw === '') {
    return '0';
  }
  if (raw === '关闭' || raw === '0') {
    return '0';
  }
  if (raw === '+' || raw === '1') {
    return '1';
  }
  if (raw === '−' || raw === '-' || raw === '–' || raw === '—' || raw === '2') {
    return '2';
  }
  return '0';
}

function writefreqCtcssValToSheetLabel(storedVal) {
  const t = String(storedVal).trim();
  if (t === '') {
    return 'OFF';
  }
  const idx = Number.parseInt(t, 10);
  if (!Number.isFinite(idx) || idx < 0 || idx >= WF_CTCSS_OPTIONS.length) {
    return t;
  }
  const menuLabel = wfCtcssIndexToMenuLabel(idx);
  return menuLabel;
}

function writefreqSheetLabelToCtcssVal(cellText) {
  const raw = String(cellText).trim();
  if (raw === '') {
    return '';
  }
  const upperOff = raw.toUpperCase();
  if (upperOff === 'OFF') {
    return '';
  }
  const compactIn = writefreqNormalizeCellCompact(raw);
  let ci = 0;
  for (; ci < WF_CTCSS_OPTIONS.length; ci++) {
    const menuLabel = wfCtcssIndexToMenuLabel(ci);
    const compactRef = writefreqNormalizeCellCompact(menuLabel);
    if (compactRef === compactIn) {
      return String(ci);
    }
    const refNoHz = compactRef.replace(/hz$/, '');
    const inNoHz = compactIn.replace(/hz$/, '');
    if (refNoHz === inNoHz && refNoHz.length > 0) {
      return String(ci);
    }
  }
  const maybeDigits = String(cellText).trim();
  const legacyIdx = Number.parseInt(maybeDigits, 10);
  if (Number.isFinite(legacyIdx) && legacyIdx >= 0 && legacyIdx < WF_CTCSS_OPTIONS.length) {
    const onlyDigits = /^\d+$/.test(maybeDigits);
    if (onlyDigits) {
      return String(legacyIdx);
    }
  }
  return '';
}

function writefreqDcsValToSheetLabel(storedVal) {
  const t = String(storedVal).trim();
  if (t === '') {
    return 'OFF';
  }
  if (t.startsWith('N:')) {
    const rest = t.slice(2);
    const ni = Number.parseInt(rest, 10);
    if (Number.isFinite(ni) && ni >= 0 && ni < WF_DCS_OPTIONS.length) {
      return wfFormatDcsMenuLabel(false, ni);
    }
    return t;
  }
  if (t.startsWith('I:')) {
    const rest2 = t.slice(2);
    const ii = Number.parseInt(rest2, 10);
    if (Number.isFinite(ii) && ii >= 0 && ii < WF_DCS_OPTIONS.length) {
      return wfFormatDcsMenuLabel(true, ii);
    }
    return t;
  }
  return t;
}

function writefreqSheetLabelToDcsVal(cellText) {
  const raw = String(cellText).trim();
  if (raw === '') {
    return '';
  }
  if (raw.toUpperCase() === 'OFF') {
    return '';
  }
  const compactIn = writefreqNormalizeCellCompact(raw);
  let ni = 0;
  for (; ni < WF_DCS_OPTIONS.length; ni++) {
    const labN = wfFormatDcsMenuLabel(false, ni);
    if (writefreqNormalizeCellCompact(labN) === compactIn) {
      return 'N:' + ni;
    }
    const labI = wfFormatDcsMenuLabel(true, ni);
    if (writefreqNormalizeCellCompact(labI) === compactIn) {
      return 'I:' + ni;
    }
  }
  const nMatch = raw.match(/^N:\s*(\d+)\s*$/i);
  if (nMatch) {
    const idx = Number.parseInt(nMatch[1], 10);
    if (Number.isFinite(idx) && idx >= 0 && idx < WF_DCS_OPTIONS.length) {
      return 'N:' + idx;
    }
    return '';
  }
  const iMatch = raw.match(/^I:\s*(\d+)\s*$/i);
  if (iMatch) {
    const idx2 = Number.parseInt(iMatch[1], 10);
    if (Number.isFinite(idx2) && idx2 >= 0 && idx2 < WF_DCS_OPTIONS.length) {
      return 'I:' + idx2;
    }
    return '';
  }
  return '';
}

function writefreqExportSheet() {
  writefreqEnsureModelInit();
  writefreqFlushDomToModel();
  const startCh = writefreqGetBaseChannel();
  const header = [
    '信道号',
    '接收频率_MHz',
    '功率',
    '接收数字亚音',
    '接收模拟亚音',
    '发射数字亚音',
    '发射模拟亚音',
    '频差方向',
    '频差频率_MHz',
    '调制模式',
    '英文信道名称',
    '中文信道名称'
  ];
  const rows = [];
  rows.push(header);
  let ri = 0;
  for (; ri < WRITE_FREQ_MR_MAX; ri++) {
    const chNum = startCh + ri;
    const fields = writefreqRowsData[ri];
    const row = [
      chNum,
      fields.rxText.trim(),
      writefreqPowerValToSheetLabel(fields.powerVal),
      writefreqDcsValToSheetLabel(fields.rxDcs),
      writefreqCtcssValToSheetLabel(fields.rxCtcss),
      writefreqDcsValToSheetLabel(fields.txDcs),
      writefreqCtcssValToSheetLabel(fields.txCtcss),
      writefreqSftValToSheetLabel(fields.sftVal),
      fields.offsetText.trim(),
      writefreqModValToSheetLabel(fields.modVal),
      fields.enText,
      fields.cnText
    ];
    rows.push(row);
  }
  const exportBasename = WRITE_FREQ_EXPORT_FILE_PREFIX + '_channels_export';
  if (typeof XLSX === 'undefined' || !XLSX.utils) {
    log('SheetJS 未加载，改用 CSV 导出', 'info');
    const lines = [];
    let li = 0;
    for (; li < rows.length; li++) {
      const escaped = rows[li].map(cell => {
        const s = String(cell);
        if (/[",\n]/.test(s)) {
          return '"' + s.replace(/"/g, '""') + '"';
        }
        return s;
      });
      lines.push(escaped.join(','));
    }
    const csvBlob = new Blob(['\ufeff' + lines.join('\n')], { type: 'text/csv;charset=utf-8' });
    const a = document.createElement('a');
    a.href = URL.createObjectURL(csvBlob);
    a.download = exportBasename + '.csv';
    a.click();
    log('已导出 CSV（MR CH1–CH' + WRITE_FREQ_MR_MAX + '，共 ' + WRITE_FREQ_MR_MAX + ' 行）', 'success');
    return;
  }
  const ws = XLSX.utils.aoa_to_sheet(rows);
  const wb = XLSX.utils.book_new();
  XLSX.utils.book_append_sheet(wb, ws, 'MR信道');
  XLSX.writeFile(wb, exportBasename + '.xlsx');
  log('已导出 Excel（MR CH1–CH' + WRITE_FREQ_MR_MAX + '，共 ' + WRITE_FREQ_MR_MAX + ' 行）', 'success');
}

function writefreqImportSheet(rowsAoA) {
  if (!rowsAoA || rowsAoA.length < 2) {
    log('表格内容为空或缺少表头', 'error');
    return;
  }
  const header = rowsAoA[0].map(c => String(c).trim());
  const idxCh = header.indexOf('信道号');
  const idxRx = header.indexOf('接收频率_MHz');
  const idxPwr = header.indexOf('功率');
  const idxRxDcs = header.indexOf('接收数字亚音');
  const idxRxCt = header.indexOf('接收模拟亚音');
  const idxTxDcs = header.indexOf('发射数字亚音');
  const idxTxCt = header.indexOf('发射模拟亚音');
  const idxSft = header.indexOf('频差方向');
  const idxOff = header.indexOf('频差频率_MHz');
  const idxMod = header.indexOf('调制模式');
  let idxEn = header.indexOf('英文信道名称');
  if (idxEn < 0) {
    idxEn = header.indexOf('英文信道名');
  }
  let idxCn = header.indexOf('中文信道名称');
  if (idxCn < 0) {
    idxCn = header.indexOf('中文信道名');
  }
  const legacyTx = header.indexOf('发射频率_MHz');
  const legacyEn = header.indexOf('英文信道名');
  const legacyCn = header.indexOf('中文信道名');
  const hasNew = idxRx >= 0 && idxOff >= 0 && idxSft >= 0;
  const hasLegacy = idxRx >= 0 && legacyTx >= 0 && legacyEn >= 0 && legacyCn >= 0;
  if (!hasNew && !hasLegacy) {
    log('表头不匹配：请使用新版导出列名，或包含 接收频率_MHz / 发射频率_MHz / 英文信道名 / 中文信道名', 'error');
    return;
  }
  if (idxCh >= 0) {
    const firstDataRow = rowsAoA[1];
    const chFirstCell = firstDataRow[idxCh];
    const chFirst = parseInt(String(chFirstCell), 10);
    if (!Number.isFinite(chFirst)) {
      log('首行数据信道号无效', 'error');
      return;
    }
    if (chFirst !== 1) {
      log('首行信道号须为 1（第一行数据对应 MR CH1），当前为 ' + chFirst + '，已取消导入', 'error');
      return;
    }
  }
  /** 与 Flash 槽 0…199 对齐：界面固定显示 CH1–CH200 */
  writefreqTableBaseChannel = 1;
  writefreqEnsureModelInit();
  let resetIdx = 0;
  for (; resetIdx < WRITE_FREQ_MR_MAX; resetIdx++) {
    writefreqRowsData[resetIdx] = writefreqEmptyRowFields();
  }
  const dataRowCount = rowsAoA.length - 1;
  const cellStrFactory = function cellStrFactory(rowArr) {
    return function cellStr(idx) {
      if (idx < 0) {
        return '';
      }
      const raw = rowArr[idx];
      if (raw === undefined || raw === null) {
        return '';
      }
      return String(raw);
    };
  };
  let di = 0;
  for (; di < dataRowCount && di < WRITE_FREQ_MR_MAX; di++) {
    const src = rowsAoA[di + 1];
    const cellStr = cellStrFactory(src);
    const merged = writefreqEmptyRowFields();
    if (hasNew) {
      merged.rxText = cellStr(idxRx);
      merged.offsetText = cellStr(idxOff);
      merged.powerVal = writefreqSheetLabelToPowerVal(cellStr(idxPwr));
      merged.rxDcs = writefreqSheetLabelToDcsVal(cellStr(idxRxDcs));
      merged.rxCtcss = writefreqSheetLabelToCtcssVal(cellStr(idxRxCt));
      merged.txDcs = writefreqSheetLabelToDcsVal(cellStr(idxTxDcs));
      merged.txCtcss = writefreqSheetLabelToCtcssVal(cellStr(idxTxCt));
      merged.sftVal = writefreqSheetLabelToSftVal(cellStr(idxSft));
      merged.modVal = writefreqSheetLabelToModVal(cellStr(idxMod));
      merged.enText = cellStr(idxEn);
      merged.cnText = cellStr(idxCn);
    }
    if (hasLegacy && !hasNew) {
      merged.rxText = cellStr(idxRx);
      merged.offsetText = '0';
      merged.sftVal = '0';
      merged.modVal = '0';
      merged.powerVal = '1';
      merged.enText = cellStr(legacyEn);
      merged.cnText = cellStr(legacyCn);
      const rxTxt = cellStr(idxRx);
      const txTxt = cellStr(legacyTx);
      const rxHzTry = Number.parseFloat(rxTxt) * 1e6;
      const txHzTry = Number.parseFloat(txTxt) * 1e6;
      if (Number.isFinite(rxHzTry) && Number.isFinite(txHzTry)) {
        const delta = txHzTry - rxHzTry;
        let dir = 0;
        let offAbs = 0;
        if (delta === 0) {
          dir = 0;
          offAbs = 0;
        } else if (delta > 0) {
          dir = 1;
          offAbs = delta;
        } else {
          dir = 2;
          offAbs = -delta;
        }
        merged.sftVal = String(dir);
        merged.offsetText = (offAbs / 1e6).toFixed(6);
      }
    }
    writefreqRowsData[di] = merged;
  }
  writefreqPageIndex = 0;
  writefreqRebuildRows();
  const importedRows = Math.min(dataRowCount, WRITE_FREQ_MR_MAX);
  log(
    '已清空内存表格并导入 ' +
      importedRows +
      ' 行数据（完整 MR ' +
      WRITE_FREQ_MR_MAX +
      ' 槽：未出现在表格中的槽保持空白）。写入设备时仅覆盖 CH1–CH' +
      WRITE_FREQ_MR_MAX +
      '。请校验后再写入。',
    'success'
  );
}

const writefreqReadBtnEl = $('writefreqReadBtn');
if (writefreqReadBtnEl) {
  writefreqReadBtnEl.addEventListener('click', () => {
    writefreqReadFromDevice();
  });
}
const writefreqWriteBtnEl = $('writefreqWriteBtn');
if (writefreqWriteBtnEl) {
  writefreqWriteBtnEl.addEventListener('click', () => {
    writefreqWriteToDevice();
  });
}
const writefreqExportBtnEl = $('writefreqExportBtn');
if (writefreqExportBtnEl) {
  writefreqExportBtnEl.addEventListener('click', () => {
    writefreqExportSheet();
  });
}
const writefreqImportBtnEl = $('writefreqImportBtn');
const writefreqImportFileEl = $('writefreqImportFile');
if (writefreqImportBtnEl && writefreqImportFileEl) {
  writefreqImportBtnEl.addEventListener('click', () => {
    writefreqImportFileEl.click();
  });
  writefreqImportFileEl.addEventListener('change', ev => {
    const file = ev.target.files && ev.target.files[0];
    if (!file) {
      return;
    }
    const reader = new FileReader();
    reader.onload = e => {
      const buf = e.target.result;
      const nameLower = file.name.toLowerCase();
      try {
        if (nameLower.endsWith('.csv')) {
          const text = new TextDecoder('utf-8').decode(new Uint8Array(buf));
          const lines = text.split(/\r?\n/).filter(L => L.length > 0);
          const rows = [];
          let li = 0;
          for (; li < lines.length; li++) {
            const parts = lines[li].split(',');
            rows.push(parts);
          }
          if (rows.length > 0 && rows[0].length > 0) {
            rows[0][0] = String(rows[0][0]).replace(/^\ufeff/, '');
          }
          writefreqImportSheet(rows);
        } else if (typeof XLSX !== 'undefined' && XLSX.read) {
          const wb = XLSX.read(buf, { type: 'array' });
          const sheetName = wb.SheetNames[0];
          const sheet = wb.Sheets[sheetName];
          const rows = XLSX.utils.sheet_to_json(sheet, { header: 1 });
          writefreqImportSheet(rows);
        } else {
          log('请先加载 SheetJS 或使用 CSV 导入', 'error');
        }
      } catch (err) {
        log('导入失败: ' + err.message, 'error');
      }
    };
    reader.readAsArrayBuffer(file);
    writefreqImportFileEl.value = '';
  });
}

const writefreqPagePrevEl = $('writefreqPagePrev');
const writefreqPageNextEl = $('writefreqPageNext');
if (writefreqPagePrevEl) {
  writefreqPagePrevEl.addEventListener('click', () => {
    writefreqPageDelta(-1);
  });
}
if (writefreqPageNextEl) {
  writefreqPageNextEl.addEventListener('click', () => {
    writefreqPageDelta(1);
  });
}

writefreqRebuildRows();
