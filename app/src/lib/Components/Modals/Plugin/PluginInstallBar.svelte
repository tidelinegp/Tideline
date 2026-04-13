<script lang="ts">
	import { type PluginMeta } from '$lib/Util/Interfaces';

	interface Props {
		hasLibArchive: boolean;
		pluginList: { [key: string]: PluginMeta };
		onInstallFromURL: (url: string) => Promise<void>;
		jsonFilePicker: (cb: () => void) => void;
		setMessageInfo: (msg: string) => void;
		setMessageError: (msg: string) => void;
		updatePluginById: (id: string, meta: PluginMeta) => Promise<boolean>;
		onUpdate: () => void;
	}

	let {
		hasLibArchive,
		pluginList,
		onInstallFromURL,
		jsonFilePicker,
		setMessageInfo,
		setMessageError,
		updatePluginById,
		onUpdate,
	}: Props = $props();

	let showURLEntry: boolean = $state(false);
	let textURLEntry: string = $state('');
	let disableButtons: boolean = $state(false);

	const handleInstallFromURL = async () => {
		if (!textURLEntry) return;
		disableButtons = true;
		try {
			await onInstallFromURL(textURLEntry);
			showURLEntry = false;
			textURLEntry = '';
		} finally {
			disableButtons = false;
		}
	};

	const updateAllPlugins = () => {
		disableButtons = true;
		const promises = Object.entries(pluginList).map(([id, meta]) => updatePluginById(id, meta));
		Promise.all(promises).then((x) => {
			const updatedCount = x.filter(b => b).length;
			if (updatedCount > 0) {
				setMessageInfo(`Updated ${updatedCount} plugin(s)`);
			} else {
				setMessageInfo("All plugins are up-to-date");
			}
		}).catch((e) => setMessageError(`Error updating plugins: ${e}`)).finally(() => {
			disableButtons = false;
			onUpdate();
		});
	};

	const onInstallFromFolder = () => {
		disableButtons = true;
		jsonFilePicker(() => (disableButtons = false));
	};
</script>

<div class="border-b border-slate-300 pb-4 dark:border-slate-800">
	<h3 class="mb-3 text-lg font-semibold">Install Plugin</h3>
	<div class="flex justify-center gap-4">
		<button
			class="flex flex-col items-center gap-2 rounded-lg bg-blue-500 px-6 py-3 duration-200 enabled:hover:opacity-75 disabled:bg-gray-500"
			onclick={() => {
				showURLEntry = !showURLEntry;
				textURLEntry = '';
			}}
			disabled={!hasLibArchive || disableButtons}
			title="Install plugin from updater URL"
		>
			<img src="svgs/download-solid.svg" alt="" class="h-6 w-6" />
			<span class="text-sm font-medium text-black">From URL</span>
		</button>
		<button
			class="flex flex-col items-center gap-2 rounded-lg bg-blue-500 px-6 py-3 duration-200 enabled:hover:opacity-75 disabled:bg-gray-500"
			onclick={onInstallFromFolder}
			disabled={disableButtons}
			title="Install plugin from local directory"
		>
			<img src="svgs/folder-solid.svg" alt="" class="h-6 w-6" />
			<span class="text-sm font-medium text-black">From Folder</span>
		</button>
		<button
			class="flex flex-col items-center gap-2 rounded-lg bg-emerald-500 px-6 py-3 duration-200 enabled:hover:opacity-75 disabled:bg-gray-500"
			onclick={updateAllPlugins}
			disabled={disableButtons}
			title="Update all plugins with updater URLs"
		>
			<img src="svgs/arrows-rotate-solid.svg" alt="" class="h-6 w-6" />
			<span class="text-sm font-medium text-black">Update All</span>
		</button>
	</div>

	<!-- URL Entry -->
	{#if showURLEntry}
		<div class="mt-4">
			<label for="plugin-updater-url-input">URL:</label>
			<textarea
				rows="1"
				id="plugin-updater-url-input"
				class="w-[50%] max-w-[60%] resize-x rounded border-2 border-slate-300 bg-slate-100 text-slate-950 dark:border-slate-800"
				bind:value={textURLEntry}
			></textarea>
			<button
				title="Confirm"
				onclick={handleInstallFromURL}
				class="select-none rounded-sm bg-emerald-500 hover:opacity-75"
				><img src="svgs/check-solid.svg" class="h-5 w-5" alt="Confirm" /></button
			>
			<button
				title="Close URL entry"
				onclick={() => {
					showURLEntry = false;
					textURLEntry = '';
				}}
				class="select-none rounded-sm bg-rose-500 hover:opacity-75"
				><img src="svgs/xmark-solid.svg" class="h-5 w-5" alt="Close" /></button
			>
		</div>
	{/if}
</div>
