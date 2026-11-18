<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';
	import { accentSwatches, defaultThemeId, isValidHex, themes } from '$lib/Util/Themes';

	const { config } = GlobalState;

	const themeId = $derived($config.theme ?? defaultThemeId);
	const customAccent = $derived(isValidHex($config.accent) ? $config.accent : null);
	// the native colour input needs a valid #rrggbb value even when no custom accent is set
	const pickerValue = $derived(
		customAccent ?? themes.find((t) => t.id === themeId)?.dark.accent ?? '#f0c14b'
	);
</script>

<div class="set">
	<h3>Appearance</h3>

	<section>
		<h4>Mode</h4>
		<div class="seg" role="group" aria-label="Light or dark">
			<button type="button" aria-pressed={$config.use_dark_theme} onclick={() => ($config.use_dark_theme = true)}>Dark</button>
			<button type="button" aria-pressed={!$config.use_dark_theme} onclick={() => ($config.use_dark_theme = false)}>Light</button>
		</div>
	</section>

	<section>
		<h4>Theme</h4>
		<div class="grid">
			{#each themes as theme (theme.id)}
				<button
					type="button"
					class="card"
					aria-pressed={themeId === theme.id}
					onclick={() => ($config.theme = theme.id)}
				>
					<span class="preview" style="background:{theme.dark.bg}; border-color:{theme.dark.line}">
						<span class="pside" style="background:{theme.dark.side}; border-color:{theme.dark.line}"></span>
						<span class="pbody">
							<span class="pbar" style="background:{theme.dark.s1}"></span>
							<span class="pbtn" style="background:linear-gradient(90deg,{theme.dark.g2},{theme.dark.g1})"></span>
						</span>
					</span>
					<span class="name">{theme.name}</span>
				</button>
			{/each}
		</div>
	</section>

	<section>
		<h4>Accent colour</h4>
		<p class="hint">Overrides the accent of the chosen theme.</p>
		<div class="accent">
			{#each accentSwatches as color}
				<button
					type="button"
					class="sw"
					aria-label="Use accent {color}"
					aria-pressed={customAccent?.toLowerCase() === color}
					style="background:{color}"
					onclick={() => ($config.accent = color)}
				></button>
			{/each}
			<label class="pick" title="Pick any colour">
				<input
					type="color"
					value={pickerValue}
					oninput={(e) => ($config.accent = (e.currentTarget as HTMLInputElement).value)}
				/>
				<span>Custom</span>
			</label>
			<button class="reset" type="button" disabled={!customAccent} onclick={() => ($config.accent = null)}>
				Reset
			</button>
		</div>
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
		margin: -2px 0 10px;
		color: var(--mut);
		font-size: 12.5px;
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
		padding: 7px 18px;
		border-radius: 9px;
		cursor: pointer;
	}
	.seg button[aria-pressed='true'] {
		background: linear-gradient(90deg, color-mix(in srgb, var(--gold) 20%, transparent), color-mix(in srgb, var(--gold) 6%, transparent));
		color: var(--gold);
		box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--gold) 35%, transparent);
	}
	.grid {
		display: grid;
		grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
		gap: 10px;
	}
	.card {
		display: grid;
		gap: 8px;
		padding: 8px;
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
	.preview {
		display: flex;
		height: 64px;
		border: 1px solid;
		border-radius: 8px;
		overflow: hidden;
	}
	.pside {
		width: 32%;
		border-left: 1px solid;
		order: 2;
	}
	.pbody {
		flex: 1;
		padding: 8px;
		display: grid;
		gap: 6px;
		align-content: start;
	}
	.pbar {
		height: 22px;
		border-radius: 5px;
	}
	.pbtn {
		height: 10px;
		width: 46%;
		border-radius: 4px;
	}
	.name {
		font: 650 13.5px var(--body);
		padding: 0 2px 2px;
	}
	.accent {
		display: flex;
		flex-wrap: wrap;
		gap: 8px;
		align-items: center;
	}
	.sw {
		width: 28px;
		height: 28px;
		border-radius: 9px;
		border: 2px solid transparent;
		cursor: pointer;
	}
	.sw[aria-pressed='true'] {
		border-color: var(--txt);
	}
	.pick {
		display: inline-flex;
		align-items: center;
		gap: 8px;
		background: var(--inp);
		border: 1px solid var(--inpb);
		border-radius: 10px;
		padding: 3px 12px 3px 4px;
		font: 600 13px var(--body);
		cursor: pointer;
	}
	.pick input {
		width: 26px;
		height: 26px;
		padding: 0;
		border: 0;
		background: none;
		cursor: pointer;
	}
	.reset {
		background: none;
		border: 0;
		color: var(--mut);
		font: 600 13px var(--body);
		cursor: pointer;
		padding: 6px 8px;
	}
	.reset:hover:not(:disabled) {
		color: var(--txt);
	}
	.reset:disabled {
		opacity: 0.4;
		cursor: default;
	}
</style>
