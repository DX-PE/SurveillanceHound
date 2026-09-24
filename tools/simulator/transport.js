// SPDX-License-Identifier: Apache-2.0
export function createTransport(runtime) {
  if (runtime === 'native') {
    return {
      label: 'LOCAL SIMULATOR RUNNING',
      async request(action) {
        const response = action ? await fetch('./api/action', {
          method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(action)
        }) : await fetch('./api/frame');
        if (!response.ok) throw new Error(`Simulator returned ${response.status}`);
        return {state: JSON.parse(response.headers.get('X-Sniffer-State')), frame: await response.arrayBuffer()};
      },
      close() {}
    };
  }
  if (!globalThis.WebAssembly || !globalThis.Worker || !globalThis.crypto?.subtle) {
    throw new Error('Use a current browser over HTTPS or localhost to run the lab.');
  }
  const worker = new Worker(new URL('./browser-worker.js', import.meta.url), {type: 'module'});
  const requests = new Map();
  const started = performance.now();
  let sequence = 0;
  let failed;
  function stop(error) {
    failed = error;
    for (const {reject, timer} of requests.values()) {clearTimeout(timer); reject(error);}
    requests.clear();
    worker.terminate();
  }
  worker.onerror = event => {event.preventDefault(); stop(new Error(event.message || 'Browser lab could not load. Reload to retry.'));};
  worker.onmessageerror = () => stop(new Error('Browser lab returned an unreadable frame. Reload to retry.'));
  worker.onmessage = ({data}) => {
    const request = requests.get(data.id);
    if (!request) return;
    clearTimeout(request.timer);
    requests.delete(data.id);
    if (data.error) {request.reject(new Error(data.error)); stop(new Error(data.error));}
    else request.resolve(data);
  };
  return {
    label: 'RUNNING IN YOUR BROWSER',
    request(action) {
      if (failed) return Promise.reject(failed);
      return new Promise((resolve, reject) => {
        const id = ++sequence;
        const timer = setTimeout(() => stop(new Error('The lab took too long to respond. Reload to retry.')), 60000);
        requests.set(id, {resolve, reject, timer});
        worker.postMessage({id, action, ms: Math.floor(performance.now() - started)});
      });
    },
    close() {stop(new Error('Lab closed'));}
  };
}
