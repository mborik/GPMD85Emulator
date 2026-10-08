// Persist /home/web_user (settings) in browser's IndexedDB via IDBFS.
Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
	var dir = '/home/web_user';
	try { FS.mkdirTree(dir); } catch (e) {}
	FS.mount(IDBFS, {}, dir);

	addRunDependency('idbfs-sync');
	FS.syncfs(true, function (err) {
		if (err) console.error('IDBFS load failed:', err);
		removeRunDependency('idbfs-sync');
	});

	var flush = function () { FS.syncfs(false, function (err) { if (err) console.error('IDBFS save failed:', err); }); };
	setInterval(flush, 5000);
	window.addEventListener('pagehide', flush);
	document.addEventListener('visibilitychange', function () { if (document.hidden) flush(); });
});
