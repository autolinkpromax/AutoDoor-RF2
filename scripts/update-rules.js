import { runEgoScript } from './ego-runner.js';

const code = `
(async () => {
  const task = await taskSpace('update-rules');
  const page = task.page('p1');
  await page.goto('https://console.firebase.google.com/u/0/project/autodoor-rf/database/autodoor-rf-default-rtdb/rules', { waitUntil: 'load' });
  await page.waitForTimeout(6000);

  // Click inside the editor box
  // The editor box is around coordinates (400, 350)
  await page.click('div.view-lines, .monaco-editor, pre');
  await page.waitForTimeout(500);

  // Select all (Ctrl+A)
  await page.keyboard.press('Control+KeyA');
  await page.waitForTimeout(200);

  // Type the new rules
  const newRules = JSON.stringify({
    rules: {
      ".read": true,
      ".write": true
    }
  }, null, 2);

  await page.keyboard.press('Backspace');
  await page.keyboard.type(newRules);
  await page.waitForTimeout(1000);

  // Look for Publish button
  const publishBtn = await page.evaluate(() => {
    const btns = Array.from(document.querySelectorAll('button'));
    const pub = btns.find(b => b.innerText.includes('Publish') || b.innerText.includes('เผยแพร่'));
    if (pub) {
      pub.click();
      return true;
    }
    return false;
  });
  console.log('CLICKED_PUBLISH:' + publishBtn);

  await page.waitForTimeout(3000);
  await page.screenshot({ path: 'C:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/scripts/rules_published.png' });
  await task.finish({ keep: [] });
})();
`;

runEgoScript(code)
  .then(r => {
    if (r.stderr) console.log('ERR:', r.stderr);
  })
  .catch(e => console.error('CATCH:', e));

