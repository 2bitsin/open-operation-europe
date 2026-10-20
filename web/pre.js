/* Operation Europe, web build: file system set-up before main() runs.
 *
 * Platform glue only (docs/port.md, "The browser build"). A tree that holds
 * the game's data files compiles them into the wasm, and the page needs no
 * file. A tree without them (the public mirror) builds a wasm that looks in
 * /data, where the page keeps the player's own 26 files. Two directories:
 *
 *   /saves   saved games (the port's save directory on Emscripten)
 *   /data    the player's data files, when the wasm has none of its own
 *
 *   browser  both are IDBFS mounts with autoPersist: every file written is
 *            copied to IndexedDB when it is closed, and loaded back from
 *            IndexedDB on the next visit.
 *   node     both are MEMFS; the host file system is mounted at /host
 *            (NODEFS), so a headless run can read an input script, its data
 *            and write its PNG:
 *            node opeu.js --headless --data /host/... --input /host/... --png /host/...
 *
 * In the browser main() also waits for the "click to start" of the shell
 * page (Module.opeuStart), which gives the canvas keyboard focus and lets
 * the browser start audio.
 */
Module['preRun'] = Module['preRun'] || [];
if (typeof Module['preRun'] === 'function') Module['preRun'] = [Module['preRun']];

Module['preRun'].push(function () {
  FS.mkdir('/saves');
  FS.mkdir('/data');
  if (ENVIRONMENT_IS_NODE) {
    FS.mkdir('/host');
    FS.mount(NODEFS, { root: '/' }, '/host');
    return;
  }
  FS.mount(IDBFS, { autoPersist: true }, '/saves');
  FS.mount(IDBFS, { autoPersist: true }, '/data');
  addRunDependency('opeu-saves');
  FS.syncfs(true, function (err) {
    if (err) console.warn('opeu: cannot load saved files and data from IndexedDB:', err);
    if (Module['opeuDataStored']) Module['opeuDataStored'](Module['opeuDataCount']());
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

/* The player's data files for the page (shell.html). When the wasm has no
 * data of its own and /data lacks a file, the entry calls Module.opeuNeedData
 * with its report; the page takes the files the player picks or drops,
 * stores them here under their upper-case names and reloads, and the entry
 * checks them again (it alone decides whether they are the release's).
 * Forget removes them. Both flush IDBFS before calling back. */
Module['opeuStoreData'] = function (files, done) {
  files.forEach(function (file) { FS.writeFile('/data/' + file.name.toUpperCase(), file.bytes); });
  FS.syncfs(false, function (err) { if (done) done(err); });
};
Module['opeuForgetData'] = function (done) {
  FS.readdir('/data').filter(function (n) { return n !== '.' && n !== '..'; })
    .forEach(function (n) { FS.unlink('/data/' + n); });
  FS.syncfs(false, function (err) { if (done) done(err); });
};
Module['opeuDataCount'] = function () {
  return FS.readdir('/data').filter(function (n) { return n !== '.' && n !== '..'; }).length;
};
