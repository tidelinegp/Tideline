<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';
	import { Game } from '$lib/Util/Interfaces';

	const { config, initialized } = GlobalState;

	let { openSettings = $bindable() } = $props();

	const today = new Date().toLocaleDateString(undefined, {
		weekday: 'long',
		month: 'long',
		day: 'numeric'
	});
</script>

<header class="head">
	<div class="brand">
		<div class="mark" aria-hidden="true">
			<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M13 2 4 14h6l-1 8 10-13h-6z" /></svg>
		</div>
		<div>
			<h1>Bolt</h1>
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
		<button
			class="ghost"
			type="button"
			disabled={!$initialized}
			onclick={() => ($config.use_dark_theme = !$config.use_dark_theme)}
		>
			{$config.use_dark_theme ? 'Light' : 'Dark'}
		</button>
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
		width: 36px;
		height: 36px;
		border-radius: 10px;
		background: linear-gradient(160deg, var(--g1), var(--g2));
		display: grid;
		place-items: center;
		color: var(--on-gold);
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
		background: linear-gradient(90deg, rgba(240, 193, 75, 0.2), rgba(240, 193, 75, 0.06));
		color: var(--gold);
		box-shadow: inset 0 0 0 1px rgba(240, 193, 75, 0.35);
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
		box-shadow: 0 0 0 3px rgba(240, 193, 75, 0.3);
	}
</style>
