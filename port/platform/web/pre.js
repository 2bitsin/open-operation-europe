/* Operation Europe, web build: file system set-up before main() runs.
 *
 * Platform glue only (port/README.md, "Web build"). The build packs the
 * original game files into opeu.data at /gamedata (read-only originals).
 * The program reads and writes its game directory /data (the port's
 * --data default on Emscripten):
 *
 *   browser  /data is an IDBFS mount with autoPersist: every file the game
 *            writes (saved games, OPEU.SAV) is copied to IndexedDB when it is
 *            closed, and loaded back from IndexedDB on the next visit.
 *   node     /data is MEMFS; the host file system is mounted at /host
 *            (NODEFS), so a headless run can read an input script and write
 *            its PNG: node opeu.js --headless --input /host/... --png /host/...
 *
 * Before main(), every original file that /data does not have yet is copied
 * from /gamedata, so the first visit starts from the original directory
 * and later visits keep what the game wrote.
 *
 * In the browser main() also waits for the "click to start" of the shell
 * page (Module.opeuStart), which gives the canvas keyboard focus and lets
 * the browser start audio.
 */
Module['preRun'] = Module['preRun'] || [];
if (typeof Module['preRun'] === 'function') Module['preRun'] = [Module['preRun']];

Module['preRun'].push(function () {
  FS.mkdir('/data');
  if (ENVIRONMENT_IS_NODE) {
    FS.mkdir('/host');
    FS.mount(NODEFS, { root: '/' }, '/host');
    return;
  }
  FS.mount(IDBFS, { autoPersist: true }, '/data');
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

(function () {
  var chained = Module['onRuntimeInitialized'];
  Module['onRuntimeInitialized'] = function () {
    var copied = 0;
    if (FS.analyzePath('/gamedata').exists) {
      FS.readdir('/gamedata').forEach(function (name) {
        if (name === '.' || name === '..') return;
        var to = '/data/' + name;
        if (FS.analyzePath(to).exists) return;
        FS.writeFile(to, FS.readFile('/gamedata/' + name));
        copied++;
      });
    }
    if (copied) console.log('opeu: copied ' + copied + ' original files into /data');
    if (chained) chained();
  };
})();
