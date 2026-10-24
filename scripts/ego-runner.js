#!/usr/bin/env node
/**
 * ego-runner.js
 * Ultra-fast bridge to execute automation tasks on ego (lite) browser via its embedded Node.js engine.
 *
 * Usage:
 *   node scripts/ego-runner.js --code "const task = await taskSpace('demo'); ..."
 *   node scripts/ego-runner.js --open "https://google.com" --snapshot
 */

import { spawn } from 'node:child_process';

const EGO_CLI = 'C:\\Program Files\\Citro Labs\\ego lite\\Application\\0.6.0.12\\ego-browser.exe';

export function runEgoScript(jsCode) {
  return new Promise((resolve, reject) => {
    const child = spawn(EGO_CLI, ['nodejs'], {
      stdio: ['pipe', 'pipe', 'pipe'],
      windowsHide: true,
    });

    let stdout = '';
    let stderr = '';

    child.stdout.on('data', (d) => {
      stdout += d.toString();
    });
    child.stderr.on('data', (d) => {
      stderr += d.toString();
    });

    child.on('close', (code) => {
      if (code === 0) {
        resolve({ stdout: stdout.trim(), stderr: stderr.trim() });
      } else {
        reject(new Error(`ego-browser exited with code ${code}\nStderr: ${stderr}\nStdout: ${stdout}`));
      }
    });

    child.on('error', (err) => {
      reject(err);
    });

    child.stdin.write(jsCode);
    child.stdin.end();
  });
}

// CLI argument parsing
const args = process.argv.slice(2);
if (args.length > 0) {
  if (args[0] === '--code' && args[1]) {
    runEgoScript(args[1])
      .then((res) => {
        if (res.stdout) console.log(res.stdout);
        if (res.stderr) console.error(res.stderr);
      })
      .catch((err) => {
        console.error(err.message);
        process.exit(1);
      });
  } else if (args[0] === '--open' && args[1]) {
    const url = args[1];
    const wantSnapshot = args.includes('--snapshot');
    const nameIdx = args.indexOf('--name');
    const taskName = nameIdx !== -1 ? args[nameIdx + 1] : 'ego-quick-task';
    const keep = args.includes('--keep');

    const code = `
const task = await taskSpace(${JSON.stringify(taskName)});
const page = task.page('p1');
await page.goto(${JSON.stringify(url)}, { waitUntil: 'load' });
const title = await page.title();
console.log('Title:', title);
${wantSnapshot ? "console.log(await page.snapshot());" : ""}
await task.finish({ keep: ${keep ? "['p1']" : "[]"} });
`;
    runEgoScript(code)
      .then((res) => {
        if (res.stdout) console.log(res.stdout);
        if (res.stderr) console.error(res.stderr);
      })
      .catch((err) => {
        console.error(err.message);
        process.exit(1);
      });
  } else {
    console.log(`
Usage:
  node scripts/ego-runner.js --code "<code>"
  node scripts/ego-runner.js --open <url> [--snapshot] [--keep] [--name <taskName>]
    `);
  }
}
