import type { Session } from '$lib/Util/Interfaces';
import { defaultConfig, type Config } from '$lib/Util/Config';
import { writable } from 'svelte/store';

export const GlobalState = {
	isNew: false,
	initialized: writable(false),
	configHasPendingChanges: false,
	pluginConfigHasPendingChanges: false,
	config: writable<Config>(defaultConfig),
	sessions: writable<{ [sub: string]: Session }>({})
};
