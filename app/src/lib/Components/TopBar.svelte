<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';
	import { Game } from '$lib/Util/Interfaces';
	import { brand } from '$lib/Util/Brand';

	import { LOCAL_DASHBOARD_URL } from '$lib/Util/Config';
	import { installUpdate, latestRelease, updateProgress } from '$lib/Util/Updates';

	const { config, initialized, dashboardInstalled } = GlobalState;

	let { openSettings = $bindable() } = $props();

	// the button shows when the user turned it on, or by default when the bundled dashboard app is installed
	const showDashboard = $derived(
		($config.show_dashboard_button ?? $dashboardInstalled) && !!$config.dashboard_url
	);
	let starting = $state(false);

	// launcher-only installs: fetch the same signed installer and run it with the dashboard ticked
	const addingDashboard = $derived(['downloading', 'verifying', 'installing'].includes($updateProgress.state));
	function addDashboard() {
		if ($latestRelease) installUpdate($latestRelease, true);
	}

	// opens the dashboard in the browser; if it is the bundled app, starts it first and waits until it answers
	async function openDashboard() {
		const url = ($config.dashboard_url ?? '').trim().replace(/\/+$/, '');
		if (!url) return;
		if (url === LOCAL_DASHBOARD_URL && $dashboardInstalled) {
			starting = true;
			await fetch('/start-dashboard').catch(() => {});
			for (let i = 0; i < 40; i++) {
				const up = await fetch(`${url}/api/ping`, { cache: 'no-store' })
					.then((r) => r.ok)
					.catch(() => false);
				if (up) break;
				await new Promise((resolve) => setTimeout(resolve, 500));
			}
			starting = false;
		}
		fetch('/open-external-url', { method: 'POST', body: `${url}/` });
	}

	const today = new Date().toLocaleDateString(undefined, {
		weekday: 'long',
		month: 'long',
		day: 'numeric'
	});
</script>

<header class="head">
	<div class="brand">
		<img class="mark" src="tideline.svg" width="38" height="38" alt="" aria-hidden="true" />
		<div>
			<h1>{brand.name}</h1>
			<small>{today}</small>
		</div>
	</div>
	<span class="spacer"></span>
	<div class="tools">
		<div class="seg" role="group" aria-label="Game">
			<button
				type="button"
				aria-pressed={$config.selected.game === Game.rs3}
				onclick={() => ($config.selected.game = Game.rs3)}>RS3</button
			>
			<button
				type="button"
				aria-pressed={$config.selected.game === Game.osrs}
				onclick={() => ($config.selected.game = Game.osrs)}>OSRS</button
			>
		</div>
		{#if showDashboard}
				<button class="ghost" type="button" title={$config.dashboard_url} disabled={starting} onclick={openDashboard}>
					{starting ? 'Starting…' : 'Dashboard'}
				</button>
			{:else if !$dashboardInstalled && $latestRelease}
				<button class="ghost" type="button" title="Download and add the flipping dashboard" disabled={addingDashboard} onclick={addDashboard}>
					{addingDashboard ? ($updateProgress.message || 'Adding…') : 'Add flipping dashboard'}
				</button>
			{/if}
		<button class="ghost" type="button" disabled={!$initialized} onclick={() => openSettings()}>
			Settings
		</button>
	</div>
</header>

<style>
	/* the whole header row, including its padding, drags the frameless window; buttons opt out */
	.head {
		display: flex;
		align-items: flex-start;
		gap: 14px;
		flex-wrap: wrap;
		margin: 0 -28px;
		padding: 22px 28px 0;
		-webkit-app-region: drag;
	}
	.tools {
		-webkit-app-region: no-drag;
	}
	.brand {
		display: flex;
		align-items: center;
		gap: 12px;
	}
	.mark {
		width: 38px;
		height: 38px;
		display: block;
		flex: none;
	}
	h1 {
		margin: 0;
		font: 700 26px var(--disp);
		letter-spacing: -0.03em;
		line-height: 1;
	}
	small {
		display: block;
		color: var(--mut);
		font-size: 13px;
		margin-top: 3px;
	}
	.spacer {
		flex: 1;
	}
	.tools {
		display: flex;
		gap: 8px;
		align-items: center;
		flex-wrap: wrap;
	}
	.seg {
		display: inline-flex;
		background: var(--s2);
		border: 1px solid var(--line);
		border-radius: 12px;
		padding: 3px;
	}
	.seg button {
		background: none;
		border: 0;
		color: var(--mut);
		font: 650 13px var(--body);
		padding: 7px 16px;
		border-radius: 9px;
		cursor: pointer;
	}
	.seg button[aria-pressed='true'] {
		background: linear-gradient(90deg, color-mix(in srgb, var(--gold) 20%, transparent), color-mix(in srgb, var(--gold) 6%, transparent));
		color: var(--gold);
		box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--gold) 35%, transparent);
	}
	.ghost {
		background: var(--s2);
		border: 1px solid var(--line);
		color: var(--mut);
		border-radius: 10px;
		padding: 8px 13px;
		font: 600 13px var(--body);
		cursor: pointer;
	}
	.ghost:hover:not(:disabled) {
		color: var(--txt);
		border-color: var(--inpb);
	}
	button:focus-visible {
		outline: none;
		box-shadow: 0 0 0 3px color-mix(in srgb, var(--gold) 30%, transparent);
	}
</style>
