async function api(path, opts = {}) {
  const timeoutMs = opts.timeoutMs || 4000;
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), timeoutMs);

  try {
    const res = await fetch(path, {
      cache: 'no-store',
      credentials: 'same-origin',
      headers: opts.body ? { 'Content-Type': 'application/json' } : undefined,
      signal: opts.signal || controller.signal,
      ...opts,
      body: opts.body ? JSON.stringify(opts.body) : undefined,
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(data.error || 'request_failed');
    return data;
  } finally {
    clearTimeout(timer);
  }
}

