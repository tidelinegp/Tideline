<script lang="ts">
	import DisclaimerModal from '$lib/Components/Modals/DisclaimerModal.svelte';
	import Launch from '$lib/Components/Launch.svelte';
	import LogView from '$lib/Components/LogView.svelte';
	import MainLayout from '$lib/Components/MainLayout.svelte';
	import TopBar from '$lib/Components/TopBar.svelte';
	import News from '$lib/Components/News.svelte';
	import { logger } from '$lib/Util/Logger';
	import { GlobalState } from '$lib/State/GlobalState';
	import { Client, Game, type Session } from '$lib/Util/Interfaces';
	import LaunchConfirmModal from '$lib/Components/Modals/LaunchConfirmModal.svelte';
	import PluginModal from '$lib/Components/Modals/PluginModal.svelte';
	import { onMount } from 'svelte';
	import LogoutConfirmModal from '$lib/Components/Modals/LogoutConfirmModal.svelte';
	import SettingsModal from '$lib/Components/Modals/SettingsModal.svelte';

	const logs = logger.logs;
	let osrsPsa: string | null = null;
	let rs3Psa: string | null = null;
	const { config, isNew } = GlobalState;

	let disclaimerModal: DisclaimerModal;
	let launchConfirmModal: LaunchConfirmModal;
	let logoutConfirmModal: LogoutConfirmModal;
	let settingsModal: SettingsModal;
	let pluginModal: PluginModal;

	let launchConfirm = (launch: (game: Game, client: Client) => void, game: Game, client: Client) => launchConfirmModal.open(launch, game, client);
	let logoutConfirm = (sub: string, session: Session) => (logoutConfirmModal.open(sub, session))
	let openSettings = () => (settingsModal.open());
	let openPluginMenu = () => (pluginModal.open());
	let showDisclaimer = () => (disclaimerModal.open());
	if (isNew) onMount(showDisclaimer);

	const mainClass = 'mt-16 grid h-full grid-flow-col grid-cols-2 sm:grid-cols-3';
	const mainClassNoNews = 'mt-16 grid h-full grid-flow-col grid-cols-3';
	const psaClass =
		'mb-5 w-full animate-pulse rounded border border-amber-500 bg-slate-100 p-2 shadow-md duration-200 dark:bg-slate-900';
</script>

{#snippet launch()}
	<Launch bind:osrsPsa bind:rs3Psa bind:launchConfirm bind:openPluginMenu></Launch>
{/snippet}

{#snippet launchMid()}
	<div class="col-span-1 col-start-2">
		{@render launch()}
	</div>
{/snippet}

<MainLayout>
	<DisclaimerModal bind:this={disclaimerModal} />
	<LaunchConfirmModal bind:this={launchConfirmModal}></LaunchConfirmModal>
	<LogoutConfirmModal bind:this={logoutConfirmModal}></LogoutConfirmModal>
	<SettingsModal bind:this={settingsModal}></SettingsModal>
	<PluginModal bind:this={pluginModal}></PluginModal>
	<TopBar bind:logoutConfirm bind:openSettings />
	<div class={$config.fetch_rss_feeds ? mainClass : mainClassNoNews}>
		{#if $config.fetch_rss_feeds}
			<div class="col-span-1 sm:col-span-2">
				<News bind:osrsPsa bind:rs3Psa />
			</div>
			<div class="col-span-1 -ml-4">
				{@render launch()}
			</div>
		{:else if $config.check_announcements}
			<div class="col-span-1 m-4">
				{#if osrsPsa && $config.selected.game === Game.osrs}
					<div class={psaClass}>
						{osrsPsa}
					</div>
				{:else if rs3Psa && $config.selected.game === Game.rs3}
					<div class={psaClass}>
						{rs3Psa}
					</div>
				{/if}
			</div>
			{@render launchMid()}
		{:else}
			{@render launchMid()}
		{/if}
	</div>
	<LogView logs={$logs} />
</MainLayout>
