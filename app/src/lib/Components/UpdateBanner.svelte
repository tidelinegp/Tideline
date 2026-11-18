<script lang="ts">
	import { availableUpdate, installUpdate, updateProgress } from '$lib/Util/Updates';

	let dismissed = '';
	$: busy = ['downloading', 'verifying', 'installing'].includes($updateProgress.state);
</script>

{#if $availableUpdate && dismissed !== $availableUpdate.version}
	<div class="upd" role="status">
		<div class="txt">
			<b>Tideline {$availableUpdate.version} is available.</b>
			{#if busy}
				<span class="mut">{$updateProgress.message}</span>
			{:else if $updateProgress.state === 'error'}
				<span class="err">{$updateProgress.message}</span>
			{:else if $availableUpdate.notes}
				<span class="mut">{$availableUpdate.notes}</span>
			{/if}
		</div>
		<div class="btns">
			<button type="button" class="go" disabled={busy} on:click={() => installUpdate($availableUpdate)}>
				{busy ? 'Updating...' : 'Update now'}
			</button>
			{#if !busy}
				<button type="button" class="later" on:click={() => (dismissed = $availableUpdate?.version ?? '')}>Later</button>
			{/if}
		</div>
	</div>
{/if}

<style>
	.upd {
		display: flex;
		gap: 12px;
		align-items: center;
		justify-content: space-between;
		padding: 10px 16px;
		margin-bottom: 12px;
		border-radius: var(--r);
		border: 1px solid color-mix(in srgb, var(--accent2) 50%, var(--line));
		background: color-mix(in srgb, var(--accent2) 12%, var(--s1));
		font-size: 13.5px;
	}
	.txt {
		display: flex;
		flex-direction: column;
		gap: 2px;
	}
	.mut {
		color: var(--mut);
		font-size: 12.5px;
	}
	.err {
		color: #e5484d;
		font-size: 12.5px;
	}
	.btns {
		display: flex;
		gap: 8px;
		flex: none;
	}
	button {
		border: 0;
		cursor: pointer;
		border-radius: 10px;
		padding: 7px 14px;
		font: 600 13px var(--body);
	}
	.go {
		background: linear-gradient(var(--g1), var(--g2));
		color: var(--on-gold);
	}
	.go:disabled {
		opacity: 0.6;
		cursor: default;
	}
	.later {
		background: none;
		color: var(--mut);
	}
	.later:hover {
		background: var(--hov);
		color: var(--txt);
	}
</style>
