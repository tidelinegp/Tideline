<script lang="ts">
	import { bolt, Platform } from '$lib/State/Bolt';
	import { GlobalState } from '$lib/State/GlobalState';
	import {
		launchHdos,
		launchOfficialClient,
		launchRS3Linux,
		launchRuneLite
	} from '$lib/Util/Functions';
	import { saveConfig } from '$lib/Util/Config';
	import { Client, clientMap, Game } from '$lib/Util/Interfaces';
	import { logger } from '$lib/Util/Logger';

	let { config, initialized, sessions, launched } = GlobalState;
	let selectedUserId = $derived($config.selected.user_id);
	let selectedCharacterId = $derived(
		selectedUserId ? $config.userDetails[selectedUserId]?.account_id : null
	);
	let session = $derived(selectedUserId ? $sessions[selectedUserId] : null);
	let selectedCharacter = $derived(
		session && selectedCharacterId ? session.characters[selectedCharacterId] : null
	);
	let lastPlayed = $derived(
		selectedCharacterId ? ($config.last_played ?? {})[selectedCharacterId] : undefined
	);

	// messages about game downtime, retrieved from game server
	let {
		osrsPsa = $bindable(),
		rs3Psa = $bindable(),
		launchConfirm = $bindable(),
		openPluginMenu = $bindable()
	} = $props();
	const PSA_FETCH_INTERVAL_MS = 60_000;
	let psaFetchedAt: number = 0;

	let gameEnabled: boolean = true;
	$effect(() => {
		if ($config.check_announcements) {
			const now = Date.now();
			if (now - psaFetchedAt < PSA_FETCH_INTERVAL_MS) {
				return;
			}

			for (const game of [Game.osrs, Game.rs3]) {
				const gameName = game == Game.osrs ? 'osrs' : bolt.env.provider;
				const url: string = `${bolt.env.psa_url}${gameName}/${gameName}.json?ts=${now}`;
				// added no-store due to an issue where new messages are not shown until cache is cleared.
				// remote server appears to be using etags incorrectly?
				fetch(url, { method: 'GET', cache: 'no-store' })
					.then((response) => response.json())
					.then((response) => {
						psaFetchedAt = now;
						let psa = response.psaEnabled && response.psaMessage ? response.psaMessage : null;
						gameEnabled = !(response.playDisabled ?? false);
						if (psa) {
							switch (game) {
								case Game.osrs:
									osrsPsa = psa;
									break;
								case Game.rs3:
									rs3Psa = psa;
									break;
							}
						}
					});
			}
		} else {
			osrsPsa = null;
			rs3Psa = null;
			gameEnabled = true;
		}
	});

	// when play is clicked, check the selected_play store for all relevant details
	// calls the appropriate launch functions
	export function launch(game: Game, client: Client): void {
		if (!selectedUserId || !selectedCharacterId || !session) {
			return logger.warn('please log in and select a character to play');
		}
		const now = Date.now();
		$config.last_played = { ...($config.last_played ?? {}), [selectedCharacterId]: now };
		launched.update((l) => ({ ...l, [selectedCharacterId]: now }));
		GlobalState.clientSeen.update((s) => ({ ...s, [selectedCharacterId]: false }));
		saveConfig();

		const { sessionId } = session;
		const character = session.characters[selectedCharacterId];
		if (!character) return logger.error(`character ${selectedCharacterId} not found`);
		const { displayName } = character;
		const isWindows = bolt.platform === Platform.Windows;
		const isLinux = bolt.platform === Platform.Linux;
		switch (game) {
			case Game.osrs:
				switch (client) {
					case Client.official:
						// when launching official osrs client on linux, set windows=true so that it will download the
						// windows exe instead of the mac app, as the exe is what we want to run through wine/proton
						launchOfficialClient(
							isWindows || isLinux,
							true,
							sessionId,
							selectedCharacterId,
							displayName
						);
						break;
					case Client.runelite:
						launchRuneLite(sessionId, selectedCharacterId, displayName, false);
						break;
					case Client.hdos:
						launchHdos(sessionId, selectedCharacterId, displayName);
						break;
				}
				break;
			case Game.rs3:
				if (isLinux) {
					launchRS3Linux(sessionId, selectedCharacterId, displayName);
				} else {
					launchOfficialClient(isWindows, false, sessionId, selectedCharacterId, displayName);
				}
				break;
		}
	}

	// shows "Launching" on the Play button for a moment after a launch, so it is clear something happened
	let launching = $state(false);
	let launchingTimer: ReturnType<typeof setTimeout> | undefined;

	// the Play button, also called by the account sidebar after it selects a character
	export function play(): void {
		if (launching) return;
		launching = true;
		clearTimeout(launchingTimer);
		launchingTimer = setTimeout(() => (launching = false), 3500);
		if (gameEnabled) {
			launch($config.selected.game, $config.selected.client);
		} else {
			launchConfirm(launch, $config.selected.game, $config.selected.client);
		}
	}

	function ago(ms: number | undefined): string {
		if (!ms) return 'Not played yet';
		const s = Math.max(0, Math.round((Date.now() - ms) / 1000));
		if (s < 60) return 'Played just now';
		const m = Math.round(s / 60);
		if (m < 60) return `Played ${m}m ago`;
		const h = Math.round(m / 60);
		if (h < 48) return `Played ${h}h ago`;
		return `Played ${Math.round(h / 24)}d ago`;
	}
</script>

<section class="hero" aria-label="Ready to play">
	<div class="who">
		<div class="who-ring" aria-hidden="true">
			{#if selectedUserId && $config.avatars?.[selectedUserId]}
				<img src={$config.avatars[selectedUserId]} alt="" />
			{:else}
				{(selectedCharacter?.displayName?.trim()[0] ?? '?').toUpperCase()}
			{/if}
		</div>
		<div class="id">
			<div class="eyebrow">{selectedCharacter ? 'Ready to play' : 'Pick a character'}</div>
			<h2>
				{selectedCharacter?.displayName ||
					(Object.keys($sessions).length > 0 ? 'Choose a character' : 'Log in to start')}
			</h2>
			<div class="meta">
				{#if session}
					<span class="pill">{session.displayName}</span>
					<span>{ago(lastPlayed)}</span>
				{:else}
					<span>Add a Jagex account in the sidebar</span>
				{/if}
			</div>
		</div>
	</div>

	<div class="act">
		{#if $config.selected.game == Game.osrs}
			<div class="clients" role="group" aria-label="Game client">
				{#each clientMap[$config.selected.game] as client}
					<button
						type="button"
						aria-pressed={$config.selected.client === client}
						onclick={() => ($config.selected.client = client)}>{client}</button
					>
				{/each}
			</div>
		{:else if $config.selected.game === Game.rs3}
			<button
				class="plug"
				type="button"
				disabled={!bolt.hasBoltPlugins}
				title={bolt.hasBoltPlugins ? null : 'This feature is disabled'}
				onclick={openPluginMenu}
			>
				Plugin menu
			</button>
		{/if}
		<button
			class="play"
			type="button"
			disabled={!$initialized || !selectedCharacter || launching}
			onclick={play}
		>
			{launching ? 'Launching…' : 'Play'}
		</button>
	</div>
</section>

<style>
	.hero {
		position: relative;
		background: linear-gradient(180deg, var(--s1), var(--s2));
		border: 1px solid var(--line);
		border-radius: var(--r);
		padding: 22px 24px;
		display: grid;
		grid-template-columns: minmax(0, 1fr) auto;
		gap: 22px;
		align-items: center;
		overflow: hidden;
	}
	.hero::before {
		content: '';
		position: absolute;
		left: 0;
		top: 0;
		right: 0;
		height: 2px;
		background: linear-gradient(90deg, var(--gold), var(--accent2) 45%, transparent 85%);
	}
	.who {
		display: flex;
		align-items: center;
		gap: 18px;
		min-width: 0;
	}
	.who-ring {
		flex: none;
		width: 64px;
		height: 64px;
		border-radius: 20px;
		display: grid;
		place-items: center;
		font: 800 28px var(--disp);
		color: var(--gold);
		background: color-mix(in srgb, var(--gold) 10%, var(--inp));
		border: 1px solid color-mix(in srgb, var(--gold) 40%, var(--inpb));
		overflow: hidden;
	}
	.who-ring img {
		width: 100%;
		height: 100%;
		object-fit: cover;
		display: block;
	}
	.id {
		min-width: 0;
	}
	.eyebrow {
		letter-spacing: 0.16em;
		font-size: 11px;
		font-weight: 700;
		color: var(--gold);
		text-transform: uppercase;
	}
	h2 {
		margin: 4px 0 6px;
		font: 800 clamp(26px, 4vw, 44px) var(--disp);
		letter-spacing: -0.04em;
		line-height: 1.05;
		overflow-wrap: anywhere;
	}
	.meta {
		display: flex;
		align-items: center;
		gap: 10px;
		color: var(--mut);
		font-size: 13.5px;
		flex-wrap: wrap;
	}
	.pill {
		background: color-mix(in srgb, var(--accent2) 14%, var(--inp));
		border: 1px solid color-mix(in srgb, var(--accent2) 40%, var(--inpb));
		color: var(--txt);
		border-radius: 99px;
		padding: 2px 10px;
		font-weight: 600;
		font-size: 12.5px;
	}
	.act {
		display: grid;
		gap: 10px;
		justify-items: stretch;
		min-width: 190px;
	}
	.clients {
		display: flex;
		background: var(--s2);
		border: 1px solid var(--line);
		border-radius: 11px;
		padding: 3px;
	}
	.clients button {
		flex: 1;
		background: none;
		border: 0;
		color: var(--mut);
		font: 650 12.5px var(--body);
		padding: 6px 10px;
		border-radius: 8px;
		cursor: pointer;
		white-space: nowrap;
	}
	.clients button[aria-pressed='true'] {
		background: color-mix(in srgb, var(--gold) 16%, transparent);
		color: var(--gold);
		box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--gold) 35%, transparent);
	}
	.plug {
		background: var(--inp);
		color: var(--txt);
		border: 1px solid var(--inpb);
		border-radius: 10px;
		padding: 8px 12px;
		font: 650 13.5px var(--body);
		cursor: pointer;
	}
	.plug:disabled {
		opacity: 0.5;
		cursor: not-allowed;
	}
	.play {
		background: linear-gradient(90deg, var(--g2), var(--g1));
		color: var(--on-gold);
		border: 0;
		border-radius: 13px;
		font: 800 19px var(--disp);
		letter-spacing: -0.01em;
		padding: 15px 36px;
		cursor: pointer;
		box-shadow: 0 6px 22px color-mix(in srgb, var(--gold) 28%, transparent);
		transition: transform 0.12s, filter 0.12s;
	}
	.play:hover:not(:disabled) {
		filter: brightness(1.07);
		transform: translateY(-1px);
	}
	.play:disabled {
		opacity: 0.5;
		cursor: not-allowed;
		box-shadow: none;
	}
	button:focus-visible {
		outline: none;
		box-shadow: 0 0 0 3px color-mix(in srgb, var(--gold) 30%, transparent);
	}
	@media (max-width: 760px) {
		.hero {
			grid-template-columns: minmax(0, 1fr);
		}
	}
	@media (prefers-reduced-motion: reduce) {
		.play {
			transition: none;
		}
	}
</style>
