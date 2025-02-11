if ('serviceWorker' in navigator) {
  navigator.serviceWorker.register('sw.js', { scope: '' })
      .then((registration) => {
          const data = {
              type: 'CACHE_URLS',
              payload: [
                  location.href,
                  // cache all files that were pulled in: https://stackoverflow.com/a/55543550/1569320
                  ...performance.getEntriesByType('resource').map((r) => r.name)
              ]
          };
          registration.installing.postMessage(data);
      })
      .catch((err) => console.log('SW registration FAIL:', err));
}