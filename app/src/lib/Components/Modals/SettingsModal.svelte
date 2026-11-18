<script lang="ts">
	import Modal from '$lib/Components/CommonUI/Modal.svelte';
	import GeneralSettingsTab from '$lib/Components/Settings/GeneralSettingsTab.svelte';
	import OsrsSettingsTab from '$lib/Components/Settings/OsrsSettingsTab.svelte';
	import Rs3SettingsTab from '$lib/Components/Settings/Rs3SettingsTab.svelte';
	import AppearanceSettingsTab from '$lib/Components/Settings/AppearanceSettingsTab.svelte';
	import IntegrationsSettingsTab from '$lib/Components/Settings/IntegrationsSettingsTab.svelte';
	import { saveConfig } from '$lib/Util/Config';

	// settings categories
	enum Tab {
		general,
		appearance,
		integrations,
		osrs,
		rs3
	}

	let modal: Modal;
	let openTab: Tab = $state(Tab.general);

	const tabs = [
		{ id: Tab.general, label: 'General' },
		{ id: Tab.appearance, label: 'Appearance' },
		{ id: Tab.integrations, label: 'Integrations' },
		{ id: Tab.osrs, label: 'OSRS' },
		{ id: Tab.rs3, label: 'RS3' }
	];

	export function open() {
		modal.open();
	}

	const close = () => saveConfig();
</script>

<Modal bind:this={modal} class="settings select-none" onClose={close}>
	<div class="wrap">
		<nav class="tabs" aria-label="Settings sections">
			<h2>Settings</h2>
			{#each tabs as tab}
				<button type="button" aria-current={openTab === tab.id} onclick={() => (openTab = tab.id)}>
					{tab.label}
				</button>
			{/each}
		</nav>

		<div class="content">
			{#if openTab == Tab.general}
				<GeneralSettingsTab />
			{:else if openTab == Tab.appearance}
				<AppearanceSettingsTab />
			{:else if openTab == Tab.integrations}
				<IntegrationsSettingsTab />
			{:else if openTab == Tab.osrs}
				<OsrsSettingsTab />
			{:else if openTab == Tab.rs3}
				<Rs3SettingsTab />
			{/if}
		</div>
	</div>
</Modal>

<style>
	:global(dialog.settings) {
		width: min(720px, 90%);
		height: min(520px, 90%);
	}
	.wrap {
		display: grid;
		grid-template-columns: 190px minmax(0, 1fr);
		height: 100%;
	}
	.tabs {
		display: flex;
		flex-direction: column;
		gap: 4px;
		padding: 22px 14px;
		border-right: 1px solid var(--line);
		background: color-mix(in srgb, var(--side) 70%, transparent);
	}
	h2 {
		margin: 0 8px 14px;
		font: 700 19px var(--disp);
		letter-spacing: -0.02em;
	}
	.tabs button {
		text-align: left;
		background: none;
		border: 0;
		border-radius: 10px;
		padding: 10px 12px;
		color: var(--txt);
		font: 600 14px var(--body);
		cursor: pointer;
	}
	.tabs button:hover {
		background: var(--hov);
	}
	.tabs button[aria-current='true'] {
		background: linear-gradient(90deg, color-mix(in srgb, var(--gold) 18%, transparent), color-mix(in srgb, var(--gold) 4%, transparent));
		color: var(--gold);
		box-shadow: inset 2px 0 0 var(--gold);
	}
	.content {
		padding: 26px 28px;
		overflow-y: auto;
		text-align: left;
	}
</style>
