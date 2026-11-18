<script lang="ts">
	import { Severity, type Log } from '$lib/Util/Logger';

	interface Props {
		logs: Log[];
	}

	let { logs }: Props = $props();
	const recent = $derived(logs.slice(-3));
</script>

<div class="log" id="message_list" aria-live="polite">
	{#each recent as { date, severity, content }}
		<div class:err={severity === Severity.error} class:warn={severity === Severity.warn}>
			<b>{date?.toLocaleTimeString([], { hour: 'numeric', minute: '2-digit' })}</b>
			{content}
		</div>
	{/each}
</div>

<style>
	.log {
		margin-top: auto;
		color: var(--mut);
		font-size: 12.5px;
		display: grid;
		gap: 2px;
		min-height: 52px;
		align-content: end;
	}
	b {
		color: var(--gold);
		font-weight: 650;
	}
	.err {
		color: #f87171;
	}
	.warn {
		color: #f2a33a;
	}
</style>
