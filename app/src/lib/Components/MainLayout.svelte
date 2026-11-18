<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';
	import { themeColors, themeIsDark, themeStyle } from '$lib/Util/Themes';
	interface Props {
		children?: import('svelte').Snippet;
	}

	let { children }: Props = $props();

	const { config } = GlobalState;
	const colors = $derived(themeColors($config.theme, $config.custom_theme));
	const darkTheme = $derived(themeIsDark(colors));
	const vars = $derived(themeStyle(colors));

	// the theme decides light or dark; keep the saved flag (used by older dark-mode styles) in step
	$effect(() => {
		if ($config.use_dark_theme !== darkTheme) $config.use_dark_theme = darkTheme;
	});
</script>

<main class:dark={darkTheme} class="bolt-root fixed top-0 h-screen w-screen text-sm" style={vars}>
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
