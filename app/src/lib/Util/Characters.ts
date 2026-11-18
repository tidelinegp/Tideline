import { GlobalState } from '$lib/State/GlobalState';
import type { Session } from '$lib/Util/Interfaces';
import { logger } from '$lib/Util/Logger';
import { get } from 'svelte/store';

// Reloads the character list of logged-in accounts. Used to recover when the list could not be
// fetched at startup (for example a short network hiccup), without restarting the launcher.

let reloading = false;

async function load(session: Session): Promise<Session['characters'] | null> {
	// discard=0: never log an account out because of a refresh that failed
	const params = new URLSearchParams({ session_id: session.sessionId, discard: '0' });
	try {
		const response = await fetch('/characters?'.concat(params.toString()));
		if (response.status !== 200) return null;
		const list: { accountId: string; displayName: string }[] = await response.json();
		const characters: Session['characters'] = {};
		for (const c of list) characters[c.accountId] = c;
		return characters;
	} catch {
		return null;
	}
}

/** Reloads characters for every account, or only those that currently have none. Returns how many failed. */
export async function reloadCharacters(onlyEmpty: boolean): Promise<number> {
	if (reloading) return 0;
	reloading = true;
	let failed = 0;
	try {
		const sessions = get(GlobalState.sessions);
		const next = { ...sessions };
		await Promise.all(
			Object.entries(sessions).map(async ([sub, session]) => {
				if (onlyEmpty && Object.keys(session.characters ?? {}).length > 0) return;
				const characters = await load(session);
				if (characters) {
					next[sub] = { ...session, characters };
				} else {
					failed += 1;
				}
			})
		);
		GlobalState.sessions.set(next);
		if (failed > 0) logger.warn(`could not load characters for ${failed} account(s), will retry`);
	} finally {
		reloading = false;
	}
	return failed;
}
