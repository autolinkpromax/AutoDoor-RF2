#!/usr/bin/env node

/**
 * AI Pass Fast CLI Bridge (Chrome DevTools Protocol)
 * Direct, instant communication with de.aipass.net/chat via port 9222.
 * Execution time: 100x faster than browser subagent, 0 subagent tokens!
 */

import fs from 'fs';

// Helper to send CDP command
function sendCommand(ws, method, params = {}) {
  return new Promise((resolve, reject) => {
    const id = Math.floor(Math.random() * 1000000);
    const handler = (evt) => {
      try {
        const msg = JSON.parse(evt.data);
        if (msg.id === id) {
          ws.removeEventListener('message', handler);
          if (msg.error) reject(new Error(msg.error.message));
          else resolve(msg.result);
        }
      } catch (e) {
        // Ignore other messages
      }
    };
    ws.addEventListener('message', handler);
    ws.send(JSON.stringify({ id, method, params }));
  });
}

async function evaluate(ws, expression) {
  const res = await sendCommand(ws, 'Runtime.evaluate', {
    expression,
    returnByValue: true,
    awaitPromise: true
  });
  return res?.result?.value;
}

async function findAiPassTab() {
  const res = await fetch('http://127.0.0.1:9222/json');
  const pages = await res.json();
  const page = pages.find(p => p.url && p.url.includes('aipass.net/chat'));
  if (!page) {
    throw new Error('No open AI Pass chat tab found on Chrome (port 9222). Please ensure de.aipass.net/chat is open.');
  }
  return page;
}

export async function askAiPass(prompt, options = {}) {
  const page = await findAiPassTab();
  const ws = new WebSocket(page.webSocketDebuggerUrl);

  await new Promise((resolve, reject) => {
    ws.onopen = resolve;
    ws.onerror = reject;
  });

  try {
    // 1. Switch model if requested
    if (options.model) {
      const targetModel = options.model.toLowerCase();

      // Check current model button text
      const currentModel = await evaluate(ws, `
        (() => {
          const btn = Array.from(document.querySelectorAll('button')).find(b => 
            b.className.includes('bg-bg-input-control') && (
              b.innerText.includes('Auto') || 
              b.innerText.includes('Claude') || 
              b.innerText.includes('GPT') || 
              b.innerText.includes('Gemini') ||
              b.innerText.includes('Kimi') ||
              b.innerText.includes('DeepSeek') ||
              b.innerText.includes('Opus')
            )
          );
          return btn ? btn.innerText.trim() : '';
        })()
      `);

      if (currentModel.toLowerCase().includes(targetModel)) {
        console.log(`[AIPass-CLI] Model "${options.model}" is already active.`);
      } else {
        console.log(`[AIPass-CLI] Switching model from "${currentModel}" to "${options.model}"...`);

        // Open modal
        await evaluate(ws, `
          (() => {
            const btn = Array.from(document.querySelectorAll('button')).find(b => 
              b.className.includes('bg-bg-input-control') && (
                b.innerText.includes('Auto') || 
                b.innerText.includes('Claude') || 
                b.innerText.includes('GPT') || 
                b.innerText.includes('Gemini') ||
                b.innerText.includes('Kimi') ||
                b.innerText.includes('DeepSeek') ||
                b.innerText.includes('Opus')
              )
            );
            if (btn) btn.click();
          })()
        `);

        await new Promise(r => setTimeout(r, 600));

        // Find and select model
        const selected = await evaluate(ws, `
          (() => {
            const target = ${JSON.stringify(targetModel)};

            function clickSelectForTarget() {
              const allEls = Array.from(document.querySelectorAll('*'));
              const modelEl = allEls.find(el => {
                if (!el.innerText) return false;
                const txt = el.innerText.trim().toLowerCase();
                return (txt === target || (txt.includes(target) && txt.length < target.length + 15)) && el.children.length === 0;
              });

              if (!modelEl) return false;

              let card = modelEl;
              let selectBtn = null;
              for (let i = 0; i < 5; i++) {
                if (!card.parentElement) break;
                card = card.parentElement;
                selectBtn = Array.from(card.querySelectorAll('button')).find(b => b.innerText.trim() === 'เลือก');
                if (selectBtn) break;
              }

              if (selectBtn) {
                selectBtn.click();
                return true;
              }
              return false;
            }

            if (clickSelectForTarget()) return true;

            // Browse through tabs if needed
            const tabs = Array.from(document.querySelectorAll('button, div')).filter(el => 
              ['Anthropic', 'OpenAI', 'Google', 'DeepSeek', 'Moonshot AI', 'Perplexity', 'Meta', 'สนทนา', 'แนะนำ'].includes(el.innerText?.trim())
            );

            for (const tab of tabs) {
              tab.click();
              if (clickSelectForTarget()) return true;
            }

            return false;
          })()
        `);

        if (selected) {
          await new Promise(r => setTimeout(r, 300));
          await evaluate(ws, `
            (() => {
              const confirmBtn = Array.from(document.querySelectorAll('button')).find(b => b.innerText.trim() === 'ยืนยัน');
              if (confirmBtn) confirmBtn.click();
            })()
          `);
          console.log(`[AIPass-CLI] Successfully selected model: ${options.model}`);
        } else {
          console.warn(`[AIPass-CLI] Model "${options.model}" not found in list, keeping current model.`);
          await evaluate(ws, `
            (() => {
              document.dispatchEvent(new KeyboardEvent('keydown', { key: 'Escape' }));
            })()
          `);
        }
        await new Promise(r => setTimeout(r, 500));
      }
    }

    // 2. Count existing message bubbles before sending
    const initialMsgCount = await evaluate(ws, `
      document.querySelectorAll('[data-role="assistant"], .markdown-content, .prose, [class*="markdown"]').length
    `) || 0;

    // 3. Focus textarea & insert prompt using native CDP Input.insertText
    console.log('[AIPass-CLI] Sending prompt...');
    await evaluate(ws, `
      (() => {
        const ta = document.querySelector('textarea');
        if (ta) {
          ta.focus();
          ta.select();
        }
      })()
    `);

    // Insert text natively through CDP
    await sendCommand(ws, 'Input.insertText', { text: prompt });
    await new Promise(r => setTimeout(r, 300));

    // 4. Click Submit Button using data-testid="send-button"
    const clicked = await evaluate(ws, `
      (() => {
        const btn = document.querySelector('button[data-testid="send-button"]') ||
                    Array.from(document.querySelectorAll('button')).find(b => 
                      b.innerText.includes('Send message') || 
                      b.getAttribute('aria-label')?.includes('Send') ||
                      b.className.includes('bg-bg-brand-primary')
                    );
        if (btn && !btn.disabled) {
          btn.click();
          return true;
        }
        return false;
      })()
    `);

    if (!clicked) {
      console.log('[AIPass-CLI] Fallback: sending Enter key...');
      await sendCommand(ws, 'Input.dispatchKeyEvent', { type: 'rawKeyDown', windowsVirtualKeyCode: 13, key: 'Enter', code: 'Enter' });
      await sendCommand(ws, 'Input.dispatchKeyEvent', { type: 'keyUp', windowsVirtualKeyCode: 13, key: 'Enter', code: 'Enter' });
    }

    console.log('[AIPass-CLI] Waiting for generation to complete...');

    // 5. Wait for Generation to Complete
    let completed = false;
    let finalAnswer = '';
    const startTime = Date.now();
    const timeoutMs = options.timeoutMs || 120000; // 2 minutes

    // Wait for AI response bubble to appear
    let seenGenerating = false;
    let lastContent = '';
    let stableCount = 0;

    while (!completed && (Date.now() - startTime < timeoutMs)) {
      await new Promise(r => setTimeout(r, 800));

      const status = await evaluate(ws, `
        (() => {
          const stopBtn = Array.from(document.querySelectorAll('button')).some(b => 
            b.textContent.includes('หยุด') || 
            b.textContent.includes('Stop') || 
            b.getAttribute('aria-label')?.includes('Stop') ||
            b.getAttribute('data-testid') === 'stop-button'
          );
          const bubbles = Array.from(document.querySelectorAll('[data-role="assistant"], .markdown-content, .prose, [class*="markdown"]'));
          const currentCount = bubbles.length;
          const latest = currentCount > 0 ? bubbles[currentCount - 1].innerText.trim() : '';

          return {
            isGenerating: stopBtn,
            count: currentCount,
            text: latest
          };
        })()
      `);

      if (!status) continue;

      if (status.isGenerating) {
        seenGenerating = true;
      }

      // Check if new bubbles were added beyond initial count
      const hasNewBubble = status.count > initialMsgCount;

      if (hasNewBubble && status.text.length > 0) {
        // If generation stopped or content stayed stable
        if (!status.isGenerating) {
          if (status.text === lastContent) {
            stableCount++;
            if (stableCount >= 2) {
              finalAnswer = status.text;
              completed = true;
              break;
            }
          } else {
            lastContent = status.text;
            stableCount = 0;
          }
        }
      }
    }

    if (!completed) {
      throw new Error('Generation timed out or no response detected');
    }

    console.log(`[AIPass-CLI] Done! Received ${finalAnswer.length} chars.`);
    return finalAnswer;

  } finally {
    ws.close();
  }
}

// CLI runner if invoked directly from terminal
if (process.argv[1]?.endsWith('aipass.js')) {
  const args = process.argv.slice(2);
  let prompt = '';
  let model = '';
  let outputFile = '';

  for (let i = 0; i < args.length; i++) {
    if (args[i] === '-m' || args[i] === '--model') {
      model = args[++i];
    } else if (args[i] === '-o' || args[i] === '--output') {
      outputFile = args[++i];
    } else if (!prompt) {
      prompt = args[i];
    }
  }

  if (!prompt) {
    console.log('Usage: node scripts/aipass.js "<prompt>" [-m "<model_name>"] [-o "<output_file>"]');
    process.exit(1);
  }

  askAiPass(prompt, { model })
    .then(answer => {
      if (outputFile) {
        fs.writeFileSync(outputFile, answer, 'utf8');
        console.log(`Saved output to ${outputFile}`);
      } else {
        console.log('\n--- ANSWER START ---\n');
        console.log(answer);
        console.log('\n--- ANSWER END ---\n');
      }
    })
    .catch(err => {
      console.error('Error:', err.message);
      process.exit(1);
    });
}
