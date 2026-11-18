import { GlobalState } from '$lib/State/GlobalState';
import { Client, Game } from '$lib/Util/Interfaces';
import { logger } from '$lib/Util/Logger';
import { onWritableChange } from '$lib/Util/OnWritableChange';
import { bolt } from '$lib/State/Bolt';
import { get } from 'svelte/store';

let saveInProgress: boolean = false;
let savePluginInProgress: boolean = false;

type UserDetails = {
	account_id: string;
};

export interface Config {
	use_dark_theme: boolean;
	rs_plugin_loader: boolean;
	close_after_launch: boolean;
	check_announcements: boolean;
	fetch_rss_feeds: boolean;
	flatpak_rich_presence: boolean;
	runelite_use_custom_jar: boolean;
	use_custom_rs_config_uri: boolean;
	discard_expired_sessions: boolean;
	rs_config_uri?: string;
	rs_launch_command: string | null;
	osrs_launch_command: string | null;
	runelite_custom_jar: string | null;
	runelite_launch_command: string | null;
	hdos_launch_command: string | null;
	/** id of the chosen colour theme, or "custom"; see Themes.ts */
	theme?: string;
	/** the four colours of the user's own theme, used when theme is "custom" */
	custom_theme?: { bg: string; accent: string; accent2: string; txt: string };
	/** when each character was last launched, in ms since epoch */
	last_played?: { [character_id: string]: number };
	/** picture for each Jagex account, as an image data URL or web link, keyed by account id (sub) */
	avatars?: { [sub: string]: string };
	selected: {
		game: Game;
		client: Client;
		user_id: string | null;
	};
	// Each user has data associated with it that is recalled on sign-in or when switching between them
	userDetails: {
		[user_id: string]: UserDetails | undefined;
	};
}

export const defaultConfig: Config = {
	use_dark_theme: true,
	rs_plugin_loader: false,
	close_after_launch: false,
	check_announcements: true,
	fetch_rss_feeds: true,
	flatpak_rich_presence: false,
	runelite_use_custom_jar: false,
	use_custom_rs_config_uri: false,
	discard_expired_sessions: true,
	rs_launch_command: null,
	osrs_launch_command: null,
	runelite_custom_jar: null,
	runelite_launch_command: null,
	hdos_launch_command: null,
	theme: 'gielinor',
	last_played: {},
	selected: {
		game: Game.osrs,
		client: Client.runelite,
		user_id: null
	},
	userDetails: {}
};

export function initConfig(params: URLSearchParams) {
	const configParam = params.get('config');

	function isConfigValid(conf: Config): conf is Config {
		return (
			typeof conf === 'object' &&
			typeof conf.use_dark_theme === 'boolean' &&
			typeof conf.rs_plugin_loader === 'boolean' &&
			typeof conf.close_after_launch === 'boolean' &&
			typeof conf.check_announcements === 'boolean' &&
			typeof conf.fetch_rss_feeds === 'boolean' &&
			typeof conf.flatpak_rich_presence === 'boolean' &&
			typeof conf.runelite_use_custom_jar === 'boolean' &&
			typeof conf.selected === 'object' &&
			Object.values(Game).includes(conf.selected.game) &&
			Object.values(Client).includes(conf.selected.client) &&
			(typeof conf.selected.user_id === 'string' || conf.selected.user_id === null) &&
			typeof conf.userDetails === 'object'
		);
	}

	const { config } = GlobalState;

	if (configParam) {
		try {
			const parsedConfig = JSON.parse(configParam);
			if (typeof parsedConfig.check_announcements === 'undefined') {
				parsedConfig.check_announcements = true;
				GlobalState.configHasPendingChanges = true;
			}
			if (typeof parsedConfig.fetch_rss_feeds === 'undefined') {
				parsedConfig.fetch_rss_feeds = true;
				GlobalState.configHasPendingChanges = true;
			}
			if (typeof parsedConfig.close_after_launch === 'undefined') {
				parsedConfig.close_after_launch = false;
				GlobalState.configHasPendingChanges = true;
			}
			if (typeof parsedConfig.discard_expired_sessions === 'undefined') {
				parsedConfig.discard_expired_sessions = true;
				GlobalState.configHasPendingChanges = true;
			}
			if (isConfigValid(parsedConfig)) {
				config.set(parsedConfig);
			} else {
				logger.error(
					'The config saved on disk is not the correct format. It has been restored to default.'
				);
				GlobalState.configHasPendingChanges = true;
			}
		} catch (e) {
			logger.error('Unable to parse config, restoring to default');
			GlobalState.configHasPendingChanges = true;
		}
	} else {
		GlobalState.configHasPendingChanges = true;
	}

	onWritableChange(GlobalState.config, () => {
		GlobalState.configHasPendingChanges = true;
	});
}

// sends an asynchronous request to save the current user config to disk, if it has changed
export function saveConfig(checkForPendingChanges = true) {
	if (saveInProgress) return;
	if (checkForPendingChanges && !GlobalState.configHasPendingChanges) return;
	saveInProgress = true;
	const config = get(GlobalState.config);
	if (config.rs_launch_command === '') config.rs_launch_command = null;
	if (config.osrs_launch_command === '') config.osrs_launch_command = null;
	if (config.runelite_launch_command === '') config.runelite_launch_command = null;
	if (config.hdos_launch_command === '') config.hdos_launch_command = null;
	postJSON('/save-config', config, () => (saveInProgress = false));
	return config;
}

// sends an asynchronous POST request containing some JSON and calls the callback with a fetch
// response object
const postJSON = async (
	api: string,
	object: object,
	callback: ((r: Response) => void) | null = null
): Promise<void> => {
	const body: string = JSON.stringify(object);
	const headers = { 'Content-Type': 'application/json' };
	const response = await fetch(api, { method: 'POST', body, headers });
	if (!response.ok)
		response.text().then((text) => logger.error(`${api} error: ${response.status}: ${text}`));
	if (callback) callback(response);
};

// sends an asynchronous request to save the current plugin config to disk, if it has changed
export function savePluginConfig(checkForPendingChanges = true) {
	// doesn't really have anything to do with the rest of this file, since this file is for the
	// regular config and this function is for the plugin config. but I needed somewhere to put it,
	// so I put it here because it has "config" in the name and uses postJSON from here.
	if (savePluginInProgress) return;
	if (checkForPendingChanges && !GlobalState.pluginConfigHasPendingChanges) return;
	savePluginInProgress = true;

	const configToSave = { ...bolt.pluginConfig, autostart: bolt.autostart };

	postJSON('/save-plugin-config', configToSave, () => {
		savePluginInProgress = false;
		GlobalState.pluginConfigHasPendingChanges = false;
	});
}
