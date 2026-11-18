<script lang="ts">
	import DisclaimerModal from '$lib/Components/Modals/DisclaimerModal.svelte';
	import Launch from '$lib/Components/Launch.svelte';
	import LogView from '$lib/Components/LogView.svelte';
	import MainLayout from '$lib/Components/MainLayout.svelte';
	import TopBar from '$lib/Components/TopBar.svelte';
	import News from '$lib/Components/News.svelte';
	import AccountSidebar from '$lib/Components/AccountSidebar.svelte';
	import UpdateBanner from '$lib/Components/UpdateBanner.svelte';
	import { startUpdateChecks } from '$lib/Util/Updates';
	import { logger } from '$lib/Util/Logger';
	import { GlobalState } from '$lib/State/GlobalState';
	import { Client, Game, type Session } from '$lib/Util/Interfaces';
	import LaunchConfirmModal from '$lib/Components/Modals/LaunchConfirmModal.svelte';
	import PluginModal from '$lib/Components/Modals/PluginModal.svelte';
	import { onMount, tick } from 'svelte';
	import LogoutConfirmModal from '$lib/Components/Modals/LogoutConfirmModal.svelte';
	import SettingsModal from '$lib/Components/Modals/SettingsModal.svelte';
	import AvatarModal from '$lib/Components/Modals/AvatarModal.svelte';
	import { startThemeSync } from '$lib/Util/ThemeSync';
	import { markSeenClients, reloadCharacters } from '$lib/Util/Characters';
	import { bolt } from '$lib/State/Bolt';

	const logs = logger.logs;
	let osrsPsa: string | null = null;
	let rs3Psa: string | null = null;
	const { config, isNew } = GlobalState;

	let disclaimerModal: DisclaimerModal;
	let launchConfirmModal: LaunchConfirmModal;
	let logoutConfirmModal: LogoutConfirmModal;
	let settingsModal: SettingsModal;
	let pluginModal: PluginModal;
	let launchPanel: Launch;
	let avatarModal: AvatarModal;
	const editAvatar = (sub: string, name: string) => avatarModal.open(sub, name);

	// ask the native app which game windows are open, so running characters are shown even after
	// the launcher was closed and reopened
	const { clientTitles } = GlobalState;
	const POLL_MS = 4000;
	const pollClients = () =>
		fetch('/running-clients')
			.then((r) => (r.ok ? r.json() : []))
			.then((titles) => {
				if (Array.isArray(titles)) {
					const list = titles.filter((t) => typeof t === 'string');
					clientTitles.set(list);
					markSeenClients(list);
				}
			})
			.catch(() => {});
	onMount(() => {
		fetch('/dashboard-installed')
			.then((r) => GlobalState.dashboardInstalled.set(r.ok))
			.catch(() => {});
		pollClients();
		const timer = setInterval(pollClients, POLL_MS);
		const stopThemeSync = startThemeSync();
		const stopUpdateChecks = startUpdateChecks();
		// if an account came up without characters (e.g. a network hiccup at startup), try again
		const retry = setInterval(() => reloadCharacters(true), 30_000);
		return () => {
			clearInterval(timer);
			clearInterval(retry);
			stopThemeSync();
			stopUpdateChecks();
		};
	});

	let launchConfirm = (launch: (game: Game, client: Client) => void, game: Game, client: Client) => launchConfirmModal.open(launch, game, client);
	let logoutConfirm = (sub: string, session: Session) => (logoutConfirmModal.open(sub, session))
	let openSettings = () => (settingsModal.open());
	let openPluginMenu = () => (pluginModal.open());
	let showDisclaimer = () => (disclaimerModal.open());
	if (isNew) onMount(showDisclaimer);

	// called by the account sidebar: the character is already selected in config, wait for the
	// launch panel's derived values to catch up and then press Play for it
	const launchCharacter = async (_sub: string, _characterId: string) => {
		await tick();
		launchPanel.play();
	};

	const psaClass =
		'mb-5 w-full animate-pulse rounded border border-amber-500 bg-slate-100 p-2 shadow-md duration-200 dark:bg-slate-900';
</script>

<MainLayout>
	<DisclaimerModal bind:this={disclaimerModal} />
	<LaunchConfirmModal bind:this={launchConfirmModal}></LaunchConfirmModal>
	<LogoutConfirmModal bind:this={logoutConfirmModal}></LogoutConfirmModal>
	<SettingsModal bind:this={settingsModal}></SettingsModal>
	<AvatarModal bind:this={avatarModal}></AvatarModal>
	{#if bolt.hasBoltPlugins}
		<PluginModal bind:this={pluginModal}></PluginModal>
	{/if}

	<div class="shell">
		<section class="main" aria-label="Launcher">
				<TopBar bind:openSettings />
				<UpdateBanner />
				{#if $config.check_announcements}
					{@const psa = $config.selected.game === Game.osrs ? osrsPsa : rs3Psa}
					{#if psa}
						<div class="psa" role="status">
							<span class="psa-ico" aria-hidden="true">!</span>
							<span>{psa}</span>
						</div>
					{/if}
				{/if}
			<Launch
				bind:this={launchPanel}
				bind:osrsPsa
				bind:rs3Psa
				bind:launchConfirm
				bind:openPluginMenu
			></Launch>
				{#if $config.fetch_rss_feeds}
					<div class="news-wrap">
						<News />
					</div>
				{/if}
				<LogView logs={$logs} />
		</section>
		<AccountSidebar {logoutConfirm} onLaunch={launchCharacter} onEditAvatar={editAvatar} />
	</div>
</MainLayout>

<style>
	.shell {
		display: flex;
		height: 100%;
		width: 100%;
	}
	.main {
		flex: 1;
		min-width: 0;
		padding: 0 28px 18px;
		display: flex;
		flex-direction: column;
		gap: 18px;
	}
	.news-wrap {
		min-height: 0;
		flex: 1;
		overflow-y: auto;
		border: 1px solid var(--line);
		border-radius: var(--r);
		background: linear-gradient(180deg, var(--s1), var(--s2));
	}
	.psa {
		display: flex;
		gap: 12px;
		align-items: flex-start;
		padding: 12px 16px;
		border-radius: var(--r);
		border: 1px solid color-mix(in srgb, #f2a33a 55%, var(--line));
		background: color-mix(in srgb, #f2a33a 12%, var(--s1));
		font-size: 13.5px;
		line-height: 1.45;
	}
	.psa-ico {
		flex: none;
		width: 22px;
		height: 22px;
		border-radius: 50%;
		display: grid;
		place-items: center;
		background: #f2a33a;
		color: #1c1503;
		font: 800 13px var(--disp);
	}
</style>
