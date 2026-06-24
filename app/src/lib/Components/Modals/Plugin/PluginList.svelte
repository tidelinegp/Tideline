<script lang="ts">
	import PluginEntry from './PluginEntry.svelte';
	import { type PluginMeta } from '$lib/Util/Interfaces';
	import type { ClientInfo } from '$lib/Util/Store';
	import { savePluginConfig } from '$lib/Util/Config';
	import { bolt } from '$lib/State/Bolt';

	interface Props {
		clientId: number;
		accountID?: string;
		pluginList: { [key: string]: PluginMeta };
		activePlugins: ClientInfo['plugins'];
		startPlugin: (clientId: number, pluginId: string, path: string | null) => void;
		stopPlugin: (client: number, uid: number) => void;
	}

	let { clientId, accountID, pluginList, activePlugins, startPlugin, stopPlugin }: Props = $props();

	let autostart = $state(bolt.autostart ?? {});

	const isPluginAutostart = (pluginId: string): boolean => {
		if (!accountID || !autostart) return false;
		return autostart[accountID]?.includes(pluginId) ?? false;
	};

	const getActivePluginUid = (pluginId: string): number | null => {
		const plugin = activePlugins.find((p) => p.id === pluginId);
		return plugin?.uid ?? null;
	};

	const handleAutostartChange = (pluginId: string, enabled: boolean) => {
		if (!accountID) return;

		if (enabled) {
			if (!autostart[accountID]) {
				autostart[accountID] = [];
			}
			if (!autostart[accountID].includes(pluginId)) {
				autostart[accountID].push(pluginId);
			}
		} else {
			if (autostart[accountID]) {
				const index = autostart[accountID].indexOf(pluginId);
				if (index !== -1) {
					autostart[accountID].splice(index, 1);
					if (autostart[accountID].length === 0) {
						delete autostart[accountID];
					}
				}
			}
		}

		// Sync back and save
		bolt.autostart = autostart;
		autostart = autostart; // trigger Svelte reactivity
		savePluginConfig(false);
	};
</script>

<div class="flex-1 mx-auto w-full max-w-2xl overflow-y-auto">
	{#if Object.keys(pluginList).length === 0}
		<p class="p-4 text-center text-slate-600 dark:text-slate-400">No plugins installed</p>
	{:else}
		{#each Object.entries(pluginList).sort((a, b) => {
			const nameA = a[1].name ?? '(unnamed)';
			const nameB = b[1].name ?? '(unnamed)';
			return nameA.localeCompare(nameB);
		}) as [id, meta]}
			<PluginEntry
				pluginId={id}
				pluginMeta={meta}
				{clientId}
				clientIdentity={accountID}
				isAutostart={isPluginAutostart(id)}
				activePluginUid={getActivePluginUid(id)}
				{startPlugin}
				{stopPlugin}
				onAutostartChange={handleAutostartChange}
			/>
		{/each}
	{/if}
</div>
