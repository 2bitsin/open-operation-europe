/* Operation Europe, web build: file system set-up before main() runs.
 *
 * Platform glue only (docs/port.md, "Web build"). The original data files
 * are compiled into the wasm (sources/opeu/opeu/assets.embed/), so there is
 * no preloaded .data file; the only files on the virtual file system are
 * saved games, in /saves (the port's save directory on Emscripten):
 *
 *   browser  /saves is an IDBFS mount with autoPersist: every file the game
 *            writes (SAVEDATA) is copied to IndexedDB when it is closed, and
 *            loaded back from IndexedDB on the next visit.
 *   node     /saves is MEMFS; the host file system is mounted at /host
 *            (NODEFS), so a headless run can read an input script and write
 *            its PNG: node opeu.js --headless --input /host/... --png /host/...
 *
 * In the browser main() also waits for the "click to start" of the shell
 * page (Module.opeuStart), which gives the canvas keyboard focus and lets
 * the browser start audio.
 */
Module['preRun'] = Module['preRun'] || [];
if (typeof Module['preRun'] === 'function') Module['preRun'] = [Module['preRun']];

Module['preRun'].push(function () {
  FS.mkdir('/saves');
  if (ENVIRONMENT_IS_NODE) {
    FS.mkdir('/host');
    FS.mount(NODEFS, { root: '/' }, '/host');
    return;
  }
  FS.mount(IDBFS, { autoPersist: true }, '/saves');
  addRunDependency('opeu-saves');
  FS.syncfs(true, function (err) {
    if (err) console.warn('opeu: cannot load saved files from IndexedDB:', err);
    removeRunDependency('opeu-saves');
  });
  if (!Module['opeuAutoStart']) {
    addRunDependency('opeu-start');
    Module['opeuStart'] = function () {
      if (Module['opeuStarted']) return;
      Module['opeuStarted'] = true;
      removeRunDependency('opeu-start');
    };
  }
});

/* Save-game export/import for the shell page (shell.html). The game writes
 * its saves to SAVEDATA in /saves (created as "Savedata": the port
 * resolves DOS names case-insensitively, so either spelling is the same
 * file). Export hands the bytes to the page; import replaces the file and
 * flushes IDBFS so the copy in IndexedDB is current. The file is the
 * original format (20 slots, 202,420 bytes), interchangeable with a DOS
 * installation of the game. */
Module['opeuSaveFileName'] = function () {
  var names = FS.readdir('/saves').filter(function (n) { return n.toLowerCase() === 'savedata'; });
  return names.length ? '/saves/' + names[0] : null;
};
Module['opeuExportSaves'] = function () {
  var name = Module['opeuSaveFileName']();
  return name ? FS.readFile(name) : null;
};
Module['opeuImportSaves'] = function (bytes, done) {
  var name = Module['opeuSaveFileName']() || '/saves/Savedata';
  FS.writeFile(name, bytes);
  FS.syncfs(false, function (err) { if (done) done(err); });
};
