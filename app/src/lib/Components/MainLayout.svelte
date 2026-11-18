<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';
	interface Props {
		children?: import('svelte').Snippet;
	}

	let { children }: Props = $props();

	const { config } = GlobalState;
	let darkTheme = $derived($config.use_dark_theme);
</script>

<main class:dark={darkTheme} class="bolt-root fixed top-0 h-screen w-screen text-sm">
	{@render children?.()}
</main>

<style>
	main {
		background:
			radial-gradient(900px 420px at 18% -10%, var(--glow1), transparent 70%),
			radial-gradient(700px 400px at 90% 110%, var(--glow2), transparent 70%), var(--bg);
		color: var(--txt);
		font-family: var(--body);
		border: 1px solid var(--line);
	}
</style>
