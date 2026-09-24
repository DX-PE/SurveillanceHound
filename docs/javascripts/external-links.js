// SPDX-License-Identifier: Apache-2.0
(() => {
  function update(root) {
    const links = root.matches?.('a[href]') ? [root] : [];
    links.push(...root.querySelectorAll('a[href]'));
    for (const link of links) {
      const url = new URL(link.href, document.baseURI);
      if (!['http:', 'https:'].includes(url.protocol) || url.origin === location.origin) continue;
      link.target = '_blank';
      link.relList.add('noopener', 'noreferrer');
    }
  }
  function start() {
    update(document);
    new MutationObserver(records => {
      for (const record of records) {
        if (record.type === 'attributes') update(record.target);
        for (const node of record.addedNodes) if (node.nodeType === Node.ELEMENT_NODE) update(node);
      }
    }).observe(document.body, {childList: true, subtree: true, attributes: true, attributeFilter: ['href']});
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start, {once: true});
  else start();
})();
