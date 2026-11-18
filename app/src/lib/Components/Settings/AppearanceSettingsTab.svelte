<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';
	import {
		autoThemeId,
		customThemeId,
		defaultCustom,
		defaultThemeId,
		themeColors,
		themes,
		type ThemeColors
	} from '$lib/Util/Themes';

	const { config } = GlobalState;

	const themeId = $derived($config.theme ?? defaultThemeId);
	const current = $derived(themeColors($config.theme, $config.custom_theme));
	const custom = $derived(themeColors(customThemeId, $config.custom_theme));

	const fields: { key: keyof ThemeColors; label: string; hint: string }[] = [
		{ key: 'bg', label: 'Background', hint: 'The window and panels' },
		{ key: 'accent', label: 'Main colour', hint: 'Buttons, selection, highlights' },
		{ key: 'accent2', label: 'Second colour', hint: 'Small accents' },
		{ key: 'txt', label: 'Text', hint: 'Text and shades derived from it' }
	];

	// editing a colour turns the current theme into "My theme", starting from the colours on screen
	function setColor(key: keyof ThemeColors, value: string) {
		$config.custom_theme = { ...current, [key]: value };
		$config.theme = customThemeId;
	}

	function reset() {
		$config.custom_theme = { ...defaultCustom };
	}
</script>

<div class="set">
	<h3>Appearance</h3>

	<section>
		<h4>Themes</h4>
		<div class="grid">
			<button
				type="button"
				class="card"
				aria-pressed={themeId === autoThemeId}
				onclick={() => ($config.theme = autoThemeId)}
				title="Tideline when Windows is in dark mode, Sandbar when it is in light mode"
			>
				<span class="sw split" style="background:linear-gradient(135deg, {themes[0].bg} 50%, {themes[1].bg} 50%)">
					<i style="background:{themes[0].accent}"></i>
					<i style="background:{themes[1].accent}"></i>
				</span>
				<span class="name">Match Windows</span>
			</button>
			{#each themes as theme (theme.id)}
				<button
					type="button"
					class="card"
					aria-pressed={themeId === theme.id}
					onclick={() => ($config.theme = theme.id)}
				>
					<span class="sw" style="background:{theme.bg}">
						<i style="background:{theme.accent}"></i>
						<i style="background:{theme.accent2}"></i>
						<i style="background:{theme.txt}"></i>
					</span>
					<span class="name">{theme.name}</span>
				</button>
			{/each}
			<button
				type="button"
				class="card"
				aria-pressed={themeId === customThemeId}
				onclick={() => ($config.theme = customThemeId)}
			>
				<span class="sw" style="background:{custom.bg}">
					<i style="background:{custom.accent}"></i>
					<i style="background:{custom.accent2}"></i>
					<i style="background:{custom.txt}"></i>
				</span>
				<span class="name">My theme</span>
			</button>
		</div>
	</section>

	<section>
		<h4>Make your own</h4>
		<p class="hint">
			Change any colour and the launcher switches to My theme, starting from the theme you were
			using. Every other shade is worked out from these four.
		</p>
		<div class="fields">
			{#each fields as f (f.key)}
				<label class="field">
					<span class="lbl">
						<b>{f.label}</b>
						<small>{f.hint}</small>
					</span>
					<input
						type="color"
						value={current[f.key]}
						oninput={(e) => setColor(f.key, (e.currentTarget as HTMLInputElement).value)}
					/>
				</label>
			{/each}
		</div>
		<button class="reset" type="button" onclick={reset}>Reset My theme</button>
	</section>
</div>

<style>
	h3 {
		margin: 0 0 14px;
		font: 700 22px var(--disp);
		letter-spacing: -0.02em;
	}
	h4 {
		margin: 0 0 8px;
		font: 650 12px var(--body);
		letter-spacing: 0.1em;
		text-transform: uppercase;
		color: var(--mut);
	}
	section {
		margin-bottom: 22px;
	}
	.hint {
		margin: -2px 0 12px;
		color: var(--mut);
		font-size: 12.5px;
		max-width: 52ch;
	}
	.grid {
		display: grid;
		grid-template-columns: repeat(auto-fill, minmax(118px, 1fr));
		gap: 8px;
	}
	.card {
		display: grid;
		gap: 7px;
		padding: 7px;
		background: var(--s2);
		border: 1px solid var(--line);
		border-radius: 12px;
		color: var(--txt);
		cursor: pointer;
		text-align: left;
	}
	.card:hover {
		border-color: var(--inpb);
	}
	.card[aria-pressed='true'] {
		border-color: var(--gold);
		box-shadow: 0 0 0 1px var(--gold);
	}
	.sw {
		display: flex;
		align-items: flex-end;
		gap: 5px;
		height: 38px;
		padding: 7px;
		border-radius: 8px;
		border: 1px solid color-mix(in srgb, var(--txt) 14%, transparent);
	}
	.sw i {
		display: block;
		width: 14px;
		height: 14px;
		border-radius: 50%;
	}
	.name {
		font: 650 13px var(--body);
		padding: 0 2px 1px;
	}
	.fields {
		display: grid;
		grid-template-columns: repeat(2, minmax(0, 1fr));
		gap: 8px;
	}
	.field {
		display: flex;
		align-items: center;
		justify-content: space-between;
		gap: 12px;
		padding: 10px 12px;
		background: var(--s2);
		border: 1px solid var(--line);
		border-radius: 12px;
		cursor: pointer;
	}
	.lbl b {
		display: block;
		font: 650 13.5px var(--body);
	}
	.lbl small {
		color: var(--mut);
		font-size: 11.5px;
	}
	.field input {
		flex: none;
		width: 46px;
		height: 30px;
		padding: 2px;
		border: 1px solid var(--inpb);
		border-radius: 8px;
		background: var(--inp);
		cursor: pointer;
	}
	.reset {
		margin-top: 12px;
		background: none;
		border: 0;
		color: var(--mut);
		font: 600 13px var(--body);
		cursor: pointer;
		padding: 6px 2px;
	}
	.reset:hover {
		color: var(--txt);
	}
</style>
