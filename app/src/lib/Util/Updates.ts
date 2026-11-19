// Update check. The launcher reads a small manifest (latest.json) from the release page; if it names a newer
// version, the page offers it. The actual download, checksum and signature checks happen in the native program
// (see ApplyUpdate in window_launcher.cxx), which refuses anything not signed by Tideline's key.
import { writable } from 'svelte/store';
import { brand } from './Brand';

export interface UpdateInfo {
	version: string;
	url: string;
	sha256: string;
	sig: string;
	notes: string;
}

export const availableUpdate = writable<UpdateInfo | null>(null);
// the newest published release, newer than this one or not (used to add the dashboard to a launcher-only install)
export const latestRelease = writable<UpdateInfo | null>(null);
export const updateProgress = writable<{ state: string; message: string }>({ state: 'idle', message: '' });

// "0.10.2" > "0.9.9"; anything that is not plain numbers is treated as not newer
export const isNewer = (candidate: string, current: string): boolean => {
	const parse = (v: string) => v.split('.').map((n) => (/^\d+$/.test(n) ? parseInt(n, 10) : NaN));
	const a = parse(candidate);
	const b = parse(current);
	if (a.some(Number.isNaN) || b.some(Number.isNaN)) return false;
	for (let i = 0; i < Math.max(a.length, b.length); i += 1) {
		const x = a[i] ?? 0;
		const y = b[i] ?? 0;
		if (x !== y) return x > y;
	}
	return false;
};

export const checkForUpdate = async (): Promise<void> => {
	if (!brand.updateManifestUrl) return; // no release page yet
	try {
		const r = await fetch(`${brand.updateManifestUrl}?t=${Date.now()}`, { cache: 'no-store' });
		if (!r.ok) return;
		const m = await r.json();
		const ok = ['version', 'url', 'sha256', 'sig'].every((k) => typeof m?.[k] === 'string' && m[k]);
		if (ok) {
			const info = { version: m.version, url: m.url, sha256: m.sha256, sig: m.sig, notes: typeof m.notes === 'string' ? m.notes : '' };
			latestRelease.set(info);
			availableUpdate.set(isNewer(m.version, brand.version) ? info : null);
		} else {
			availableUpdate.set(null);
		}
	} catch {
		// offline or the page is down: try again at the next check
	}
};

let polling: ReturnType<typeof setInterval> | null = null;

export const installUpdate = async (u: UpdateInfo, addDashboard = false): Promise<void> => {
	updateProgress.set({ state: 'downloading', message: 'Starting...' });
	const q = new URLSearchParams({ version: u.version, url: u.url, sha256: u.sha256, sig: u.sig });
	if (addDashboard) q.set('dashboard', '1');
	try {
		const r = await fetch(`/apply-update?${q.toString()}`);
		if (!r.ok) throw new Error('refused');
	} catch {
		updateProgress.set({ state: 'error', message: 'Could not start the update.' });
		return;
	}
	if (polling) clearInterval(polling);
	polling = setInterval(async () => {
		try {
			const s = await (await fetch('/update-status')).json();
			updateProgress.set(s);
			if (s.state === 'error' && polling) {
				clearInterval(polling);
				polling = null;
			}
		} catch {
			// the program is being replaced; nothing more to show
		}
	}, 700);
};

// check now, then every few hours while the launcher stays open
export const startUpdateChecks = (): (() => void) => {
	checkForUpdate();
	const timer = setInterval(checkForUpdate, 4 * 60 * 60 * 1000);
	return () => clearInterval(timer);
};
