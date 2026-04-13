<script lang="ts">
	import type { PluginMeta, PluginConfig } from '$lib/Util/Interfaces';
	import { GlobalState } from '$lib/State/GlobalState';

	interface Props {
		pluginId: string;
		pluginMeta: PluginMeta;
		pluginConfig: PluginConfig | null;
		onRemove: (id: string) => void;
		onUpdate: () => void;
		setMessageInfo: (msg: string) => void;
		setMessageError: (msg: string) => void;
		updatePluginById: (id: string, meta: PluginMeta) => Promise<boolean>;
	}

	let {
		pluginId,
		pluginMeta,
		pluginConfig,
		onRemove,
		onUpdate,
		setMessageInfo,
		setMessageError,
		updatePluginById,
	}: Props = $props();

	const browseData = () => {
		if (pluginMeta.path) {
			fetch('/browse-directory?' + new URLSearchParams({ path: pluginMeta.path }).toString());
		} else {
			fetch('/browse-plugin-data?' + new URLSearchParams({ id: pluginId }).toString());
		}
	};

	const browseConfig = () => {
		fetch('/browse-plugin-config?' + new URLSearchParams({ id: pluginId }).toString());
	};

	const updatePlugin = () => {
		updatePluginById(pluginId, pluginMeta).then(didUpdate => {
			if (didUpdate) {
				setMessageInfo(`Plugin "${pluginMeta.name}" updated`);
			} else {
				setMessageInfo("Plugin already up-to-date");
			}
		}).catch(e => setMessageError(`Error updating: ${e}`)).finally(onUpdate);
	};

	const removePlugin = async () => {
		await fetch(
			'/uninstall-plugin?' +
				new URLSearchParams({
					id: pluginId,
					delete_data_dir: typeof pluginMeta.path === 'string' ? '0' : '1'
				}).toString()
		);
		GlobalState.pluginConfigHasPendingChanges = true;
		setMessageInfo(`plugin '${pluginMeta.name}' uninstalled`);
		onRemove(pluginId);
	};
</script>

{#if pluginConfig}
	<div class="flex h-full flex-col items-center justify-center px-2 py-5 text-center">
		<h2 class="pb-2 text-2xl font-bold">
			{pluginConfig.name ?? '(unnamed)'}
		</h2>
		{#if pluginConfig.version}
			<p class="pb-4 text-lg italic text-slate-600 dark:text-slate-400">
				v{pluginConfig.version}
			</p>
		{/if}
		<p class="pb-6 {pluginConfig.description ? '' : 'italic text-slate-500'}">
			{pluginConfig.description ?? 'no description'}
		</p>

		<div class="w-full max-w-96 flex flex-col items-center justify-center gap-2">
			<div class="w-full flex justify-around">
				<button
					class="w-full rounded-lg bg-blue-500 px-4 py-1 mx-1 font-bold text-black duration-200 hover:opacity-75"
					onclick={browseData}
				>
					Data
				</button>

				<button
					class="w-full rounded-lg bg-blue-500 px-4 py-1 mx-1 font-bold text-black duration-200 hover:opacity-75"
					onclick={browseConfig}
				>
					Config
				</button>
			</div>
			<div class="w-full flex justify-around">
				{#if pluginMeta.updaterURL}
					<button
						class="w-full rounded-lg bg-blue-500 px-4 py-1 mx-1 font-bold text-black duration-200 hover:opacity-75"
						onclick={updatePlugin}
					>
						Update
					</button>
				{/if}
				<button
					class="w-full rounded-lg bg-rose-500 px-4 py-1 mx-1 font-bold text-black duration-200 hover:opacity-75"
					onclick={removePlugin}
				>
					Remove
				</button>
			</div>
		</div>
	</div>
{:else}
	<div class="flex h-full items-center justify-center p-6">
		<div>
			<p class="text-slate-600 dark:text-slate-400">Loading plugin details...</p>
			<button
				class="w-full rounded-lg bg-rose-500 px-4 py-1 font-bold text-black duration-200 hover:opacity-75"
				onclick={removePlugin}
			>
				Remove
			</button>
		</div>
	</div>
{/if}
