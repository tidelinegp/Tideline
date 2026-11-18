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

	let { config, initialized, sessions } = GlobalState;
	let selectedUserId = $derived($config.selected.user_id);
	let selectedCharacterId = $derived(
		selectedUserId ? $config.userDetails[selectedUserId]?.account_id : null
	);
	let session = $derived(selectedUserId ? $sessions[selectedUserId] : null);
	let selectedCharacter = $derived(
		session && selectedCharacterId ? session.characters[selectedCharacterId] : null
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

	// the Play button, also called by the account sidebar after it selects a character
	export function play(): void {
		if (gameEnabled) {
			launch($config.selected.game, $config.selected.client);
		} else {
			launchConfirm(launch, $config.selected.game, $config.selected.client);
		}
	}
</script>

<section class="hero" aria-label="Ready to play">
	<div class="info">
		<div class="eyebrow">{selectedCharacter ? 'Ready to play' : 'Pick a character'}</div>
		<h2>
			{selectedCharacter?.displayName ||
				(Object.keys($sessions).length > 0 ? 'Choose a character' : 'Log in to start')}
		</h2>
		<div class="acct-line">
			{session ? `Jagex account ${session.displayName}` : 'Add a Jagex account in the sidebar'}
		</div>
		<div class="facts">
			{#if $config.selected.game == Game.osrs}
				<div>
					<small>Game client</small>
					<select aria-label="Game client" bind:value={$config.selected.client}>
						{#each clientMap[$config.selected.game] as client}
							<option value={client}>{client}</option>
						{/each}
					</select>
				</div>
			{:else if $config.selected.game === Game.rs3}
				<div>
					<small>RS3 plugins</small>
					<button
						class="plug"
						type="button"
						disabled={!bolt.hasBoltPlugins}
						title={bolt.hasBoltPlugins ? null : 'This feature is disabled'}
						onclick={openPluginMenu}
					>
						Plugin menu
					</button>
				</div>
			{/if}
		</div>
	</div>
	<div class="playcol">
		<button class="play" type="button" disabled={!$initialized || !selectedCharacter} onclick={play}>
			Play
		</button>
	</div>
</section>

<style>
	.hero {
		position: relative;
		background: linear-gradient(180deg, var(--s1), var(--s2));
		border: 1px solid var(--line);
		border-radius: var(--r);
		padding: 26px 28px;
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
		background: linear-gradient(90deg, var(--gold), transparent);
	}
	.eyebrow {
		letter-spacing: 0.16em;
		font-size: 11px;
		font-weight: 700;
		color: var(--gold);
		text-transform: uppercase;
	}
	h2 {
		margin: 8px 0 4px;
		font: 800 clamp(30px, 5vw, 54px) var(--disp);
		letter-spacing: -0.04em;
		line-height: 1.05;
		overflow-wrap: anywhere;
	}
	.acct-line {
		color: var(--mut);
		font-size: 15px;
	}
	.facts {
		display: flex;
		gap: 22px;
		flex-wrap: wrap;
		margin-top: 20px;
	}
	.facts > div {
		border-left: 2px solid var(--gold);
		padding: 2px 0 2px 11px;
		min-width: 0;
	}
	.facts small {
		display: block;
		color: var(--mut);
		font-size: 12px;
	}
	select {
		background: var(--inp);
		color: var(--txt);
		border: 1px solid var(--inpb);
		border-radius: 9px;
		padding: 6px 10px;
		font: 650 14px var(--body);
		margin-top: 2px;
		cursor: pointer;
	}
	select option {
		background: var(--s1);
		color: var(--txt);
	}
	.plug {
		background: var(--inp);
		color: var(--txt);
		border: 1px solid var(--inpb);
		border-radius: 9px;
		padding: 6px 12px;
		font: 650 14px var(--body);
		margin-top: 2px;
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
		border-radius: 14px;
		font: 800 20px var(--disp);
		letter-spacing: -0.01em;
		padding: 18px 44px;
		cursor: pointer;
	}
	.play:hover:not(:disabled) {
		filter: brightness(1.07);
	}
	.play:disabled {
		opacity: 0.45;
		cursor: not-allowed;
	}
	:is(button, select):focus-visible {
		outline: none;
		box-shadow: 0 0 0 3px rgba(240, 193, 75, 0.3);
	}
	@media (max-width: 760px) {
		.hero {
			grid-template-columns: minmax(0, 1fr);
		}
	}
</style>
