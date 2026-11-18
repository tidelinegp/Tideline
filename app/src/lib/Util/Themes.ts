// Colour themes for the launcher. Each theme is a dark palette plus the accent that goes with it;
// light mode shares one set of neutrals and only takes the theme's accent.

export interface DarkPalette {
	bg: string;
	side: string;
	s1: string;
	s2: string;
	hov: string;
	inp: string;
	inpb: string;
	line: string;
	txt: string;
	mut: string;
	accent: string;
	g1: string;
	g2: string;
	glow1: string;
	glow2: string;
	/** accent used in light mode (darker so it reads on white) */
	accentLight: string;
}

export interface Theme {
	id: string;
	name: string;
	dark: DarkPalette;
}

export const themes: Theme[] = [
	{
		id: 'ember',
		name: 'Ember',
		dark: {
			bg: '#0b0e14', side: '#0e121a', s1: '#171d28', s2: '#131822', hov: '#1a2132', inp: '#1b2232',
			inpb: '#2c3548', line: '#232b3a', txt: '#e8ebf2', mut: '#8a94a8',
			accent: '#f0c14b', g1: '#f6cd62', g2: '#dba935', accentLight: '#b4800c',
			glow1: 'rgba(240,193,75,0.075)', glow2: 'rgba(108,178,255,0.05)'
		}
	},
	{
		id: 'midnight',
		name: 'Midnight',
		dark: {
			bg: '#0a1020', side: '#0f172a', s1: '#16203a', s2: '#121b31', hov: '#1c2947', inp: '#16213a',
			inpb: '#2b3a5e', line: '#253456', txt: '#e6edf9', mut: '#8b9bbd',
			accent: '#6cb2ff', g1: '#8cc4ff', g2: '#3f8fe6', accentLight: '#1f6fe5',
			glow1: 'rgba(90,169,255,0.09)', glow2: 'rgba(240,193,75,0.04)'
		}
	},
	{
		id: 'abyss',
		name: 'Abyss',
		dark: {
			bg: '#07110f', side: '#0a1713', s1: '#10211d', s2: '#0d1c18', hov: '#16302a', inp: '#10211d',
			inpb: '#24473f', line: '#20403a', txt: '#e3f3ee', mut: '#7fa399',
			accent: '#2dd4bf', g1: '#5eead4', g2: '#14b8a6', accentLight: '#0f9488',
			glow1: 'rgba(45,212,191,0.08)', glow2: 'rgba(251,191,36,0.04)'
		}
	},
	{
		id: 'wilderness',
		name: 'Wilderness',
		dark: {
			bg: '#120808', side: '#190d0d', s1: '#221212', s2: '#1c0f0f', hov: '#2e1919', inp: '#241313',
			inpb: '#4a2626', line: '#3d2020', txt: '#f5e6e4', mut: '#a98a86',
			accent: '#f05252', g1: '#ff8a80', g2: '#dc3a3a', accentLight: '#c92a2a',
			glow1: 'rgba(240,82,82,0.08)', glow2: 'rgba(240,179,107,0.04)'
		}
	},
	{
		id: 'stone',
		name: 'Stone',
		dark: {
			bg: '#1d1913', side: '#2a241b', s1: '#332b21', s2: '#2b251d', hov: '#41372b', inp: '#352d23',
			inpb: '#56493a', line: '#4a3f30', txt: '#f0e7d3', mut: '#a89a84',
			accent: '#ffd43a', g1: '#ffe57f', g2: '#e0ae1e', accentLight: '#a56d00',
			glow1: 'rgba(255,212,58,0.07)', glow2: 'rgba(190,130,70,0.07)'
		}
	},
	{
		id: 'violet',
		name: 'Violet',
		dark: {
			bg: '#0e0b16', side: '#120f1d', s1: '#1b1629', s2: '#161222', hov: '#241d37', inp: '#1d1830',
			inpb: '#352c52', line: '#2b2342', txt: '#ece8f6', mut: '#9a90b5',
			accent: '#a78bfa', g1: '#c4b5fd', g2: '#8b5cf6', accentLight: '#6d3ee0',
			glow1: 'rgba(167,139,250,0.09)', glow2: 'rgba(240,193,75,0.04)'
		}
	}
];

export const defaultThemeId = 'ember';

/** Quick accent colours shown next to the custom colour picker. */
export const accentSwatches = ['#f0c14b', '#6cb2ff', '#2dd4bf', '#f05252', '#a78bfa', '#4ade80', '#fb923c', '#f472b6'];

const lightNeutrals = {
	bg: '#eef1f7', side: '#f6f8fc', s1: '#ffffff', s2: '#f4f6fb', hov: '#eaeff9', inp: '#eef2fa',
	inpb: '#cfd7e8', line: '#d8deea', txt: '#151b29', mut: '#5d6a84'
};

function hexToRgb(hex: string): [number, number, number] | null {
	const m = /^#?([0-9a-f]{6})$/i.exec(hex.trim());
	if (!m) return null;
	const n = parseInt(m[1], 16);
	return [(n >> 16) & 255, (n >> 8) & 255, n & 255];
}

function rgbToHex(r: number, g: number, b: number): string {
	const c = (v: number) => Math.max(0, Math.min(255, Math.round(v))).toString(16).padStart(2, '0');
	return `#${c(r)}${c(g)}${c(b)}`;
}

/** Mix a colour toward white (amount > 0) or black (amount < 0). */
function shade(hex: string, amount: number): string {
	const rgb = hexToRgb(hex);
	if (!rgb) return hex;
	const target = amount >= 0 ? 255 : 0;
	const a = Math.abs(amount);
	return rgbToHex(...(rgb.map((v) => v + (target - v) * a) as [number, number, number]));
}

/** Dark or light text, whichever reads better on top of the given colour. */
export function readableOn(hex: string): string {
	const rgb = hexToRgb(hex);
	if (!rgb) return '#1c1503';
	const lum = (0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2]) / 255;
	return lum > 0.55 ? '#1c1503' : '#ffffff';
}

export function isValidHex(value: string | null | undefined): value is string {
	return typeof value === 'string' && hexToRgb(value) !== null;
}

/**
 * Builds the inline CSS variable declarations for the chosen theme. `customAccent` replaces the
 * theme's accent colour and the gradient built from it.
 */
export function themeStyle(themeId: string | undefined, dark: boolean, customAccent?: string | null): string {
	const theme = themes.find((t) => t.id === themeId) ?? themes[0];
	const p = theme.dark;
	const custom = isValidHex(customAccent) ? customAccent : null;

	let accent = dark ? p.accent : p.accentLight;
	let g1 = dark ? p.g1 : p.g1;
	let g2 = dark ? p.g2 : p.g2;
	if (custom) {
		accent = dark ? custom : shade(custom, -0.25);
		g1 = shade(custom, 0.22);
		g2 = shade(custom, -0.18);
	}

	const base = dark
		? { bg: p.bg, side: p.side, s1: p.s1, s2: p.s2, hov: p.hov, inp: p.inp, inpb: p.inpb, line: p.line, txt: p.txt, mut: p.mut }
		: lightNeutrals;
	const glow1 = dark ? p.glow1 : `color-mix(in srgb, ${accent} 16%, transparent)`;
	const glow2 = dark ? p.glow2 : 'rgba(31,111,229,0.06)';
	const vars: { [name: string]: string } = {
		...Object.fromEntries(Object.entries(base).map(([k, v]) => [`--${k}`, v])),
		'--gold': accent,
		'--g1': g1,
		'--g2': g2,
		'--on-gold': readableOn(g1),
		'--glow1': glow1,
		'--glow2': glow2
	};
	return Object.entries(vars)
		.map(([k, v]) => `${k}:${v}`)
		.join(';');
}
