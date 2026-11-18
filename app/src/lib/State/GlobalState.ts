import type { Session } from '$lib/Util/Interfaces';
import { defaultConfig, type Config } from '$lib/Util/Config';
import { writable } from 'svelte/store';

export const GlobalState = {
	isNew: false,
	initialized: writable(false),
	configHasPendingChanges: false,
	pluginConfigHasPendingChanges: false,
	config: writable<Config>(defaultConfig),
	sessions: writable<{ [sub: string]: Session }>({}),
	// characters launched during this run of the launcher, by character id (ms since epoch)
	launched: writable<{ [character_id: string]: number }>({}),
	// titles of the game windows currently open on this PC, polled from the native app
	clientTitles: writable<string[]>([])
};
