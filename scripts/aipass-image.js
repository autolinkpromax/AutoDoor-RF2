#!/usr/bin/env node

/**
 * AI Pass Fast Image Generation CLI (Chrome DevTools Protocol)
 * Direct, instant image generation with de.aipass.net/chat via port 9222.
 * 
 * Features:
 * - 0 subagent tokens: runs completely outside LLM context
 * - Ultra-fast: connects via native WebSocket CDP to Chrome
 * - Automatically switches to image generation models (Nano Banana 2, Seedream, GPT-Image, etc.)
 * - Automatically downloads the generated high-resolution image to disk
 */

import fs from 'fs';
import path from 'path';

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

export async function generateAiPassImage(prompt, options = {}) {
  const page = await findAiPassTab();
  const ws = new WebSocket(page.webSocketDebuggerUrl);

  await new Promise((resolve, reject) => {
    ws.onopen = resolve;
    ws.onerror = reject;
  });

  try {
    const targetModel = options.model || 'Nano Banana 2';
    console.log(`[AIPass-Image] Preparing image model: ${targetModel}...`);

    // 1. Open model selector modal if not already selected
    const currentModel = await evaluate(ws, `
      (() => {
        const btn = Array.from(document.querySelectorAll('button')).find(b => 
          b.innerText && (b.innerText.includes('Banana') || b.innerText.includes('Seedream') || b.innerText.includes('GPT-Image') || b.innerText.includes('Claude') || b.innerText.includes('GPT'))
        );
        return btn ? btn.innerText.trim() : '';
      })()
    `);

    if (!currentModel.toLowerCase().includes(targetModel.toLowerCase())) {
      console.log(`[AIPass-Image] Opening model switcher (currently on ${currentModel})...`);
      await evaluate(ws, `
        (() => {
          const btn = Array.from(document.querySelectorAll('button')).find(b => 
            b.innerText && (b.innerText.includes('Banana') || b.innerText.includes('Seedream') || b.innerText.includes('GPT-Image') || b.innerText.includes('Claude') || b.innerText.includes('GPT') || b.getAttribute('aria-haspopup') === 'menu')
          );
          if (btn) btn.click();
        })()
      `);
      await new Promise(r => setTimeout(r, 600));

      // Switch to "สร้างรูปภาพ" tab
      console.log('[AIPass-Image] Switching to สร้างรูปภาพ tab...');
      await evaluate(ws, `
        (() => {
          const tabs = Array.from(document.querySelectorAll('button, div, [role="tab"]'));
          const imgTab = tabs.find(el => el.innerText && el.innerText.trim() === 'สร้างรูปภาพ');
          if (imgTab) imgTab.click();
        })()
      `);
      await new Promise(r => setTimeout(r, 600));

      // Select target model
      console.log(`[AIPass-Image] Selecting model "${targetModel}"...`);
      const selected = await evaluate(ws, `
        (() => {
          const cards = Array.from(document.querySelectorAll('div, [role="dialog"] div')).filter(el => 
            el.innerText && el.innerText.toLowerCase().includes("${targetModel.toLowerCase()}") && el.querySelector('button')
          );
          for (const card of cards) {
            const btns = Array.from(card.querySelectorAll('button'));
            const selectBtn = btns.find(b => b.innerText && b.innerText.trim() === 'เลือก');
            if (selectBtn) {
              selectBtn.click();
              return true;
            }
          }
          // Fallback: search any select button
          const allBtns = Array.from(document.querySelectorAll('button')).filter(b => b.innerText.trim() === 'เลือก');
          for (const b of allBtns) {
            if (b.closest('div')?.innerText.toLowerCase().includes("${targetModel.toLowerCase()}")) {
              b.click();
              return true;
            }
          }
          return false;
        })()
      `);

      if (!selected) {
        console.warn(`[AIPass-Image] Warning: Could not find exact button for ${targetModel}, using default.`);
        await evaluate(ws, 'document.body.click();'); // close modal
      }
      await new Promise(r => setTimeout(r, 500));
    }

    // 2. Type image prompt into textarea
    console.log('[AIPass-Image] Setting prompt into textarea...');
    const escapedPrompt = JSON.stringify(prompt);
    const typed = await evaluate(ws, `
      (() => {
        const ta = document.querySelector('textarea');
        if (!ta) return false;
        const nativeSetter = Object.getOwnPropertyDescriptor(window.HTMLTextAreaElement.prototype, 'value').set;
        nativeSetter.call(ta, ${escapedPrompt});
        ta.dispatchEvent(new Event('input', { bubbles: true }));
        ta.dispatchEvent(new Event('change', { bubbles: true }));
        return true;
      })()
    `);

    if (!typed) throw new Error('Could not find textarea on AI Pass chat page');

    await new Promise(r => setTimeout(r, 300));

    // 3. Click send button or send Enter
    console.log('[AIPass-Image] Submitting prompt...');
    const sent = await evaluate(ws, `
      (() => {
        const sentImg = document.querySelector('img[src*="icon-sent"]');
        if (sentImg) {
          const btn = sentImg.closest('button');
          if (btn && !btn.disabled) {
            btn.click();
            return true;
          }
        }
        const submitBtn = document.querySelector('button[type="submit"]');
        if (submitBtn && !submitBtn.disabled) {
          submitBtn.click();
          return true;
        }
        return false;
      })()
    `);

    if (!sent) {
      await sendCommand(ws, 'Input.dispatchKeyEvent', { type: 'rawKeyDown', windowsVirtualKeyCode: 13, key: 'Enter', code: 'Enter' });
      await sendCommand(ws, 'Input.dispatchKeyEvent', { type: 'keyUp', windowsVirtualKeyCode: 13, key: 'Enter', code: 'Enter' });
    }

    // 4. Wait for generation to start and complete
    console.log('[AIPass-Image] Generation started! Waiting for image output...');
    let finalImageUrl = null;
    const startTime = Date.now();
    const timeoutMs = options.timeoutMs || 180000; // 3 minutes for high-res generation

    // Initial wait
    await new Promise(r => setTimeout(r, 3000));

    while (!finalImageUrl && (Date.now() - startTime < timeoutMs)) {
      const status = await evaluate(ws, `
        (() => {
          const stopBtn = Array.from(document.querySelectorAll('button')).some(b => 
            b.innerText && (b.innerText.includes('หยุด') || b.innerText.includes('Stop'))
          );

          // Find generated image (exclude avatars, icons, logo)
          const imgs = Array.from(document.querySelectorAll('img'))
            .filter(img => {
              const src = img.src || '';
              const alt = img.alt || '';
              const w = img.naturalWidth || img.width || 0;
              return (
                alt === 'Generated image' ||
                (w >= 512 && !src.includes('organization-images') && !src.includes('googleusercontent') && !src.includes('icon') && !src.includes('logo'))
              );
            })
            .map(img => img.src);

          return {
            isGenerating: stopBtn,
            foundImage: imgs.length > 0 ? imgs[imgs.length - 1] : null
          };
        })()
      `);

      if (status && !status.isGenerating && status.foundImage) {
        finalImageUrl = status.foundImage;
        break;
      }

      await new Promise(r => setTimeout(r, 2000));
    }

    if (!finalImageUrl) {
      throw new Error('Image generation timed out or no image output detected');
    }

    console.log('[AIPass-Image] Image generated successfully! Downloading...');

    // 5. Download image to disk
    let imageBuffer;
    if (finalImageUrl.startsWith('data:image')) {
      const base64Data = finalImageUrl.split(',')[1];
      imageBuffer = Buffer.from(base64Data, 'base64');
    } else {
      const imgRes = await fetch(finalImageUrl);
      const arrayBuf = await imgRes.arrayBuffer();
      imageBuffer = Buffer.from(arrayBuf);
    }

    const outputPath = options.output || 'output_image.png';
    const dir = path.dirname(outputPath);
    if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
    fs.writeFileSync(outputPath, imageBuffer);

    console.log(`[AIPass-Image] Done! Image saved to: ${outputPath} (${imageBuffer.length} bytes)`);
    return {
      success: true,
      path: outputPath,
      url: finalImageUrl,
      size: imageBuffer.length
    };

  } finally {
    ws.close();
  }
}

// CLI runner
if (process.argv[1].endsWith('aipass-image.js')) {
  const args = process.argv.slice(2);
  let prompt = '';
  let model = 'Nano Banana 2';
  let output = '';

  for (let i = 0; i < args.length; i++) {
    if (args[i] === '-m' || args[i] === '--model') {
      model = args[++i];
    } else if (args[i] === '-o' || args[i] === '--output') {
      output = args[++i];
    } else if (!prompt) {
      prompt = args[i];
    }
  }

  if (!prompt) {
    console.log('Usage: node scripts/aipass-image.js "<image_prompt>" [-m "<model_name>"] [-o "<output_file.png>"]');
    console.log('\nSupported Models:');
    console.log('  - "Nano Banana 2" (Default, Google Imagen 3 - high fidelity & text)');
    console.log('  - "Nano Banana Pro"');
    console.log('  - "Seedream 5.0 Lite"');
    console.log('  - "Seedream 4.0"');
    console.log('  - "GPT-Image-2.5 Flare" (DALL-E 3 / OpenAI)');
    console.log('  - "GPT-Image-2.5 Sunburst"');
    process.exit(1);
  }

  if (!output) {
    const timestamp = Date.now();
    output = `docs/images/generated_${timestamp}.png`;
  }

  generateAiPassImage(prompt, { model, output })
    .then(res => {
      console.log(`\nImage ready at: ${res.path}`);
    })
    .catch(err => {
      console.error('\nError generating image:', err.message);
      process.exit(1);
    });
}
