<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';

	const { config } = GlobalState;

	let status = $state('');
	let testing = $state(false);

	// asks the dashboard for its shared theme, just to see whether it answers
	async function test() {
		testing = true;
		status = '';
		const base = ($config.dashboard_url ?? '').trim().replace(/\/+$/, '');
		if (!/^https?:\/\/\S+$/i.test(base)) {
			status = 'Enter a link that starts with http:// or https://';
			testing = false;
			return;
		}
		try {
			const r = await fetch(`${base}/api/theme`, { cache: 'no-store' });
			status = r.ok ? 'Connected to the dashboard.' : `The dashboard answered with an error (${r.status}).`;
		} catch {
			status = 'Could not reach the dashboard. Is it running?';
		}
		testing = false;
	}
</script>

<div class="set">
	<h3>Integrations</h3>
	<p class="intro">
		Optional links to a dashboard website running on this PC. Nothing is sent anywhere else, and
		everything here is off until you turn it on.
	</p>

	<label class="field" for="dash_url">
		<b>Dashboard link</b>
		<small>The address of the website, for example http://127.0.0.1:8080</small>
	</label>
	<div class="link">
		<input
			id="dash_url"
			type="text"
			placeholder="http://127.0.0.1:8080"
			bind:value={$config.dashboard_url}
		/>
		<button class="ghost" type="button" disabled={testing} onclick={test}>Test</button>
	</div>
	{#if status}<p class="status">{status}</p>{/if}

	<label class="opt">
		<input id="show_dashboard" type="checkbox" bind:checked={$config.show_dashboard_button} />
		<span>
			<b>Show an Open dashboard button</b>
			<small>Adds a button to the top bar that opens the link above in your browser.</small>
		</span>
	</label>
	<label class="opt">
		<input id="sync_theme" type="checkbox" bind:checked={$config.sync_dashboard_theme} />
		<span>
			<b>Sync colour theme with the dashboard</b>
			<small>
				Picking a theme here changes the dashboard and the other way round. Only the theme name and
				its four colours are shared.
			</small>
		</span>
	</label>
</div>

<style>
	h3 {
		margin: 0 0 8px;
		font: 700 22px var(--disp);
		letter-spacing: -0.02em;
	}
	.intro {
		margin: 0 0 18px;
		color: var(--mut);
		font-size: 13px;
		max-width: 56ch;
	}
	.field b {
		display: block;
		font: 650 14.5px var(--body);
	}
	.field small {
		color: var(--mut);
		font-size: 12.5px;
	}
	.link {
		display: flex;
		gap: 8px;
		margin: 8px 0 6px;
	}
	.link input {
		flex: 1;
		min-width: 0;
	}
	.ghost {
		background: var(--inp);
		border: 1px solid var(--inpb);
		color: var(--txt);
		border-radius: 10px;
		padding: 8px 16px;
		font: 600 13px var(--body);
		cursor: pointer;
	}
	.ghost:hover:not(:disabled) {
		border-color: var(--gold);
		color: var(--gold);
	}
	.status {
		margin: 4px 0 0;
		color: var(--mut);
		font-size: 12.5px;
	}
	.opt {
		display: flex;
		gap: 14px;
		align-items: flex-start;
		padding: 12px 14px;
		border: 1px solid var(--line);
		border-radius: 12px;
		background: var(--s2);
		margin-top: 12px;
		cursor: pointer;
	}
	.opt:hover {
		border-color: var(--inpb);
	}
	.opt input {
		margin-top: 3px;
	}
	.opt b {
		display: block;
		font: 650 14.5px var(--body);
	}
	.opt small {
		color: var(--mut);
		font-size: 12.5px;
	}
</style>
