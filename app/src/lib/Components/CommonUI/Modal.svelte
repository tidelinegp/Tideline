<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';

	const { config } = GlobalState;
	let darkTheme = $derived($config.use_dark_theme);

	interface Props {
		class?: string;
		canSelfClose?: boolean;
		onClose?: () => void;
		children?: import('svelte').Snippet;
	}

	let {
		class: className = '',
		canSelfClose = true,
		onClose = () => {},
		children
	}: Props = $props();

	let dialog: HTMLDialogElement;
	let isOpen = $state(false);

	export function open() {
		dialog.showModal();
		isOpen = true;
	}

	export function close() {
		onClose();
		dialog.close();
		isOpen = false;
	}
</script>

<dialog
	bind:this={dialog}
	onkeydown={(e) => {
		if (e.key !== 'Escape') return;
		e.preventDefault();
		if (canSelfClose) close();
	}}
	onmousedown={(e: MouseEvent) => {
		if (canSelfClose && dialog === (e.target as Node)) close();
	}}
	class:dark={darkTheme}
	class="bolt-dialog {className}"
>
	{#if canSelfClose}
		<button class="x" type="button" aria-label="Close" onclick={close}>
			<svg width="12" height="12" viewBox="0 0 12 12"><path d="M1.5 1.5l9 9M10.5 1.5l-9 9" stroke="currentColor" stroke-width="1.5" /></svg>
		</button>
	{/if}
	{#if isOpen}
		{@render children?.()}
	{/if}
</dialog>

<style>
	.bolt-dialog {
		background: linear-gradient(180deg, var(--s1), var(--s2));
		color: var(--txt);
		font-family: var(--body);
		border: 1px solid var(--line);
		border-radius: var(--r);
		max-height: 90%;
		max-width: 90%;
		overflow: auto;
		padding: 0;
		box-shadow: 0 24px 60px rgba(0, 0, 0, 0.55);
	}
	.bolt-dialog:focus-visible {
		outline: none;
	}
	.bolt-dialog::backdrop {
		background: rgba(5, 8, 14, 0.74);
	}
	.x {
		position: absolute;
		right: 14px;
		top: 14px;
		width: 30px;
		height: 30px;
		display: grid;
		place-items: center;
		border-radius: 9px;
		border: 1px solid var(--line);
		background: var(--s2);
		color: var(--mut);
		cursor: pointer;
		z-index: 2;
	}
	.x:hover {
		color: var(--txt);
		border-color: var(--inpb);
	}
</style>
