<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';
	import type { Session } from '$lib/Util/Interfaces';
	import WindowControls from '$lib/Components/WindowControls.svelte';

	const { config, sessions, initialized, launched } = GlobalState;

	// "5m", "3h", "2d" since the character was last launched, empty if never
	function short(ms: number | undefined): string {
		if (!ms) return '';
		const m = Math.max(0, Math.round((Date.now() - ms) / 60000));
		if (m < 1) return 'now';
		if (m < 60) return `${m}m`;
		const h = Math.round(m / 60);
		return h < 48 ? `${h}h` : `${Math.round(h / 24)}d`;
	}

	let {
		logoutConfirm,
		onLaunch
	}: {
		logoutConfirm: (sub: string, session: Session) => void;
		onLaunch: (sub: string, characterId: string) => void;
	} = $props();

	let collapsed: { [sub: string]: boolean } = $state({});
	const accounts = $derived(Object.entries($sessions));

	function isSelected(sub: string, characterId: string): boolean {
		return (
			$config.selected.user_id === sub && $config.userDetails[sub]?.account_id === characterId
		);
	}

	// first click selects a character, clicking the selected one launches it
	function pick(sub: string, characterId: string) {
		if (isSelected(sub, characterId)) {
			onLaunch(sub, characterId);
			return;
		}
		$config.selected.user_id = sub;
		$config.userDetails[sub] = { account_id: characterId };
	}

	function initial(name: string | undefined): string {
		return (name?.trim()[0] ?? '?').toUpperCase();
	}
</script>

<aside class="side" aria-label="Accounts and characters">
	<div class="winbar">
		<span class="drag"></span>
		<WindowControls />
	</div>
	<div class="side-h">
		<h2>Accounts</h2>
		<small>{accounts.length} {accounts.length === 1 ? 'account' : 'accounts'}</small>
	</div>

	<div class="accts">
		{#each accounts as [sub, session] (sub)}
			{@const chars = Object.entries(session.characters ?? {})}
			<section class="acct">
				<div class="acct-h">
					<button
						class="acct-toggle"
						type="button"
						aria-expanded={!collapsed[sub]}
						onclick={() => (collapsed[sub] = !collapsed[sub])}
					>
						<span class="avatar">{initial(session.displayName)}</span>
						<span class="t">
							<b>{session.displayName}</b>
							<small>{chars.length} {chars.length === 1 ? 'character' : 'characters'}</small>
						</span>
						<span class="chev" class:closed={collapsed[sub]}>&#9662;</span>
					</button>
					<button
						class="logout"
						type="button"
						title="Log out of {session.displayName}"
						onclick={() => logoutConfirm(sub, session)}
					>
						Log out
					</button>
				</div>
				{#if !collapsed[sub]}
					<div class="chars">
						{#each chars as [characterId, character] (characterId)}
							<button
								class="char"
								type="button"
								aria-current={isSelected(sub, characterId)}
								disabled={!$initialized}
								onclick={() => pick(sub, characterId)}
							>
								<span class="dot" class:run={!!$launched[characterId]}></span>
								<span class="n">{character.displayName || 'New Character'}</span>
								<span class="when">{short($config.last_played?.[characterId])}</span>
								<span class="go">PLAY</span>
							</button>
						{:else}
							<p class="none">No characters</p>
						{/each}
					</div>
				{/if}
			</section>
		{/each}
	</div>

	<button class="add" type="button" disabled={!$initialized} onclick={() => fetch('/login')}>
		+ Add Jagex account
	</button>
	<p class="hint">Click a character to select it. Click it again, or press Play, to launch.</p>
</aside>

<style>
	.side {
		width: 320px;
		flex: none;
		border-left: 1px solid var(--line);
		background: linear-gradient(
			180deg,
			color-mix(in srgb, var(--side) 66%, transparent),
			color-mix(in srgb, var(--side) 88%, transparent)
		);
		padding: 22px 16px 18px;
		display: flex;
		flex-direction: column;
		gap: 14px;
		min-width: 0;
		overflow-y: auto;
	}
	.winbar {
		display: flex;
		height: 36px;
		margin: -22px -16px 0;
		flex: none;
	}
	.drag {
		flex: 1;
		-webkit-app-region: drag;
	}
	.side-h {
		display: flex;
		align-items: baseline;
		gap: 8px;
		padding: 0 6px;
	}
	.side-h h2 {
		margin: 0;
		font: 700 19px var(--disp);
		letter-spacing: -0.02em;
	}
	.side-h small {
		color: var(--mut);
		font-size: 12.5px;
		margin-left: auto;
	}
	.accts {
		display: grid;
		gap: 12px;
	}
	.acct {
		background: linear-gradient(180deg, var(--s1), var(--s2));
		border: 1px solid var(--line);
		border-radius: var(--r);
		overflow: hidden;
	}
	.acct-h {
		display: flex;
		align-items: center;
		padding-right: 10px;
	}
	.acct-toggle {
		display: flex;
		align-items: center;
		gap: 10px;
		flex: 1;
		min-width: 0;
		background: none;
		border: 0;
		color: var(--txt);
		padding: 12px 8px 10px 14px;
		cursor: pointer;
		text-align: left;
	}
	.avatar {
		width: 30px;
		height: 30px;
		border-radius: 9px;
		display: grid;
		place-items: center;
		font: 800 13px var(--disp);
		background: var(--inp);
		border: 1px solid var(--inpb);
		color: var(--gold);
		flex: none;
	}
	.t {
		min-width: 0;
	}
	.t b {
		display: block;
		font: 650 15px var(--disp);
		letter-spacing: -0.01em;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.t small {
		color: var(--mut);
		font-size: 12px;
	}
	.chev {
		margin-left: auto;
		color: var(--mut);
		transition: transform 0.15s;
	}
	.chev.closed {
		transform: rotate(-90deg);
	}
	.logout {
		background: none;
		border: 0;
		color: var(--mut);
		font: 600 11.5px var(--body);
		cursor: pointer;
		padding: 4px 6px;
		border-radius: 6px;
	}
	.logout:hover {
		color: var(--txt);
		background: var(--hov);
	}
	.chars {
		display: grid;
		gap: 2px;
		padding: 0 8px 8px;
	}
	.char {
		display: flex;
		align-items: center;
		gap: 10px;
		width: 100%;
		background: none;
		border: 0;
		border-radius: 10px;
		padding: 9px 10px;
		color: var(--txt);
		font: 550 14px var(--body);
		cursor: pointer;
		text-align: left;
	}
	.char:hover:not(:disabled) {
		background: var(--hov);
	}
	.char[aria-current='true'] {
		background: linear-gradient(90deg, color-mix(in srgb, var(--gold) 18%, transparent), color-mix(in srgb, var(--gold) 4%, transparent));
		color: var(--gold);
		font-weight: 650;
		box-shadow: inset 2px 0 0 var(--gold);
	}
	.dot {
		width: 8px;
		height: 8px;
		border-radius: 50%;
		background: var(--inpb);
		flex: none;
	}
	.char[aria-current='true'] .dot {
		background: var(--gold);
	}
	.char .dot.run {
		background: var(--pos);
		box-shadow: 0 0 0 3px color-mix(in srgb, var(--pos) 25%, transparent);
	}
	.when {
		margin-left: auto;
		color: var(--mut);
		font-size: 12px;
		font-weight: 500;
	}
	.char:hover:not(:disabled) .when,
	.char:focus-visible .when {
		display: none;
	}
	.n {
		min-width: 0;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.go {
		display: none;
		margin-left: auto;
		background: linear-gradient(90deg, var(--g2), var(--g1));
		color: var(--on-gold);
		border-radius: 7px;
		padding: 2px 9px;
		font: 800 11px var(--disp);
		letter-spacing: 0.04em;
	}
	.char:hover:not(:disabled) .go,
	.char:focus-visible .go {
		display: inline-block;
	}
	.none {
		color: var(--mut);
		font-size: 12.5px;
		padding: 6px 10px;
		margin: 0;
	}
	.add {
		border: 1px dashed var(--inpb);
		background: none;
		color: var(--mut);
		border-radius: var(--r);
		padding: 12px;
		font: 600 13.5px var(--body);
		cursor: pointer;
	}
	.add:hover:not(:disabled) {
		color: var(--gold);
		border-color: var(--gold);
	}
	.hint {
		color: var(--mut);
		font-size: 12px;
		margin: auto 6px 0;
	}
	button:focus-visible {
		outline: none;
		box-shadow: 0 0 0 3px color-mix(in srgb, var(--gold) 30%, transparent);
	}
	@media (prefers-reduced-motion: reduce) {
		.chev {
			transition: none;
		}
	}
</style>
