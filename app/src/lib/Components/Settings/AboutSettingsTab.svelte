<script lang="ts">
	import { bolt } from '$lib/State/Bolt';
	import { brand } from '$lib/Util/Brand';

	const open = (url: string) => fetch('/open-external-url', { method: 'POST', body: url });
</script>

<div class="set">
	<h3>About {brand.name}</h3>
	<p class="ver">Version {brand.version}, based on {brand.basedOn} {bolt.boltVersion ?? ''}</p>

	<section>
		<h4>Credits</h4>
		<p>
			{brand.name} is a modified version of
			<button class="link" type="button" onclick={() => open(brand.basedOnUrl)}>{brand.basedOn}</button>,
			created by {brand.basedOnAuthor}, and is free software under the {brand.licence}. It adds a new
			interface and some extra features. Thank you to {brand.basedOnAuthor} and the contributors to
			{brand.basedOn} for the launcher it is built on.
		</p>
		<div class="links">
			<button class="ghost" type="button" onclick={() => open(brand.licenceUrl)}>Read the licence</button>
			{#if brand.sourceUrl}
				<button class="ghost" type="button" onclick={() => open(brand.sourceUrl)}>Get the source code</button>
			{/if}
			<button class="ghost" type="button" onclick={() => open(brand.basedOnUrl)}>Original {brand.basedOn} project</button>
		</div>
	</section>

	<section>
		<h4>Not affiliated with Jagex</h4>
		<p>
			Created using intellectual property belonging to Jagex Limited under the terms of Jagex's
			<button class="link" type="button" onclick={() => open(brand.fanContentPolicyUrl)}>Fan Content Policy</button>.
			This content is not endorsed by or affiliated with Jagex.
		</p>
		<p>
			{brand.name} is a fan-made project. RuneScape and Old School RuneScape are trademarks of Jagex Limited.
		</p>
	</section>
</div>

<style>
	h3 {
		margin: 0 0 4px;
		font: 700 22px var(--disp);
		letter-spacing: -0.02em;
	}
	.ver {
		margin: 0 0 18px;
		color: var(--mut);
		font-size: 13px;
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
	p {
		margin: 0 0 12px;
		max-width: 60ch;
		font-size: 13.5px;
		line-height: 1.5;
	}
	.link {
		background: none;
		border: 0;
		padding: 0;
		color: var(--gold);
		font: inherit;
		cursor: pointer;
		text-decoration: underline;
	}
	.links {
		display: flex;
		gap: 8px;
		flex-wrap: wrap;
	}
	.ghost {
		background: var(--inp);
		border: 1px solid var(--inpb);
		color: var(--txt);
		border-radius: 10px;
		padding: 8px 14px;
		font: 600 13px var(--body);
		cursor: pointer;
	}
	.ghost:hover {
		border-color: var(--gold);
		color: var(--gold);
	}
</style>
