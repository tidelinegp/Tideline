import { GlobalState } from '$lib/State/GlobalState';
import { defaultCustom, defaultThemeId } from '$lib/Util/Themes';
import { get } from 'svelte/store';

// Keeps the launcher's colour theme in step with a dashboard running on this PC, if the user has
// turned that on in Settings. The dashboard stores {theme, custom, updated} at <url>/api/theme.
// The newest change wins. Nothing is sent unless sync is enabled, and only the theme name and four
// colours are ever sent.

const POLL_MS = 15_000;
const DEBOUNCE_MS = 500;

interface RemoteTheme {
	theme?: string;
	custom?: { bg: string; accent: string; accent2: string; txt: string };
	updated?: number;
}

const isHex = (v: unknown): boolean => typeof v === 'string' && /^#[0-9a-f]{6}$/i.test(v);

function endpoint(): string | null {
	const { sync_dashboard_theme, dashboard_url } = get(GlobalState.config);
	if (!sync_dashboard_theme || !dashboard_url) return null;
	const base = dashboard_url.trim().replace(/\/+$/, '');
	if (!/^https?:\/\/\S+$/i.test(base)) return null;
	return `${base}/api/theme`;
}

const signature = () => {
	const c = get(GlobalState.config);
	return JSON.stringify([c.theme ?? defaultThemeId, c.custom_theme ?? null]);
};

export function startThemeSync(): () => void {
	const { config } = GlobalState;
	let lastSig = signature();
	let pushTimer: ReturnType<typeof setTimeout> | undefined;

	async function push() {
		const url = endpoint();
		if (!url) return;
		const c = get(config);
		const updated = Date.now();
		config.update((x) => ({ ...x, theme_updated: updated }));
		lastSig = signature();
		try {
			await fetch(url, {
				method: 'POST',
				headers: { 'Content-Type': 'application/json' },
				body: JSON.stringify({
					theme: c.theme ?? defaultThemeId,
					custom: c.custom_theme ?? defaultCustom,
					updated
				})
			});
		} catch {
			// the dashboard is not running; the next change tries again
		}
	}

	async function pull() {
		const url = endpoint();
		if (!url) return;
		let remote: RemoteTheme;
		try {
			const response = await fetch(url, { cache: 'no-store' });
			if (!response.ok) return;
			remote = await response.json();
		} catch {
			return;
		}
		if (!remote || typeof remote.theme !== 'string') {
			// nothing shared yet, so share this one
			schedulePush();
			return;
		}
		const mine = get(config).theme_updated ?? 0;
		if (!(typeof remote.updated === 'number' && remote.updated > mine)) return;
		const custom =
			remote.custom && ['bg', 'accent', 'accent2', 'txt'].every((k) => isHex((remote.custom as never)[k]))
				? remote.custom
				: undefined;
		config.update((x) => ({
			...x,
			theme: remote.theme,
			custom_theme: custom ?? x.custom_theme,
			theme_updated: remote.updated
		}));
		lastSig = signature();
	}

	function schedulePush() {
		clearTimeout(pushTimer);
		pushTimer = setTimeout(push, DEBOUNCE_MS);
	}

	// a local theme change is pushed to the dashboard
	const unsubscribe = config.subscribe(() => {
		const sig = signature();
		if (sig === lastSig) return;
		lastSig = sig;
		schedulePush();
	});

	pull();
	const timer = setInterval(pull, POLL_MS);
	let lastCheck = 0;
	const onActivity = () => {
		const now = Date.now();
		if (now - lastCheck < 4000) return;
		lastCheck = now;
		pull();
	};
	window.addEventListener('focus', onActivity);
	document.addEventListener('visibilitychange', onActivity);
	window.addEventListener('mousemove', onActivity);

	return () => {
		unsubscribe();
		clearInterval(timer);
		clearTimeout(pushTimer);
		window.removeEventListener('focus', onActivity);
		document.removeEventListener('visibilitychange', onActivity);
		window.removeEventListener('mousemove', onActivity);
	};
}
