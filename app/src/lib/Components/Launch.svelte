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
	import LaunchConfirmModal from './Modals/LaunchConfirmModal.svelte';
	import PluginModal from './Modals/PluginModal.svelte';

	let confirmModal: LaunchConfirmModal;
	let pluginModal: PluginModal;
	let { config, initialized, sessions } = GlobalState;
	let selectedUserId = $derived($config.selected.user_id);
	let selectedCharacterId = $derived(
		selectedUserId ? $config.userDetails[selectedUserId]?.account_id : null
	);
	let session = $derived(selectedUserId ? $sessions[selectedUserId] : null);
	let characterIds = $derived(session ? Object.keys(session.characters) : null);

	// messages about game downtime, retrieved from game server
	let { osrsPsa = $bindable(), rs3Psa = $bindable() } = $props();
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

	function handleAccountChange() {
		if (!selectedUserId) return;
		if (selectedCharacterId) {
			$config.userDetails[selectedUserId] = { account_id: selectedCharacterId };
		} else {
			delete $config.userDetails[selectedUserId];
		}
	}
</script>

<LaunchConfirmModal bind:this={confirmModal}></LaunchConfirmModal>
<PluginModal bind:this={pluginModal}></PluginModal>

<div class="bg-grad flex h-full flex-col border-slate-300 pt-5 duration-200 dark:border-slate-800">
	<div class="flex flex-col items-center gap-4">
		<img
			src="svgs/rocket-solid.svg"
			alt="Launch icon"
			class="mb-3 w-24 rounded-3xl bg-gradient-to-br from-rose-500 to-violet-500 p-5"
		/>
		<button
			class="w-52 rounded-lg bg-emerald-500 p-2 font-bold text-black duration-200 enabled:hover:opacity-75 disabled:bg-gray-500"
			disabled={!$initialized}
			onclick={() => {
				if (gameEnabled) {
					launch($config.selected.game, $config.selected.client);
				} else {
					confirmModal.open(launch, $config.selected.game, $config.selected.client);
				}
			}}
		>
			Play
		</button>
		{#if $config.selected.game == Game.osrs}
			<label class="flex flex-col">
				<span class="text-sm">Game Client</span>
				<select
					class="w-52 cursor-pointer rounded-lg border-2 border-slate-300 bg-inherit p-2 text-inherit duration-200 hover:opacity-75 dark:border-slate-800"
					bind:value={$config.selected.client}
				>
					{#each clientMap[$config.selected.game] as client}
						<option class="dark:bg-slate-900" value={client}>{client}</option>
					{/each}
				</select>
			</label>
		{:else if $config.selected.game === Game.rs3}
			<button
				disabled={!bolt.hasBoltPlugins}
				title={bolt.hasBoltPlugins ? null : 'This feature is disabled'}
				class="w-52 rounded-lg p-2 font-bold text-black duration-200 enabled:bg-blue-500 enabled:hover:opacity-75 disabled:bg-gray-500"
				onclick={() => {
					pluginModal.open();
				}}
			>
				Plugin menu
			</button>
		{/if}
		<label class="flex flex-col">
			<span class="text-sm">Character</span>
			<select
				class="mx-auto w-52 cursor-pointer rounded-lg border-2 border-slate-300 bg-inherit p-2 text-inherit duration-200 hover:opacity-75 dark:border-slate-800"
				disabled={!$initialized || $config.selected.user_id === null}
				bind:value={selectedCharacterId}
				onchange={handleAccountChange}
			>
				<option value={undefined} disabled class="dark:bg-slate-900">Select an account</option>
				{#if session && characterIds && characterIds.length > 0}
					{#each characterIds as characterId}
						{@const character = session.characters[characterId]}
						<option value={characterId} class="dark:bg-slate-900">
							{#if character.displayName}
								{character.displayName}
							{:else}
								New Character
							{/if}
						</option>
					{/each}
				{:else}
					<option class="dark:bg-slate-900" disabled selected>No characters</option>
				{/if}
			</select>
		</label>
	</div>
</div>
