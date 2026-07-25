#!/usr/bin/env node
'use strict';

const fs = require('fs');
const path = require('path');

const HEADER = `/*
 * Dondji Firmware
 *
 * Copyright (c) 2026 BD1AHN
 *
 * Licensed under the Apache License, Version 2.0
 *
 * Project:
 *     叮咚鸡 (Dondji)
 *
 * Maintainer:
 *     BD1AHN
 *
 * Official Website:
 *     https://ethanyan6.github.io/Dondji/
 *
 * The Dondji name, logo, and official project identity
 * are protected separately from the source code license.
 */

`;

const MARKER = 'Dondji Firmware';
const docsRoot = path.resolve(__dirname, '..');
const targets = process.argv.slice(2).map((p) => path.resolve(docsRoot, p));

if (targets.length === 0) {
  console.error('Usage: node js/_prepend_license.js <file>...');
  process.exit(1);
}

for (const file of targets) {
  if (!fs.existsSync(file)) {
    console.error('MISSING', file);
    process.exitCode = 1;
    continue;
  }
  const raw = fs.readFileSync(file, 'utf8');
  if (raw.slice(0, 1500).includes(MARKER)) {
    console.log('skip', path.relative(docsRoot, file));
    continue;
  }
  const out = HEADER + (raw.startsWith('\n') ? '' : '\n') + raw;
  fs.writeFileSync(file, out, 'utf8');
  console.log('ok', path.relative(docsRoot, file));
}
