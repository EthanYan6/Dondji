/**
 * Language Switcher for Dondji Web Tool
 * Supports Chinese (zh) and English (en)
 */

(function() {
  'use strict';

  // Translation dictionary
  const translations = {
    zh: {
      // Page title and meta
      'pageTitle': 'UV-K1 / UV-K5(K6) V3',
      'pageTitleHelp': '帮助文档 - 叮咚鸡（Dondji）',
      
      // Loading overlay
      'loadingNetworkPoor': '兄弟，你的网有些差呀~',
      'loadingChicken': '叮咚鸡正在努力加载中...',
      'loadingProgress': '0%',
      
      // Top marquee
      'marqueeText': '📻 共有 {uv} 位 Ham 访问过本网站，今日与你一起刷机的有 {today_uv} 人 📻',
      
      // Header buttons
      'helpDoc': '帮助文档',
      'helpDocTitle': '帮助文档',
      'themeToggleTitle': '切换浅色主题',
      
      // Social chips
      'douyin': '抖音',
      'douyinName': '小闫连不上',
      'bili': 'B站',
      'biliName': '小闫同学啊',
      'redbook': '小红书',
      'redbookName': '小闫同学',
      'wechatVideo': '微信视频号',
      'wechatVideoName': '小闫连不上',
      'fmoScreen': 'FMO副屏',
      'clickView': '点击查看',
      'wechatGroup': '微信群',
      'otherFirmware': '其它固件合集',
      
      // Timeline sidebar
      'timelineTitle': '叮咚鸡（Dondji）历史版本',
      'timelineLoading': '加载中...',
      
      // Main header
      'mainTitle': 'UV-K1 / UV-K5(K6) V3 Web Tool',
      'devBy': '由 {dev} 开发',
      'buyCoffee': '请他喝杯咖啡',
      'buyCoffeeTitle': '点击打开页面进行打赏',
      
      // Flash steps
      'flashSteps': '刷机步骤',
      'flashStepsOrder': '推荐刷机顺序',
      'step1': '备份校准',
      'step1Tooltip': '原厂固件备份一次校准即可，后续使用原始校准恢复。开机状态下备份。',
      'step2': '刷固件',
      'step2Tooltip': '关机后，按住PTT键的同时，转动旋钮开机，进入刷机模式。',
      'step3': '清除数据',
      'step3Tooltip': '初次刷入叮咚鸡固件后需要此步骤。建议先"备份配置"。关机后，同时按PTT键和紧挨着PTT键下方的侧键，一起按下后，再开机，解锁全部功能菜单。在"其它"中"恢复出厂"里选择全部。重置设备存储的信道与配置。之后可通过"恢复配置"还原设置项。',
      'step4': '刷字库',
      'step4Tooltip': '如果版本更新日志中没有字库更改，那么无需此步骤。开机状态下刷入字库，每个版本的固件配合每个固件的中文字库，不要混用！',
      'step5': '恢复校准',
      'step5Tooltip': '初次刷入叮咚鸡固件或初次升级V5.0.0以后版本需要此步骤。开机状态下进行，注意：只有本网站以及uvtools2网站的校准文件支持恢复，其它网站不支持。',
      'step6': '写频',
      'step6Tooltip': '开机状态下进行，读取或者写入后都会断开连接，是正常现象，防止长时间占用端口，期间误操作/误碰。',
      
      // Warning messages
      'calibWarning': '恢复校准使用原厂固件初始备份。不需要每次备份校准，后面的也许备份是错的！',
      'snowScreenWarning': '哪个小倒霉蛋刷了V5.0.0后，变雪花屏了，肯定是校准有问题。下载刷入校准即可',
      'downloadCalibK1': '点击下载校准(K1)',
      'downloadCalibK6V3': '点击下载校准(K6V3)',
      
      // Tabs
      'tabDump': '备份校准',
      'tabFlash': '刷固件',
      'tabFont': '刷字库',
      'tabRestore': '恢复校准',
      'tabWritefreq': '写频',
      'tabLogo': '开机图片',
      'tabBackupcfg': '备份配置',
      'tabRestorecfg': '恢复配置',
      'tabToolbox': '应急工具箱',
      
      // Flash firmware tab
      'fwFile': '固件文件 (.bin)',
      'noFileSelected': '未选择文件',
      'remoteFetch': '远程获取',
      'localSelect': '本地选择',
      'selectLocalFw': '选择本地固件文件',
      'flashFirmware': '刷入固件',
      
      // Font tab
      'fontDesc1': '将中文字库刷入 SPI Flash，支持 1392 个中文字符（省份、城市、姓氏、全国中继常用字等）。如果最新版本更新日志中，字库有变化，就需要重新刷入字库。',
      'fontWarning1': '⚠️ 仅 V4.5.3 及以后支持中文信道的固件需要刷入字库。',
      'fontWarning2': '⚠️ 请先正常开机进入使用界面后再连接 USB 刷字库（与备份/恢复校准相同，无需 BOOT 模式）。',
      'fontFile': '字库文件 (.bin)',
      'selectLocalFont': '选择本地字库文件',
      'flashFont': '刷入字库',
      
      // Dump calibration tab
      'dumpWarning': '⚠️ 仅需原厂固件刷机前备份一次。先正常开机进入使用界面（能显示信道、菜单），再连接 USB 并导出。',
      'exportCalib': '导出校准数据',
      'downloadCalib': '下载 calibration.dat',
      
      // Restore calibration tab
      'restoreWarning': '⚠️ 恢复前同样需正常开机进入使用界面后再连接 USB；写入完成后设备会自动重启。请使用与本机固件版本匹配的备份文件（512 字节）。',
      'selectCalibFile': '选择校准备份文件 (.dat)',
      'restoreCalib': '恢复校准',
      
      // Calib check modal
      'calibCheckTitle': '校准数据检查',
      'calibCheckDesc': '校准解读',
      'calibCheckOffical': '官方/v4+ 校准地址(0x1E00)',
      'calibCheckThird': '第三方固件/v5+ 校准地址(0xB000)',
      'calibCheckBackup': '备份校准',
      'calibCheckConfirm': '确认恢复',
      'calibCheckCancel': '取消',
      'calibReadDevice': '读取设备校准',
      'calibLoadBackup': '加载备份校准文件',
      'calibExportBackup': '导出备份校准',
      'calibWriteOfficial': '写入官方地址',
      'calibWriteThirdParty': '写入第三方地址',
      
      // Flash device warning modal
      'flashWarningTitle': '刷机前请确认设备型号',
      'flashWarningText1': '请检查你的设备是否为泉盛 uvk1、泉盛 uvk5/uvk6 V3 版本。老版本泉盛 uvk5 和 uvk6 刷机会变砖！！！',
      'flashWarningText2': '查看方法：机器背部标签是否有 V3 字样，或者询问卖家。',
      'flashWarningOk': '我已了解',
      
      // Snow screen warning HTML
      'snowScreenWarningHtml': '哪个小倒霉蛋刷了V5.0.0后，变雪花屏了，肯定是校准有问题。下载刷入校准即可',
      
      // Help page
      'helpBack': '返回',
      'helpTitle': '帮助文档',
      'helpTocLoading': '加载中...',
      'helpContentLoading': '正在加载文档...',
      'helpTocEmpty': '暂无目录',
      'helpContentError': '加载文档失败',
      'helpContentErrorHint': '请确保 data/help.md 文件存在。',
      'helpTocError': '加载失败',
      
      // Language switcher
      'langSwitchTitle': '切换语言',
      'langSwitchZh': '中文',
      'langSwitchEn': 'English',
      'langSwitchLabel': 'EN',
      
      // Write frequency
      'freqChannel': '信道',
      'freqReceive': '接收频率',
      'freqTransmit': '发射频率',
      'freqOffset': '频差',
      'freqMode': '模式',
      'freqName': '名称',
      'freqPower': '功率',
      'freqBandwidth': '带宽',
      'freqAdd': '添加',
      'freqRead': '读取',
      'freqWrite': '写入',
      'freqClear': '清空',
      'writefreqNote': '提示：写频功能仅供叮咚鸡（Dondji）固件使用。如缺字，',
      'clickSupplement': '点击补充',
      'readFromDevice': '从设备读取',
      'writeToDevice': '写入设备',
      'exportExcel': '导出 Excel',
      'importExcel': '导入 Excel',
      'dragSort': '拖动排序',
      'freqNameSub': 'UTF-8 最长 15 字节',
      'freqMHz': 'MHz',
      'freqPowerSub': '（排除 USER）',
      'freqRxDCS': '接收数字亚音',
      'freqRxCTCSS': '接收模拟亚音',
      'freqTxDCS': '发射数字亚音',
      'freqTxCTCSS': '发射模拟亚音',
      'freqDCS': 'DCS',
      'freqCTCSS': 'CTCSS',
      'freqOffsetDir': '频差方向',
      'freqOffsetDirSub': 'MENU 频差方向',
      'freqModeSub': 'FM/AM/USB',
      'freqStep': '步进',
      'freqkHz': 'kHz',
      'freqList': '信道列表',
      'freqListSub': '多选参与',
      'clearRow': '清空本行',
      'prevPage': '上一页',
      'nextPage': '下一页',
      'freqPageInfoText': '共 {total} 条 · 已填写 {filled} 条 · 第 {cur} / {pages} 页 · 每页 {size} 信道',
    
    // Log messages
    'logRequestSerial': '请求串口...',
    'logConnected': '已连接',
    'logDisconnected': '已断开',
    'logWaitingDevice': '等待设备...',
    'logDeviceInfo': '设备信息: ',
    'logUid': 'UID: ',
    'logBootloader': 'Bootloader: ',
    'logFirmwareInfo': '固件 v{ver}：校准区基址 {addr}',
    'logDeviceInfoHex': '设备信息(hex): ',
    'logRequestingDeviceInfo': '正在请求设备信息（{purpose}）...',
    'logReceivedMessage': '收到消息: 0x{type}',
    'logDeviceReady': '设备已就绪（{purpose}会话）',
    'logFirmwareLoaded': '固件已加载：{name} ({size} bytes)',
    'logStartFlash': '开始刷入固件...',
    'logFlashProgress': '页面 {page}/{total}',
    'logFlashComplete': '固件刷入完成！',
    'logError': '错误: {msg}',
    'logLoadingFile': '正在加载...{name}',
    'logFirmwareLoadedDefault': '固件已加载: {name} ({size} bytes)',
    'logLoadFailed': '加载失败: {msg}',
    'logFontLoaded': '字库已加载：{name} ({size} bytes)',
    'logFontLoadedDefault': '字库已加载: {name} ({size} bytes)',
    'logDetectDevice': '检测设备模式...',
    'logDeviceCustomFirmware': '设备已运行自定义固件，开始刷入字库...',
    'logRetry': '重试 @ 0x{addr} ({retry})',
    'logWrittenBytes': '已写入 {written}/{total} bytes',
    'logFontFlashComplete': '字库刷入完成！共 {size} bytes',
    'logVersionWriteTimeout': '版本标记写入超时（可能固件不支持 SPI Flash 写入）',
    'logVersionWritten': '版本标记已写入',
    'logVerifyPass': '验证通过：字库数据正确',
    'logVerifyWarning': '验证警告：首字节 0x{w0} 0x{w1}（期望 0x1100 0x2100）',
    'logVerifySkip': '验证跳过：读取超时',
    'logExportCalibration': '导出校准数据...',
    'logCalibrationExportComplete': '校准数据导出完成',
    'logFileSizeError': '文件大小错误: {size} (需要 {expected})',
    'logCalibrationFileLoaded': '校准文件已加载: {name}',
    'logRestoreCalibration': '恢复校准数据...',
    'logCalibrationRestoreComplete': '校准数据恢复完成！正在重启...',
    'logDeviceRebooted': '设备已重启',
    'logFirmwareCalibBase': '固件 v{ver}：校准区基址 0x{addr}',
    'logReadCalibration': '读取校准数据 @ 0x{addr}...',
    'logCalibrationReadComplete': '校准读取完成 @ 0x{addr}',
    'logDeviceCalibrationReadComplete': '设备校准读取完成',
    'logBackupCalibrationLoaded': '备份校准已加载: {name}',
    'logNoBackupCalibration': '没有备份校准数据可导出',
    'logBackupCalibrationExported': '备份校准已导出',
    'logWriteCalibration': '写入校准到 0x{addr}...',
    'logCalibrationWriteComplete': '校准写入完成！正在重启...',
    'logNoBackupCalibrationToWrite': '没有备份校准数据可写入',
    'logExportConfig': '导出配置数据...',
    'logConfigExportComplete': '配置数据导出完成',
    'logConfigFileLoaded': '配置文件已加载: {name}',
    'logRestoreConfig': '恢复配置数据...',
    'logConfigRestoreComplete': '配置数据恢复完成！正在重启...',
    'logWebSerialUnsupported': '浏览器不支持 Web Serial API，请使用 Chrome/Edge/Opera',
    'logWritefreqReadFailed': '写频读取失败: {msg}',
    'logWritefreqReadSuccess': '表格已清空后，已从设备填入 {count} 条（已跳过未使用槽，以及不符合完整校验的槽：RX 范围、有效功率、属性 band 与频率一致、亚音可解析）；已扫描 {scanned} 槽',
    'logChannelNameTruncate': '信道名截断提示（≤15 字节 UTF-8，超长末尾为 ...）：\n{warn}',
    'logValidationFailed': '校验未通过，未写入设备',
    'logWritefreqSuccess': '写入成功 {count} 个已填写信道；其余 {empty} 个已擦除为未使用',
    'logRebootingDevice': '正在重启设备以加载新信道数据…',
    'logRebootSent': '已发送重启指令（设备将自动复位）',
    'logWritefreqWriteFailed': '写频写入失败: {msg}',
    'logSheetJSNotLoaded': 'SheetJS 未加载，改用 CSV 导出',
    'logCsvExportSuccess': '已导出 CSV（仅已填写接收频率的信道，共 {count} 行）',
    'logExcelExportSuccess': '已导出 Excel（仅已填写接收频率的信道，共 {count} 行）',
    'logTableEmpty': '表格内容为空或缺少表头',
    'logCsvImportSuccess': '已导入 CSV（共 {count} 行，{valid} 个有效信道）',
    'logFirstRowInvalid': '首行数据信道号无效',
    'logRowChannelInvalid': '第 {row} 行数据：信道号无效',
    'logRowChannelOutOfBounds': '第 {row} 行数据：信道号映射越界',
    'logLoadSheetJSFirst': '请先加载 SheetJS 或使用 CSV 导入',
    'logImportFailed': '导入失败: {msg}',
    'logSelectImageFirst': '请先选择一张图片',
    'logConnectFailed': '连接失败: {msg}',
    'logUploadingBootLogo': '正在上传开机图片...',
    'logWrittenBytesProgress': '已写入 {written}/{total} bytes',
    'logBootLogoUploadSuccess': '开机图片上传成功！请在菜单「开机画面」中选择「自定义」',
    'logUploadFailed': '上传失败: {msg}',
    'logReadingBootLogo': '正在读取开机图片...',
    'logBootLogoReadSuccess': '开机图片读取成功',
    'logReadFailed': '读取失败: {msg}',
    'logVerifyWarning': '验证警告：首字节 0x{w0} 0x{w1}（期望 0x1100 0x2100）',
    'logReadCalibrationAddr': '读取校准数据 @ 0x{addr}...',
    'logCalibrationReadAddrComplete': '校准读取完成 @ 0x{addr}',
    'logDeviceCalibrationReadComplete': '设备校准读取完成',
    'logBackupCalibrationLoaded': '备份校准已加载: {name}',
    'logNoBackupCalibrationExport': '没有备份校准数据可导出',
    'logBackupCalibrationExported': '备份校准已导出',
    'logWriteCalibrationAddr': '写入校准到 0x{addr}...',
    'logCalibrationWriteComplete': '校准写入完成！正在重启...',
    'logNoBackupCalibrationWrite': '没有备份校准数据可写入',
    'logExportConfig': '导出配置数据...',
    'logConfigExportComplete': '配置数据导出完成',
    'logConfigFileSizeError': '文件大小错误: {size} (需要 {expected})',
    'logConfigFileLoaded': '配置文件已加载: {name}',
    'logRestoreConfig': '恢复配置数据...',
    'logConfigRestoreComplete': '配置数据恢复完成！正在重启...',
    
    // Visitor statistics
    'visitorTotalPrefix': '共有',
    'visitorTotalSuffix': '位 Ham 访问过本网站',
    'visitorTodayPrefix': '今日与你一起刷机的有',
    'visitorTodaySuffix': '人',
      'writeSuccess': '写入成功 {count} 个已填写信道；其余 {empty} 个已擦除为未使用',
      'exportCSVSucc': '已导出 CSV（仅已填写接收频率的信道，共 {count} 行）',
      'exportExcelSucc': '已导出 Excel（仅已填写接收频率的信道，共 {count} 行）',
      
      // Writefreq table placeholders and options
      'freqNamePlaceholder': 'ASCII 或汉字等，≤15 字节 UTF-8',
      'freqRxPlaceholder': '例 438.500000',
      'freqOffsetPlaceholder': '例 5.000000',
      'freqSelectPower': '请选择功率',
      'freqNoParticipate': '不参与',
      'freqScanlistAll': '全部',
      'freqScanlistItem': '列表 {num}',
      'freqValidationError': '请选择功率（已排除 USER）',
      'freqSftClose': '关闭',
      'freqSftPlus': '+',
      'freqSftMinus': '−',
      
      // Logo
      'logoDesc': '上传自定义开机图片（128x64像素，黑白位图）',
      'logoDesc1': '上传自定义 128×64 开机图片到对讲机。支持 PNG、JPG、BMP 等格式，图片会自动缩放并转为单色位图。在菜单「开机画面」中选择「自定义」即可显示。',
      'logoWarning': '⚠️ 请先正常开机进入使用界面后再连接 USB 操作（与刷字库、备份校准相同，无需 BOOT 模式）。',
      'logoFile': '图片文件 (.bmp)',
      'logoFlash': '刷入开机图片',
      'logoDefault': '恢复默认图片',
      
      // Backup config
      'backupCfgDesc': '备份当前设备的配置文件',
      'backupCfgDesc1': '备份设备配置数据（菜单设置、按键设置等），用于恢复出厂设置后快速恢复所有设置项。',
      'backupCfgWarning': '⚠️ 请先正常开机进入使用界面后再连接 USB 进行备份。',
      'backupCfgUsage': '使用场景：先备份配置 → 刷了其它固件又刷回来后，清除了全部数据 → 通过"恢复配置"恢复所有设置项。',
      'backupCfg': '备份配置',
      'exportCfgData': '导出配置数据',
      'downloadCfgBackup': '下载 config_backup.dat',
      
      // Restore config
      'restoreCfgDesc': '恢复之前备份的配置数据，快速还原所有菜单设置项。',
      'restoreCfgWarning': '⚠️ 恢复前同样需正常开机进入使用界面后再连接 USB；写入完成后设备会自动重启。',
      'restoreCfgFile': '选择配置文件 (.dat)',
      'selectCfgBackup': '选择配置备份文件 (.dat)',
      'restoreCfg': '恢复配置',
      'restoreCfgData': '恢复配置数据',
      'selectFile': '选择文件',
      
      // Toolbox
      'toolboxDesc': '应急工具箱，用于修复常见问题',
      'toolboxDesc2': '应急工具箱提供校准文件和原厂固件的下载，用于救砖或恢复出厂状态。',
      'toolboxReset': '重置设备',
      'toolboxReboot': '重启设备',
      'toolboxDowngrade': '刷回官方解锁写频密码',
      'clickDetail': '点击查看详情',
      'toolboxK1Calib': 'K1 校准文件',
      'toolboxK6Calib': 'K6 V3 校准文件',
      'toolboxK1StockEn': 'K1 原厂英文固件',
      'toolboxK1StockEnV703': 'K1 原厂英文固件',
      'toolboxK5V3StockEn': 'K5 V3 原厂英文固件',
      'toolboxK1StockCn': 'K1 官方固件',
      'toolboxK5V3StockCn': 'K5 V3 官方固件',
      'toolboxK1CPS': 'K1 官方写频软件',
      
      // Downgrade guide modal
      'downgradeTitle': '🔧 刷回官方解锁写频密码',
      'downgradeSubtitle': '叮咚鸡降级官方解锁写频密码：',
      'downgradeStep1Title': '恢复全部出厂设置：',
      'downgradeStep1Content': '按住 "PTT+ 侧键 1" 开机，选择：恢复出厂->全部->确定。',
      'downgradeStep2Title': '降级到 V4.3.2 版本：',
      'downgradeStep2Content': '刷 "f4hwn.IOTCU_K1_v4.323_b02.bin" 固件',
      'downgradeStep3Title': '恢复降级配置：',
      'downgradeStep3Content1': '恢复 "降级配置.bin"',
      'downgradeStep3Content2': '恢复配置地址：',
      'downgradeStep3Note': '注意：进度正常卡到 50%-60% 左右，直接关机就行，不用重复恢复。',
      'downgradeStep4Title': '恢复全部出厂设置：',
      'downgradeStep4Content': '按住 PTT+ 侧键 1 开机，选择：恢复出厂设置->全部参数->确定。',
      'downgradeStep5Title': '官方 CPS 写频软件更新官方固件：',
      'downgradeStep5Content': '设备->升级程序->升级。',
      'downgradeFiles': '所需文件下载：',
      'downgradeConfigBin': '降级配置.bin',
      'toolboxStockFw': 'K1 官方固件',
      'toolboxK6StockFw': 'K6 V3 官方固件',
      
      // Logo tab
      'selectImage': '选择图片',
      'threshold': '阈值',
      'invert': '反色',
      'invertBW': '反转黑白',
      'preview': '预览',
      'uploadLogo': '上传开机图片',
      'readLogo': '读取开机图片',
      'downloadLogo': '下载 boot_logo.png',
      
      // Log
      'showLog': '显示日志',
      'hideLog': '隐藏日志',
      
      // Footer and aside
      'note': '注意：',
      'browserNote': '需要 Chrome / Edge / Opera 浏览器。',
      'flashFirmwareBoot': '仅刷固件',
      'enterBootMode': '需进入 BOOT 模式（按住 PTT 开机）。',
      'otherOperations': '刷字库、备份校准、恢复校准、开机图片',
      'normalMode': '均在正常开机使用界面下连接 USB 操作。',
      'footerDev': '固件 & 工具由 BD1AHN 开发',
      'footerUVTools': 'UVTools2',
      
      // Coffee modal
      'coffeeTitle': '☕ 请作者喝杯咖啡',
      'coffeeText1': '感谢您使用叮咚鸡固件！维护这个网站和固件需要投入大量时间和精力，运营、开发、测试等都需要成本支持。',
      'coffeeText2': '如果这个项目对您有帮助，欢迎打赏支持！打赏时请备注您的呼号，方便我感谢您的支持！',
      'wechatPay': '微信支付',
      'aliPay': '支付宝',
      'donationBoard': '打赏榜单',
      'donationTime': '时间',
      'donationName': '昵称',
      'donationAmount': '金额',
      'donationMessage': '留言',
      'donationSource': '来源',
      'loading': '加载中...',
      'loadingFile': '正在加载...',
      'noDonationRecord': '暂无打赏记录',
      'coffeeNotice': '量力而行，心意至上，拒绝攀比，自愿为主。',
      'exit': '退出',
      'coffeeDesc': '感谢您的支持！',
      'donationList': '打赏名单',
      'close': '关闭',
      
      // Misc
      'connecting': '正在连接...',
      'connected': '已连接',
      'disconnected': '已断开',
      'success': '操作成功',
      'error': '操作失败',
      'confirm': '确认',
      'cancel': '取消'
    },
    
    en: {
      // Page title and meta
      'pageTitle': 'UV-K1 / UV-K5(K6) V3',
      'pageTitleHelp': 'Help Documentation - Dondji',
      
      // Loading overlay
      'loadingNetworkPoor': 'Buddy, your network seems slow~',
      'loadingChicken': 'Dondji is loading...',
      'loadingProgress': '0%',
      
      // Top marquee
      'marqueeText': '📻 Total {uv} Hams visited this site, {today_uv} flashing with you today 📻',
      
      // Header buttons
      'helpDoc': 'Help',
      'helpDocTitle': 'Help Documentation',
      'themeToggleTitle': 'Toggle light theme',
      
      // Social chips
      'douyin': 'Douyin',
      'douyinName': 'Little Yan Can\'t Connect',
      'bili': 'Bilibili',
      'biliName': 'Little Yan Student',
      'redbook': 'Red',
      'redbookName': 'Little Yan Student',
      'wechatVideo': 'WeChat Video',
      'wechatVideoName': 'Little Yan Can\'t Connect',
      'fmoScreen': 'FMO Screen',
      'clickView': 'Click to view',
      'wechatGroup': 'WeChat Group',
      'otherFirmware': 'Other Firmware Collection',
      
      // Timeline sidebar
      'timelineTitle': 'Dondji Version History',
      'timelineLoading': 'Loading...',
      
      // Main header
      'mainTitle': 'UV-K1 / UV-K5(K6) V3 Web Tool',
      'devBy': 'Developed by {dev}',
      'buyCoffee': 'Buy him a coffee',
      'buyCoffeeTitle': 'Click to open donation page',
      
      // Flash steps
      'flashSteps': 'Flash Steps',
      'flashStepsOrder': 'Recommended flashing order',
      'step1': 'Backup Calibration',
      'step1Tooltip': 'Backup calibration once with stock firmware. Use original calibration for later restore. Backup while device is powered on.',
      'step2': 'Flash Firmware',
      'step2Tooltip': 'After powering off, hold PTT button and turn knob to power on, entering flash mode.',
      'step3': 'Clear Data',
      'step3Tooltip': 'Required after first flashing Dondji firmware. Suggest "Backup Config" first. Power off, hold PTT and side key below PTT, then power on to unlock all menus. Select "All" in "Factory Reset" under "Other". Reset channel and config storage. Can restore via "Restore Config".',
      'step4': 'Flash Font',
      'step4Tooltip': 'Not needed if update log shows no font changes. Flash font while powered on. Each firmware version has its own font file, do not mix!',
      'step5': 'Restore Calibration',
      'step5Tooltip': 'Required after first flashing Dondji or upgrading to V5.0.0+. Perform while powered on. Note: Only calibration files from this site and uvtools2 support restore.',
      'step6': 'Frequency Programming',
      'step6Tooltip': 'Perform while powered on. Connection disconnects after read/write, this is normal to prevent long port occupation and accidental operations.',
      
      // Warning messages
      'calibWarning': 'Use original stock firmware calibration backup. Do not backup calibration every time, later backups may be wrong!',
      'snowScreenWarning': 'If you see snow screen after flashing V5.0.0, calibration is the issue. Download and flash calibration',
      'downloadCalibK1': 'Download calibration (K1)',
      'downloadCalibK6V3': 'Download calibration (K6V3)',
      
      // Tabs
      'tabDump': 'Backup Calib',
      'tabFlash': 'Flash Firmware',
      'tabFont': 'Flash Font',
      'tabRestore': 'Restore Calib',
      'tabWritefreq': 'Freq Program',
      'tabLogo': 'Boot Logo',
      'tabBackupcfg': 'Backup Config',
      'tabRestorecfg': 'Restore Config',
      'tabToolbox': 'Toolbox',
      
      // Flash firmware tab
      'fwFile': 'Firmware file (.bin)',
      'noFileSelected': 'No file selected',
      'remoteFetch': 'Remote Fetch',
      'localSelect': 'Local Select',
      'selectLocalFw': 'Select local firmware file',
      'flashFirmware': 'Flash Firmware',
      
      // Font tab
      'fontDesc1': 'Flash Chinese font to SPI Flash, supports 1392 Chinese characters (provinces, cities, surnames, relay common characters etc.). Re-flash if font changes in latest version update log.',
      'fontWarning1': '⚠️ Only firmware V4.5.3+ supporting Chinese channels needs font flash.',
      'fontWarning2': '⚠️ Please enter normal usage interface first before connecting USB to flash font (same as backup/restore calibration, no BOOT mode needed).',
      'fontFile': 'Font file (.bin)',
      'selectLocalFont': 'Select local font file',
      'flashFont': 'Flash Font',
      
      // Dump calibration tab
      'dumpWarning': '⚠️ Only backup once before flashing from stock firmware. First enter normal usage interface (showing channels and menus), then connect USB to export.',
      'exportCalib': 'Export Calibration Data',
      'downloadCalib': 'Download calibration.dat',
      
      // Restore calibration tab
      'restoreWarning': '⚠️ Same as restore, enter normal usage interface first before connecting USB; device auto-reboots after write. Use backup file matching firmware version (512 bytes).',
      'selectCalibFile': 'Select calibration backup file (.dat)',
      'restoreCalib': 'Restore Calibration',
      
      // Calib check modal
      'calibCheckTitle': 'Calibration Data Check',
      'calibCheckDesc': 'Calibration Interpretation',
      'calibCheckOffical': 'Official/v4+ Calib Addr(0x1E00)',
      'calibCheckThird': 'Third-party/v5+ Calib Addr(0xB000)',
      'calibCheckBackup': 'Backup Calibration',
      'calibCheckConfirm': 'Confirm Restore',
      'calibCheckCancel': 'Cancel',
      'calibReadDevice': 'Read Device Calibration',
      'calibLoadBackup': 'Load Backup Calibration File',
      'calibExportBackup': 'Export Backup Calibration',
      'calibWriteOfficial': 'Write to Official Address',
      'calibWriteThirdParty': 'Write to Third-party Address',
      
      // Flash device warning modal
      'flashWarningTitle': 'Confirm Device Model Before Flashing',
      'flashWarningText1': 'Check if your device is Quansheng UVK1, UVK5/UVK6 V3 version. Older UVK5 and UVK6 will brick!!!',
      'flashWarningText2': 'Check method: Look for V3 on back label, or ask seller.',
      'flashWarningOk': 'I understand',
      
      // Snow screen warning HTML
      'snowScreenWarningHtml': 'If you see snow screen after flashing V5.0.0, calibration is the issue. Download and flash calibration',
      
      // Help page
      'helpBack': 'Back',
      'helpTitle': 'Help Documentation',
      'helpTocLoading': 'Loading...',
      'helpContentLoading': 'Loading documentation...',
      'helpTocEmpty': 'No TOC',
      'helpContentError': 'Failed to load document',
      'helpContentErrorHint': 'Please ensure data/help.md file exists.',
      'helpTocError': 'Load failed',
      
      // Language switcher
      'langSwitchTitle': 'Switch Language',
      'langSwitchZh': '中文',
      'langSwitchEn': 'English',
      'langSwitchLabel': '中文',
      
      // Write frequency
      'freqChannel': 'Channel',
      'freqReceive': 'RX Frequency',
      'freqTransmit': 'TX Frequency',
      'freqOffset': 'Offset',
      'freqMode': 'Mode',
      'freqName': 'Name',
      'freqPower': 'Power',
      'freqBandwidth': 'Bandwidth',
      'freqAdd': 'Add',
      'freqRead': 'Read',
      'freqWrite': 'Write',
      'freqClear': 'Clear',
      'writefreqNote': 'Note: Frequency programming is for Dondji firmware only. Missing characters:',
      'clickSupplement': 'Click to submit',
      'readFromDevice': 'Read from Device',
      'writeToDevice': 'Write to Device',
      'exportExcel': 'Export Excel',
      'importExcel': 'Import Excel',
      'dragSort': 'Drag to sort',
      'freqNameSub': 'UTF-8 max 15 bytes',
      'freqMHz': 'MHz',
      'freqPowerSub': '(Exclude USER)',
      'freqRxDCS': 'RX DCS',
      'freqRxCTCSS': 'RX CTCSS',
      'freqTxDCS': 'TX DCS',
      'freqTxCTCSS': 'TX CTCSS',
      'freqDCS': 'DCS',
      'freqCTCSS': 'CTCSS',
      'freqOffsetDir': 'Offset Dir',
      'freqOffsetDirSub': 'MENU Offset Dir',
      'freqModeSub': 'FM/AM/USB',
      'freqStep': 'Step',
      'freqkHz': 'kHz',
      'freqList': 'Channel List',
      'freqListSub': 'Multi-select',
      'freqNoParticipate': 'Not Participate',
      'freqScanlistAll': 'All',
      'freqScanlistItem': 'List {num}',
      'clearRow': 'Clear row',
      'prevPage': 'Previous',
      'nextPage': 'Next',
      'freqPageInfoText': 'Total {total} · Filled {filled} · Page {cur} / {pages} · {size} channels per page',
    
    // Log messages
    'logRequestSerial': 'Requesting serial port...',
    'logConnected': 'Connected',
    'logDisconnected': 'Disconnected',
    'logWaitingDevice': 'Waiting for device...',
    'logDeviceInfo': 'Device info: ',
    'logUid': 'UID: ',
    'logBootloader': 'Bootloader: ',
    'logFirmwareInfo': 'Firmware v{ver}: Calibration base address {addr}',
    'logDeviceInfoHex': 'Device info(hex): ',
    'logRequestingDeviceInfo': 'Requesting device info ({purpose})...',
    'logReceivedMessage': 'Received message: 0x{type}',
    'logDeviceReady': 'Device ready ({purpose} session)',
    'logFirmwareLoaded': 'Firmware loaded: {name} ({size} bytes)',
    'logStartFlash': 'Starting firmware flash...',
    'logFlashProgress': 'Page {page}/{total}',
    'logFlashComplete': 'Firmware flash complete!',
    'logError': 'Error: {msg}',
    'logLoadingFile': 'Loading...{name}',
    'logFirmwareLoadedDefault': 'Firmware loaded: {name} ({size} bytes)',
    'logLoadFailed': 'Load failed: {msg}',
    'logFontLoaded': 'Font loaded: {name} ({size} bytes)',
    'logFontLoadedDefault': 'Font loaded: {name} ({size} bytes)',
    'logDetectDevice': 'Detecting device mode...',
    'logDeviceCustomFirmware': 'Device running custom firmware, starting font flash...',
    'logRetry': 'Retry @ 0x{addr} ({retry})',
    'logWrittenBytes': 'Written {written}/{total} bytes',
    'logFontFlashComplete': 'Font flash complete! Total {size} bytes',
    'logVersionWriteTimeout': 'Version write timeout (firmware may not support SPI Flash write)',
    'logVersionWritten': 'Version written',
    'logVerifyPass': 'Verification passed: font data correct',
    'logVerifyWarning': 'Verification warning: first bytes 0x{w0} 0x{w1} (expected 0x1100 0x2100)',
    'logVerifySkip': 'Verification skipped: read timeout',
    'logExportCalibration': 'Exporting calibration data...',
    'logCalibrationExportComplete': 'Calibration data export complete',
    'logFileSizeError': 'File size error: {size} (expected {expected})',
    'logCalibrationFileLoaded': 'Calibration file loaded: {name}',
    'logRestoreCalibration': 'Restoring calibration data...',
    'logCalibrationRestoreComplete': 'Calibration restore complete! Rebooting...',
    'logDeviceRebooted': 'Device rebooted',
    'logFirmwareCalibBase': 'Firmware v{ver}: calibration base 0x{addr}',
    'logReadCalibration': 'Reading calibration @ 0x{addr}...',
    'logCalibrationReadComplete': 'Calibration read complete @ 0x{addr}',
    'logDeviceCalibrationReadComplete': 'Device calibration read complete',
    'logBackupCalibrationLoaded': 'Backup calibration loaded: {name}',
    'logNoBackupCalibration': 'No backup calibration data to export',
    'logBackupCalibrationExported': 'Backup calibration exported',
    'logWriteCalibration': 'Writing calibration to 0x{addr}...',
    'logCalibrationWriteComplete': 'Calibration write complete! Rebooting...',
    'logNoBackupCalibrationToWrite': 'No backup calibration data to write',
    'logExportConfig': 'Exporting config data...',
    'logConfigExportComplete': 'Config data export complete',
    'logConfigFileLoaded': 'Config file loaded: {name}',
    'logRestoreConfig': 'Restoring config data...',
    'logConfigRestoreComplete': 'Config restore complete! Rebooting...',
    'logWebSerialUnsupported': 'Browser does not support Web Serial API, please use Chrome/Edge/Opera',
    'logWritefreqReadFailed': 'Writefreq read failed: {msg}',
    'logWritefreqReadSuccess': 'Table cleared and filled {count} channels from device (skipped unused slots and invalid slots: RX range, valid power, band matches frequency, parseable tones); scanned {scanned} slots',
    'logChannelNameTruncate': 'Channel name truncate warning (≤15 bytes UTF-8, excess ends with ...):\n{warn}',
    'logValidationFailed': 'Validation failed, not written to device',
    'logWritefreqSuccess': 'Successfully written {count} filled channels; remaining {empty} erased as unused',
    'logRebootingDevice': 'Rebooting device to load new channel data...',
    'logRebootSent': 'Reboot command sent (device will auto-reset)',
    'logWritefreqWriteFailed': 'Writefreq write failed: {msg}',
    'logSheetJSNotLoaded': 'SheetJS not loaded, using CSV export instead',
    'logCsvExportSuccess': 'CSV exported (only filled Rx frequency channels, {count} rows)',
    'logExcelExportSuccess': 'Excel exported (only filled Rx frequency channels, {count} rows)',
    'logTableEmpty': 'Table empty or missing headers',
    'logCsvImportSuccess': 'CSV imported ({count} rows, {valid} valid channels)',
    'logFirstRowInvalid': 'First row channel number invalid',
    'logRowChannelInvalid': 'Row {row} data: channel number invalid',
    'logRowChannelOutOfBounds': 'Row {row} data: channel number out of bounds',
    'logLoadSheetJSFirst': 'Please load SheetJS first or use CSV import',
    'logImportFailed': 'Import failed: {msg}',
    'logSelectImageFirst': 'Please select an image first',
    'logConnectFailed': 'Connection failed: {msg}',
    'logUploadingBootLogo': 'Uploading boot logo...',
    'logWrittenBytesProgress': 'Written {written}/{total} bytes',
    'logBootLogoUploadSuccess': 'Boot logo uploaded successfully! Please select "Custom" in menu "Boot Screen"',
    'logUploadFailed': 'Upload failed: {msg}',
    'logReadingBootLogo': 'Reading boot logo...',
    'logBootLogoReadSuccess': 'Boot logo read successfully',
    'logReadFailed': 'Read failed: {msg}',
    'logVerifyWarning': 'Verification warning: first bytes 0x{w0} 0x{w1} (expected 0x1100 0x2100)',
    'logReadCalibrationAddr': 'Reading calibration @ 0x{addr}...',
    'logCalibrationReadAddrComplete': 'Calibration read complete @ 0x{addr}',
    'logDeviceCalibrationReadComplete': 'Device calibration read complete',
    'logBackupCalibrationLoaded': 'Backup calibration loaded: {name}',
    'logNoBackupCalibrationExport': 'No backup calibration data to export',
    'logBackupCalibrationExported': 'Backup calibration exported',
    'logWriteCalibrationAddr': 'Writing calibration to 0x{addr}...',
    'logCalibrationWriteComplete': 'Calibration write complete! Rebooting...',
    'logNoBackupCalibrationWrite': 'No backup calibration data to write',
    'logExportConfig': 'Exporting config data...',
    'logConfigExportComplete': 'Config data export complete',
    'logConfigFileSizeError': 'File size error: {size} (expected {expected})',
    'logConfigFileLoaded': 'Config file loaded: {name}',
    'logRestoreConfig': 'Restoring config data...',
    'logConfigRestoreComplete': 'Config restore complete! Rebooting...',
    
    // Visitor statistics
    'visitorTotalPrefix': 'Total',
    'visitorTotalSuffix': 'Hams have visited this site',
    'visitorTodayPrefix': 'Today you are flashing with',
    'visitorTodaySuffix': 'people',
      'writeSuccess': 'Write successful: {count} filled channels; {empty} erased as unused',
      'exportCSVSucc': 'CSV exported (filled channels only, {count} rows)',
      'exportExcelSucc': 'Excel exported (filled channels only, {count} rows)',
      
      // Writefreq table placeholders and options
      'freqNamePlaceholder': 'ASCII or Chinese, ≤15 bytes UTF-8',
      'freqRxPlaceholder': 'Ex: 438.500000',
      'freqOffsetPlaceholder': 'Ex: 5.000000',
      'freqSelectPower': 'Select Power',
      'freqNoParticipate': 'Not Participate',
      'freqValidationError': 'Please select power (USER excluded)',
      'freqSftClose': 'Close',
      'freqSftPlus': '+',
      'freqSftMinus': '−',
      'freqScanlistAll': 'All',
      
      // Logo
      'logoDesc': 'Upload custom boot logo (128x64 pixels, black and white bitmap)',
      'logoDesc1': 'Upload custom 128×64 boot logo. Supports PNG, JPG, BMP formats. Image will be scaled and converted to monochrome bitmap. Select "Custom" in menu "Boot Logo" to display.',
      'logoWarning': '⚠️ Please enter normal usage interface first before connecting USB (same as font flash, calibration backup, no BOOT mode needed).',
      'logoFile': 'Image file (.bmp)',
      'logoFlash': 'Flash Boot Logo',
      'logoDefault': 'Restore Default Logo',
      
      // Backup config
      'backupCfgDesc': 'Backup current device configuration',
      'backupCfgDesc1': 'Backup device config data (menu settings, key settings etc.) for quick restore after factory reset.',
      'backupCfgWarning': '⚠️ Please enter normal usage interface first before connecting USB for backup.',
      'backupCfgUsage': 'Usage: Backup config → flash other firmware and back → cleared all data → restore via "Restore Config".',
      'backupCfg': 'Backup Config',
      'exportCfgData': 'Export Config Data',
      'downloadCfgBackup': 'Download config_backup.dat',
      
      // Restore config
      'restoreCfgDesc': 'Restore previously backed up config data to quickly restore all menu settings.',
      'restoreCfgWarning': '⚠️ Please enter normal usage interface first before connecting USB; device auto-reboots after write.',
      'restoreCfgFile': 'Select config file (.dat)',
      'selectCfgBackup': 'Select config backup file (.dat)',
      'restoreCfg': 'Restore Config',
      'restoreCfgData': 'Restore Config Data',
      'selectFile': 'Select File',
      
      // Toolbox
      'toolboxDesc': 'Emergency toolbox for fixing common issues',
      'toolboxDesc2': 'Emergency toolbox provides calibration files and stock firmware downloads for unbricking or factory restore.',
      'toolboxReset': 'Reset Device',
      'toolboxReboot': 'Reboot Device',
      'toolboxDowngrade': 'Flash Stock to Unlock Freq Password',
      'clickDetail': 'Click for details',
      'toolboxK1Calib': 'K1 Calibration File',
      'toolboxK6Calib': 'K6 V3 Calibration File',
      'toolboxK1StockEn': 'K1 Stock English Firmware',
      'toolboxK1StockEnV703': 'K1 Stock English Firmware',
      'toolboxK5V3StockEn': 'K5 V3 Stock English Firmware',
      'toolboxK1StockCn': 'K1 Stock Firmware',
      'toolboxK5V3StockCn': 'K5 V3 Stock Firmware',
      'toolboxK1CPS': 'K1 Official CPS',
      
      // Downgrade guide modal
      'downgradeTitle': '🔧 Restore Stock Unlock Password',
      'downgradeSubtitle': 'Dondji downgrade stock unlock password:',
      'downgradeStep1Title': 'Restore factory settings:',
      'downgradeStep1Content': 'Hold "PTT + Side Key 1" to power on, select: Factory Reset -> All -> Confirm.',
      'downgradeStep2Title': 'Downgrade to V4.3.2:',
      'downgradeStep2Content': 'Flash "f4hwn.IOTCU_K1_v4.323_b02.bin" firmware',
      'downgradeStep3Title': 'Restore downgrade config:',
      'downgradeStep3Content1': 'Restore "降级配置.bin"',
      'downgradeStep3Content2': 'Config restore URL:',
      'downgradeStep3Note': 'Note: Progress normally stalls at 50%-60%, just power off directly, no need to repeat.',
      'downgradeStep4Title': 'Restore factory settings:',
      'downgradeStep4Content': 'Hold PTT + Side Key 1 to power on, select: Factory Reset -> All Parameters -> Confirm.',
      'downgradeStep5Title': 'Update stock firmware via CPS:',
      'downgradeStep5Content': 'Device -> Upgrade Program -> Upgrade.',
      'downgradeFiles': 'Required files download:',
      'downgradeConfigBin': '降级配置.bin',
      'toolboxStockFw': 'K1 Stock Firmware',
      'toolboxK6StockFw': 'K6 V3 Stock Firmware',
      
      // Logo tab
      'selectImage': 'Select Image',
      'threshold': 'Threshold',
      'invert': 'Invert',
      'invertBW': 'Invert Black/White',
      'preview': 'Preview',
      'uploadLogo': 'Upload Boot Logo',
      'readLogo': 'Read Boot Logo',
      'downloadLogo': 'Download boot_logo.png',
      
      // Log
      'showLog': 'Show Log',
      'hideLog': 'Hide Log',
      
      // Footer and aside
      'note': 'Note:',
      'browserNote': 'Requires Chrome / Edge / Opera browser.',
      'flashFirmwareBoot': 'Only firmware flash',
      'enterBootMode': 'requires entering BOOT mode (hold PTT while powering on).',
      'otherOperations': 'Font flash, calibration backup/restore, boot logo',
      'normalMode': 'all operate in normal usage interface with USB connection.',
      'footerDev': 'Firmware & Tools developed by BD1AHN',
      'footerUVTools': 'UVTools2',
      
      // Coffee modal
      'coffeeTitle': '☕ Buy Author a Coffee',
      'coffeeText1': 'Thanks for using Dondji firmware! Maintaining this website and firmware requires significant time and effort. Operations, development, testing all need cost support.',
      'coffeeText2': 'If this project helps you, welcome to donate! Please note your callsign when donating, so I can thank you!',
      'wechatPay': 'WeChat Pay',
      'aliPay': 'Alipay',
      'donationBoard': 'Donation Board',
      'donationTime': 'Time',
      'donationName': 'Name',
      'donationAmount': 'Amount',
      'donationMessage': 'Message',
      'donationSource': 'Source',
      'loading': 'Loading...',
      'loadingFile': 'Loading...',
      'noDonationRecord': 'No donation records',
      'coffeeNotice': 'Donate what you can, heart matters most, no comparison, voluntary only.',
      'exit': 'Exit',
      'coffeeDesc': 'Thank you for your support!',
      'donationList': 'Donation List',
      'close': 'Close',
      
      // Misc
      'connecting': 'Connecting...',
      'connected': 'Connected',
      'disconnected': 'Disconnected',
      'success': 'Success',
      'error': 'Error',
      'confirm': 'Confirm',
      'cancel': 'Cancel'
    }
  };

  // Current language
  let currentLang = 'zh';

  // Get translation for a key
  function t(key, params = {}) {
    const langData = translations[currentLang] || translations.zh;
    let text = langData[key] || translations.zh[key] || key;
    
    // Replace placeholders like {total}, {filled}, etc. with actual values
    if (params && typeof params === 'object') {
      Object.keys(params).forEach(paramKey => {
        const regex = new RegExp('\\{' + paramKey + '\\}', 'g');
        text = text.replace(regex, String(params[paramKey]));
      });
    }
    
    return text;
  }

  // Export translation function globally
  window.t = t;
  window.getCurrentLang = () => currentLang;

  // Apply translations to elements with data-i18n attribute
  function applyTranslations() {
    document.querySelectorAll('[data-i18n]').forEach(el => {
      const key = el.getAttribute('data-i18n');
      const params = el.getAttribute('data-i18n-params');
      let paramObj = {};
      if (params) {
        try {
          paramObj = JSON.parse(params);
        } catch (e) {}
      }
      el.textContent = t(key, paramObj);
    });

    document.querySelectorAll('[data-i18n-title]').forEach(el => {
      const key = el.getAttribute('data-i18n-title');
      el.title = t(key);
    });

    document.querySelectorAll('[data-i18n-aria]').forEach(el => {
      const key = el.getAttribute('data-i18n-aria');
      el.setAttribute('aria-label', t(key));
    });

    document.querySelectorAll('[data-i18n-placeholder]').forEach(el => {
      const key = el.getAttribute('data-i18n-placeholder');
      el.placeholder = t(key);
    });

    // Update dynamic content: writefreq table and pagination
    if (window.writefreqRebuildRows) {
      window.writefreqRebuildRows();
    }
    if (window.writefreqUpdatePaginationUI) {
      window.writefreqUpdatePaginationUI();
    }

    // Update page title
    const titleEl = document.querySelector('title');
    if (titleEl) {
      if (document.body.classList.contains('help-page')) {
        titleEl.textContent = t('pageTitleHelp');
      } else {
        titleEl.textContent = t('pageTitle');
      }
    }

    // Update html lang attribute
    document.documentElement.lang = currentLang;
  }

  // Switch language
  function switchLanguage(lang) {
    if (lang === currentLang) return;
    currentLang = lang;
    
    // Save preference
    try {
      localStorage.setItem('uvk1-web-lang', lang);
    } catch (e) {}
    
    // Apply translations
    applyTranslations();
    
    // Update button text
    const langBtn = document.getElementById('langSwitchBtn');
    if (langBtn) {
      const langText = langBtn.querySelector('.lang-switch__text');
      if (langText) {
        langText.textContent = lang === 'zh' ? 'EN' : '中文';
      }
    }

    // Dispatch event for other scripts to react
    window.dispatchEvent(new CustomEvent('langchange', { detail: { lang } }));
  }

  // Toggle language
  function toggleLanguage() {
    switchLanguage(currentLang === 'zh' ? 'en' : 'zh');
  }

  // Initialize language from storage
  function initLanguage() {
    try {
      const savedLang = localStorage.getItem('uvk1-web-lang');
      if (savedLang && (savedLang === 'zh' || savedLang === 'en')) {
        currentLang = savedLang;
      }
    } catch (e) {}

    // Apply initial translations
    applyTranslations();

    // Update button text
    const langBtn = document.getElementById('langSwitchBtn');
    if (langBtn) {
      const langText = langBtn.querySelector('.lang-switch__text');
      if (langText) {
        langText.textContent = currentLang === 'zh' ? 'EN' : '中文';
      }
    }
  }

  // Create language switch button
  function createLangSwitchButton() {
    const langBtn = document.getElementById('langSwitchBtn');
    if (langBtn) {
      langBtn.addEventListener('click', toggleLanguage);
    }
  }

  // Initialize on DOM ready
  function init() {
    initLanguage();
    createLangSwitchButton();
  }

  // Export functions for external use
  window.i18n = {
    t,
    switchLanguage,
    getCurrentLang: () => currentLang,
    applyTranslations
  };

  // Run init
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();