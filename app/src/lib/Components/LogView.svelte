<script lang="ts">
	import { Severity, type Log } from '$lib/Util/Logger';
	import { brand } from '$lib/Util/Brand';

	interface Props {
		logs: Log[];
	}

	let { logs }: Props = $props();
	const latest = $derived(logs.length > 0 ? logs[logs.length - 1] : null);
</script>

<footer class="foot" id="message_list" aria-live="polite">
	<span
		class="status"
		class:err={latest?.severity === Severity.error}
		class:warn={latest?.severity === Severity.warn}
	>
		<span class="led"></span>
		{#if latest}
			<b>{latest.date?.toLocaleTimeString([], { hour: 'numeric', minute: '2-digit' })}</b>
			<span class="msg">{latest.content}</span>
		{:else}
			<span class="msg">Ready</span>
		{/if}
	</span>
	<span class="ver">{brand.name} {brand.version}</span>
</footer>

<style>
	.foot {
		margin-top: auto;
		display: flex;
		align-items: center;
		gap: 12px;
		padding-top: 12px;
		border-top: 1px solid var(--line);
		color: var(--mut);
		font-size: 12.5px;
	}
	.status {
		display: flex;
		align-items: center;
		gap: 8px;
		min-width: 0;
	}
	.led {
		flex: none;
		width: 7px;
		height: 7px;
		border-radius: 50%;
		background: var(--pos);
	}
	.warn .led {
		background: #f2a33a;
	}
	.err .led {
		background: #f87171;
	}
	.err .msg {
		color: #f87171;
	}
	b {
		color: var(--gold);
		font-weight: 650;
		flex: none;
	}
	.msg {
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.ver {
		margin-left: auto;
		flex: none;
	}
</style>
