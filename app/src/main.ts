import { Platform, bolt } from '$lib/State/Bolt';
import { GlobalState } from '$lib/State/GlobalState';
import { initConfig } from '$lib/Util/Config';
import type { BoltMessage, Session, PluginMeta } from '$lib/Util/Interfaces';
import { logger } from '$lib/Util/Logger';
import { clientList } from '$lib/Util/Store';
import BoltApp from '@/BoltApp.svelte';
import { mount } from 'svelte';
import { get } from 'svelte/store';

let app: ReturnType<typeof mount> | null = null;
const appConfig = {
	target: document.getElementById('app') as HTMLElement
};

const w = window as {
	sessions?: { [sub: string]: Session };
	plugins?: { [id: string]: PluginMeta };
	autostart?: { [id: string]: string[] };
};
GlobalState.isNew = !w.sessions;

// this function can trigger the native code to discard sessions, which in turn would cause it to
// send a new session list to this function. so it uses a boolean lock and a "pending" system to
// avoid data races.
let handlingSessionList: boolean = false;
let pendingSessionList: { [sub: string]: Session } | null = null;
const handleSessionList = async (list: { [sub: string]: Session }) => {
	if (handlingSessionList) {
		pendingSessionList = list;
		return;
	}
	handlingSessionList = true;
	let config = get(GlobalState.config);
	const discardExpired = config.discard_expired_sessions;
	const promises = Object.entries(list).map(([sub, session]) => {
		let params = new URLSearchParams({
			session_id: session.sessionId,
			discard: discardExpired ? '1' : '0'
		});
		const url = '/characters?'.concat(params.toString());
		const promise = fetch(url).then((x) => {
			if (x.status !== 200) {
				// error fetching sessions
				if (session.isNew) {
					handleAuthError(
						`failed to fetch characters for ${session.displayName}#${session.suffix}`
					);
					session.isNew = false;
				} else {
					logger.error(
						`session for ${session.displayName}#${session.suffix} is expired or invalid`
					);
				}
				return;
			}
			return x.json().then((characters: { accountId: string; displayName: string }[]) => {
				session.characters = {};
				for (const character of characters) {
					session.characters[character.accountId] = character;
				}
				if (session.isNew) {
					config.selected.user_id = sub;
					GlobalState.configHasPendingChanges = true;
					logger.info(`logged in as ${session.displayName}#${session.suffix}`);
					session.isNew = false;
				}
			});
		});
		return promise;
	});
	return Promise.all(promises).then(() => {
		GlobalState.sessions.set(list);
		handlingSessionList = false;
		if (pendingSessionList) {
			const list = pendingSessionList;
			pendingSessionList = null;
			handleSessionList(list);
		}
	});
};

const params = new URLSearchParams(window.location.search);
initBolt(params);
initConfig(params);
addMessageListeners();
handleSessionList(w.sessions ?? {}).finally(() => {
	w.sessions = undefined;
	GlobalState.initialized.set(true);
});
app = mount(BoltApp, appConfig);

export default app;

const handleAuthError = (err: string) => logger.error(`auth error: ${err}`);

function initBolt(params: URLSearchParams) {
	bolt.platform = params.get('platform') as Platform | null;
	bolt.isFlathub = params.get('flathub') === '1';
	bolt.hasLibArchive = params.get('has_libarchive') === '1';
	bolt.boltVersion = params.get('bolt_version');
	bolt.rs3debInstalledHash = params.get('rs3_deb_installed_hash');
	bolt.rs3exeInstalledHash = params.get('rs3_exe_installed_hash');
	bolt.rs3appInstalledHash = params.get('rs3_app_installed_hash');
	bolt.osrsexeInstalledHash = params.get('osrs_exe_installed_hash');
	bolt.osrsappInstalledHash = params.get('osrs_app_installed_hash');
	bolt.runeLiteInstalledId = params.get('runelite_installed_id');
	bolt.hdosInstalledVersion = params.get('hdos_installed_version');

	bolt.hasBoltPlugins = typeof w.plugins === 'object';
	bolt.pluginConfig = w.plugins ?? {};
	bolt.autostart = w.autostart ?? {};
	w.plugins = undefined;
	w.autostart = undefined;
}

function addMessageListeners(): void {
	window.addEventListener('message', async (event: MessageEvent<BoltMessage>) => {
		switch (event.data.type) {
			case 'authError': {
				handleAuthError(event.data.error);
				break;
			}
			case 'gameClientList':
				clientList.set(event.data.clients);
				break;
			case 'sessionList':
				GlobalState.initialized.set(false);
				handleSessionList(event.data.sessions).finally(() => GlobalState.initialized.set(true));
				break;
			default: {
				const type = (event.data as { type: string | undefined })?.type ?? 'no type provided';
				logger.info(`Unknown message type: ${type}`);
				break;
			}
		}
	});
}
