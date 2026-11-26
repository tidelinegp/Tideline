// Colour themes for the launcher, built the same way as the flipping dashboard: every theme is just
// four colours (background, main colour, second colour, text) and all other shades are derived from
// them. Pick one of the presets, or edit the four colours to make your own.

export interface ThemeColors {
	bg: string;
	accent: string;
	accent2: string;
	txt: string;
}

export interface Theme extends ThemeColors {
	id: string;
	name: string;
}

export const themes: Theme[] = [
	// the app's own look: a mid-dark sea blue with sand highlights, matching the icon
	{ id: 'tideline', name: 'Tideline', bg: '#26405a', accent: '#e8d4a8', accent2: '#5fa2d0', txt: '#eaf0f6' },
	{ id: 'sandbar', name: 'Sandbar', bg: '#f3e9d2', accent: '#1d7fb3', accent2: '#b98f55', txt: '#22303a' },
	{ id: 'gielinor', name: 'Gielinor', bg: '#0b0e14', accent: '#f0c14b', accent2: '#6cb2ff', txt: '#e8ebf2' },
	{ id: 'stone', name: 'Stone', bg: '#1d1913', accent: '#ffd43a', accent2: '#8cc0e0', txt: '#f0e7d3' },
	{ id: 'midnight', name: 'Midnight', bg: '#0a1020', accent: '#6cb2ff', accent2: '#f0c14b', txt: '#e6edf9' },
	{ id: 'abyss', name: 'Abyss', bg: '#07110f', accent: '#2dd4bf', accent2: '#fbbf24', txt: '#e3f3ee' },
	{ id: 'wilderness', name: 'Wilderness', bg: '#120808', accent: '#f05252', accent2: '#f0b36b', txt: '#f5e6e4' },
	{ id: 'slayer', name: 'Slayer', bg: '#120d1d', accent: '#b57bff', accent2: '#f0c14b', txt: '#ece6f7' },
	{ id: 'ancient', name: 'Ancient', bg: '#0f0b14', accent: '#d9468b', accent2: '#e8c26a', txt: '#f1e8ef' },
	{ id: 'dragonfire', name: 'Dragonfire', bg: '#130c06', accent: '#ff8a1f', accent2: '#ffd166', txt: '#f6e9dc' },
	{ id: 'saradomin', name: 'Saradomin', bg: '#0a1226', accent: '#4f8cff', accent2: '#e9edf7', txt: '#eaf0fb' },
	{ id: 'guthix', name: 'Guthix', bg: '#0b130b', accent: '#7bd34a', accent2: '#d9c36a', txt: '#e6f0df' },
	{ id: 'rose', name: 'Rose', bg: '#170c12', accent: '#ff7aa8', accent2: '#8be9fd', txt: '#f6e4ec' },
	{ id: 'ocean', name: 'Ocean', bg: '#06141c', accent: '#38bdf8', accent2: '#34d399', txt: '#e1f1f8' },
	{ id: 'toxic', name: 'Toxic', bg: '#0b0f06', accent: '#c6f432', accent2: '#ff6ec7', txt: '#eef5da' },
	{ id: 'slate', name: 'Slate', bg: '#15181d', accent: '#94a3b8', accent2: '#f0c14b', txt: '#e5e9f0' },
	{ id: 'nightfall', name: 'Nightfall', bg: '#000000', accent: '#e5e7eb', accent2: '#6cb2ff', txt: '#f3f4f6' },
	{ id: 'parchment', name: 'Parchment', bg: '#efe5cf', accent: '#9a5b12', accent2: '#2f6f8f', txt: '#2a2117' },
	{ id: 'daylight', name: 'Daylight', bg: '#f2f4f8', accent: '#2563eb', accent2: '#c2410c', txt: '#1b2330' },
	{ id: 'mint', name: 'Mint', bg: '#eef6f1', accent: '#0f766e', accent2: '#b45309', txt: '#17261f' }
];

/** "Match Windows": Tideline when Windows is in dark mode, Sandbar when it is in light mode. */
export const autoThemeId = 'auto';
export const defaultThemeId = autoThemeId;
export const customThemeId = 'custom';

/** Whether Windows (the browser) currently prefers dark apps. Assumes dark if it cannot tell. */
export function osPrefersDark(): boolean {
	if (typeof window === 'undefined' || !window.matchMedia) return true;
	return window.matchMedia('(prefers-color-scheme: light)').matches === false;
}

/** The concrete theme id for a saved one: "auto" (or none saved) becomes Tideline or Sandbar. */
export function resolveThemeId(id: string | undefined, dark: boolean = osPrefersDark()): string {
	if (id === undefined || id === autoThemeId) return dark ? 'tideline' : 'sandbar';
	return id;
}
export const defaultCustom: ThemeColors = { bg: '#10141c', accent: '#f0c14b', accent2: '#6cb2ff', txt: '#e8ebf2' };

/** The four colours behind a theme id, or behind the custom theme. Unknown ids fall back to the default. */
export function themeColors(id: string | undefined, custom: ThemeColors | undefined): ThemeColors {
	if (id === customThemeId) return normalise(custom ?? defaultCustom);
	const resolved = resolveThemeId(id);
	return themes.find((t) => t.id === resolved) ?? themes[0];
}

export function isValidHex(value: unknown): value is string {
	return typeof value === 'string' && /^#[0-9a-f]{6}$/i.test(value.trim());
}

function normalise(c: ThemeColors): ThemeColors {
	return {
		bg: isValidHex(c.bg) ? c.bg : defaultCustom.bg,
		accent: isValidHex(c.accent) ? c.accent : defaultCustom.accent,
		accent2: isValidHex(c.accent2) ? c.accent2 : defaultCustom.accent2,
		txt: isValidHex(c.txt) ? c.txt : defaultCustom.txt
	};
}

const hx = (h: string): [number, number, number] => [1, 3, 5].map((i) => parseInt(h.slice(i, i + 2), 16)) as [number, number, number];

/** Mix colour a toward colour b by t (0 to 1). */
function mix(a: string, b: string, t: number): string {
	const A = hx(a);
	const B = hx(b);
	return '#' + A.map((v, i) => Math.round(v + (B[i] - v) * t).toString(16).padStart(2, '0')).join('');
}

function luminance(h: string): number {
	const [r, g, b] = hx(h).map((v) => v / 255);
	return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

const rgba = (h: string, a: number) => `rgba(${hx(h).join(',')},${a})`;

export function themeIsDark(c: ThemeColors): boolean {
	return luminance(c.bg) < 0.5;
}

/** Inline CSS variable declarations for a set of four colours. */
export function themeStyle(c: ThemeColors): string {
	const dark = themeIsDark(c);
	const up = (t: number) => mix(c.bg, c.txt, t);
	const A = c.accent;
	const vars: { [name: string]: string } = {
		'--bg': c.bg,
		'--side': dark ? mix(c.bg, '#000000', 0.25) : up(0.045),
		'--s1': up(0.07),
		'--s2': up(0.045),
		'--hov': up(0.11),
		'--inp': up(0.07),
		'--inpb': up(0.19),
		'--line': up(0.14),
		'--txt': c.txt,
		'--mut': mix(c.txt, c.bg, 0.42),
		'--gold': A,
		'--g1': mix(A, '#ffffff', 0.3),
		'--g2': mix(A, '#000000', 0.18),
		'--accent2': c.accent2,
		'--on-gold': luminance(A) > 0.42 ? '#17120a' : '#ffffff',
		'--pos': dark ? '#4ade80' : '#15803d',
		'--glow1': rgba(A, dark ? 0.08 : 0.12),
		'--glow2': rgba(c.accent2, 0.05)
	};
	return Object.entries(vars)
		.map(([k, v]) => `${k}:${v}`)
		.join(';');
}
