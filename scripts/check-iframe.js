import { runEgoScript } from './ego-runner.js';

const code = `
(async () => {
  const task = await taskSpace('check-iframe');
  const page = task.page('p1');
  await page.goto('https://console.firebase.google.com/u/0/project/autodoor-rf/database/autodoor-rf-default-rtdb/data', { waitUntil: 'load' });
  await page.waitForTimeout(6000);

  const iframes = await page.evaluate(() => {
    return Array.from(document.querySelectorAll('iframe')).map(f => f.src);
  });
  console.log('IFRAMES:' + JSON.stringify(iframes));
  await task.finish({ keep: [] });
})();
`;

runEgoScript(code)
  .then(r => {
    if (r.stderr) console.log('ERR:', r.stderr);
  })
  .catch(e => console.error('CATCH:', e));

