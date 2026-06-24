<script lang="ts">
	import Modal from '$lib/Components/CommonUI/Modal.svelte';
	import PluginList from './Plugin/PluginList.svelte';
	import PluginManagementList from './Plugin/PluginManagementList.svelte';
	import PluginManagementDetail from './Plugin/PluginManagementDetail.svelte';
	import PluginInstallBar from './Plugin/PluginInstallBar.svelte';
	import { bolt } from '$lib/State/Bolt';
	import { requestNewClientListPromise } from '$lib/Util/Functions';
	import { savePluginConfig } from '$lib/Util/Config';
	import type { PluginConfig, PluginMeta, PluginUpdaterConfig } from '$lib/Util/Interfaces';
	import { clientList } from '$lib/Util/Store';
	import { GlobalState } from '$lib/State/GlobalState';

	const { config, sessions } = GlobalState;

	let modal: Modal;

	let messageText: string | null = $state(null);
	let messageIsError: boolean = $state(false);
	let pluginList: { [key: string]: PluginMeta } = $state(bolt.pluginConfig);

	const platformFileSep: string = bolt.platform === 'windows' ? '\\' : '/';
	const configFileName: string = 'bolt.json';
	const sepConfigFileName: string = platformFileSep.concat(configFileName);
	const defaultMainLuaFilename = 'main.lua';

	const sessionList = $derived(Object.values($sessions));
	const getCharacterNameById = (id?: string): string => {
		if (!id) return unnamedClientName;
		for (const session of sessionList) {
			for (const [characterId, character] of Object.entries(session.characters)) {
				if (character.displayName && characterId === id) return character.displayName;
			}
		}
		return unnamedClientName;
	};

	export function open() {
		modal.open();
	}

	const saveConfig = (check: boolean = true) => {
		pluginList = pluginList;
		bolt.pluginConfig = pluginList;
		savePluginConfig(check);
	};

	const close = () => saveConfig();

	// sets the info message to some white text
	const setMessageInfo = (msg: string) => {
		console.log(msg);
		messageText = msg;
		messageIsError = false;
	};

	// sets the info message to some red error text
	const setMessageError = (msg: string) => {
		console.error(msg);
		messageText = msg;
		messageIsError = true;
	};

	// creates a PluginConfig from a path to a bolt.json manifest,
	// usually an absolute path created by a filepicker
	const getPluginConfigPromiseByPath = async (dirpath: string): Promise<PluginConfig | null> => {
		const path: string = dirpath.concat(
			dirpath.endsWith(platformFileSep) ? configFileName : sepConfigFileName
		);
		const x = await fetch('/read-json-file?'.concat(new URLSearchParams({ path }).toString()));
		if (!x.ok) {
			setMessageError(`couldn't get plugin config: ${x.status}: ${x.statusText}`);
			return null;
		}
		const text = await x.json();
		return <PluginConfig>text;
	};

	// creates a PluginConfig from the ID of an installed plugin, assuming it was
	// installed from the URL and therefore is in a bolt-managed directory.
	// if it was installed from a user-managed location (i.e. PluginMeta.path exists),
	// this function will return null, so use getPluginConfigPromiseFromID instead.
	const getPluginConfigPromiseFromDataDir = async (id: string): Promise<PluginConfig | null> => {
		const x = await fetch('/get-plugindir-json?'.concat(new URLSearchParams({ id }).toString()));
		if (!x.ok) {
			setMessageError(`Couldn't load plugin config: ${x.status} ${x.statusText}`);
			return null;
		}
		return x.json();
	};

	// creates a PluginConfig from the ID of an installed plugin
	const getPluginConfigPromiseByID = async (id: string | null): Promise<PluginConfig | null> => {
		if (id === null) {
			return null;
		}
		const meta = pluginList[id];
		if (!meta) return null;
		const path = meta.path;
		if (path) return getPluginConfigPromiseByPath(path);
		return getPluginConfigPromiseFromDataDir(id);
	};

	// creates a new unique plugin ID
	const getNewPluginID = () => {
		const ids = Object.keys(pluginList);
		let id;
		do {
			id = crypto.randomUUID();
		} while (ids.includes(id));
		return id;
	};

	const unnamedPluginName = '(unnamed)';
	const unnamedClientName = '(new character)';

	// tries to configure a new plugin from a file path to a bolt.json manifest
	const addPluginFromPath = async (folderPath: string) => {
		try {
			const plugin = await getPluginConfigPromiseByPath(folderPath);
			if (!plugin) return; // if this returns null, error message was already set
			selectedPlugin = getNewPluginID();
			const name = plugin.name ?? unnamedPluginName;
			pluginList[selectedPlugin] = {
				name,
				path: folderPath,
				version: plugin.version,
				originalName: name
			};
			handlePluginMetaChange();
		} catch {
			setMessageError("can't install plugin: unhandled exception");
		}
	};

	// returns true if the hash is correct, false if not
	const checkSHA256 = async (data: ArrayBuffer, hash: string): Promise<boolean> => {
		const dataHash = await crypto.subtle.digest('SHA-256', data);
		const dataHashStr = Array.from(new Uint8Array(dataHash))
			.map((x) => x.toString(16).padStart(2, '0'))
			.join('');
		return dataHashStr.localeCompare(hash, 'en', { sensitivity: 'base' }) === 0;
	};

	// tries to configure a new plugin from an updater URL
	const addPluginFromUpdaterURL = async (url: string) => {
		try {
			setMessageInfo('downloading...');
			const metaUrlResponse = await fetch(url);
			if (!metaUrlResponse) return;
			const config: PluginUpdaterConfig = await metaUrlResponse.json();

			if (!config.url) {
				setMessageError("can't install plugin: no download URL");
				return;
			}

			const configUrlResponse = await fetch(config.url);
			if (!configUrlResponse.ok) {
				setMessageError(
					`can't install plugin: remote download URL returned ${configUrlResponse.status}: ${configUrlResponse.statusText}`
				);
				return;
			}
			const data = await configUrlResponse.arrayBuffer();
			if (config.sha256 && !checkSHA256(data, config.sha256)) {
				setMessageError("can't install plugin: incorrect file hash");
				return;
			}

			const id = getNewPluginID();
			const r = await fetch('/install-plugin?'.concat(new URLSearchParams({ id }).toString()), {
				method: 'POST',
				body: data
			});
			if (!r.ok) {
				setMessageError(`can't install plugin: ${r.statusText}`);
				return;
			}
			const plugin = await getPluginConfigPromiseFromDataDir(id);
			if (!plugin) {
				setMessageError(`can't install plugin: ${configFileName} not found`);
				return;
			}

			selectedPlugin = id;
			const name = plugin.name ?? unnamedPluginName;
			pluginList[selectedPlugin] = {
				name,
				version: plugin.version,
				updaterURL: url,
				sha256: config.sha256,
				originalName: name
			};
			handlePluginMetaChange();
			setMessageInfo(`plugin '${plugin.name}' installed`);
		} catch {
			setMessageError("can't install plugin: unhandled exception");
		}
	};

	const handlePluginMetaChange = () => saveConfig(false);

	// shows the user a file picker for .json files which will attempt to add
	// a plugin using the manifest that the user selects, if any.
	// optionally, takes a callback that will be called when the file-picking
	// has finished, no matter what the outcome was.
	const jsonFilePicker = (cb: (() => void) | null | undefined) => {
		fetch('/json-file-picker')
			.then(async (x) => {
				// note this function won't be called until the user selects a file (or not),
				// so the request can take a very long time to return.
				if (!x.ok) {
					setMessageError(`file picker error: ${x.status}: ${x.statusText}`);
					return;
				}
				if (x.status !== 200) {
					// usually indicates that the user closed the file select
					return;
				}

				const text = await x.text();
				if (!text.endsWith(sepConfigFileName)) {
					setMessageError(`selection '${text}' is not named ${configFileName}; ignored`);
					return;
				}
				const subpath: string = text.substring(0, text.length - configFileName.length);
				addPluginFromPath(subpath);
			})
			.finally(cb);
	};

	const updatePluginById = async (id: string, meta: PluginMeta): Promise<boolean> => {
		if (!meta.updaterURL) return false;

		const url = meta.updaterURL;
		const x = await fetch(url);
		if (!x.ok) return false;

		let config: PluginUpdaterConfig = await x.json();
		if (!config.url) return false;

		let downloadNeeded = false;
		if (config.sha256) {
			if (!meta.sha256 || meta.sha256.localeCompare(config.sha256, 'en', { sensitivity: 'base' }))
				downloadNeeded = true;
		} else if (config.version) {
			if (meta.version !== config.version) downloadNeeded = true;
		}

		if (!downloadNeeded) return false;

		const r = await fetch(config.url);
		if (!r.ok) return false;

		const data = await r.arrayBuffer();
		if (config.sha256 && !checkSHA256(data, config.sha256)) {
			return false;
		}

		await fetch('/install-plugin?' + new URLSearchParams({ id }).toString(), {
			method: 'POST',
			body: data
		});

		const plugin = await getPluginConfigPromiseFromDataDir(id);
		if (!plugin) return false;

		if (config.sha256) meta.sha256 = config.sha256;
		if (plugin.name) meta.name = plugin.name;
		if (plugin.version) meta.version = plugin.version;

		GlobalState.pluginConfigHasPendingChanges = true;
		return true;
	};

	const stopPluginPromise = (client: number, uid: number) =>
		fetch(
			'/stop-plugin?' +
				new URLSearchParams({ client: client.toString(), uid: uid.toString() }).toString()
		);

	const stopPlugin = (client: number, uid: number) =>
		stopPluginPromise(client, uid).then((response) => {
			requestNewClientListPromise();
			if (!response.ok) {
				setMessageError(`couldn't stop plugin: ${response.status}: ${response.statusText}`);
			}
		});

	const removePluginById = async (id: string) => {
		const meta = pluginList[id];
		if (meta === null) return;

		// stop any active instances of this plugin
		let promises = [];
		for (const client of $clientList) {
			for (const plugin of client.plugins) {
				if (plugin.id === id) {
					promises.push(stopPluginPromise(client.uid, plugin.uid));
				}
			}
		}
		await Promise.all(promises).then(() => {
			requestNewClientListPromise();
		});

		// remove from autostart
		let autostart: { [characterID: string]: string[] } = {};
		for (const [character, list] of Object.entries(bolt.autostart)) {
			const newList = list.filter((x) => x !== id);
			if (newList.length > 0) autostart[character] = newList;
		}
		bolt.autostart = autostart;

		// update plugin config
		delete pluginList[id];
		selectedPlugin = null;
		saveConfig(false);

		// request deletion of config and data directories
		const delete_data = typeof meta.path !== 'string';
		const params: Record<string, string> = { id, delete_data_dir: delete_data ? '1' : '0' };
		await fetch('/uninstall-plugin?' + new URLSearchParams(params).toString()).finally(() =>
			setMessageInfo(`plugin '${meta.name}' uninstalled`)
		);
	};

	const onManagementPluginUpdate = () => saveConfig();

	// get connected client list - the UI can be re-opened while clients are already running
	requestNewClientListPromise();

	// plugin management interface - currently-selected plugin
	var selectedPlugin: string | null = $state(null);

	const openAboutPlugins = () =>
		fetch('/open-external-url', { method: 'POST', body: 'https://bolt.adamcake.com/plugins' });

	// connected clients list
	var selectedClientId: number | null = $state(0);
	$effect(() => {
		if (!$clientList.some((x) => x.uid === selectedClientId)) {
			selectedClientId = null;
		}
	});

	let selectedPluginMeta = $derived(selectedPlugin ? pluginList[selectedPlugin] : null);

	// the PluginConfig of the plugin currently selected
	let managementPluginPromise: Promise<PluginConfig | null> = $derived(
		getPluginConfigPromiseByID(selectedPlugin)
	);
	let managementPlugin: PluginConfig | null = $state(null);
	$effect(() => {
		managementPluginPromise.then((x) => {
			managementPlugin = x;
			if (!x) return;

			if (selectedPlugin === null) return;
			let meta = pluginList[selectedPlugin];
			if (!meta) return;

			// if bolt.json has been changed, update the PluginMeta and our plugin config file accordingly
			let dirty = false;
			if (x.name !== meta.originalName) {
				if (meta.name === meta.originalName) {
					meta.name = x.name;
				}
				meta.originalName = x.name;
				dirty = true;
			}
			if (x.version !== meta.version) {
				meta.version = x.version;
				dirty = true;
			}
			if (dirty) {
				saveConfig(false);
			}
		});
		managementPlugin = null;
	});

	const startPlugin = async (client: number, id: string, path: string | null) => {
		const config = await getPluginConfigPromiseByID(id);
		const main = config && config.main ? config.main : defaultMainLuaFilename;
		const params: Record<string, string> = { client: client.toString(), id, main };
		if (path) {
			const pathWithCorrectSeps = path.replaceAll('\\', '/');
			params.path = pathWithCorrectSeps.endsWith('/')
				? pathWithCorrectSeps
				: pathWithCorrectSeps + '/';
		}
		const response = await fetch('/start-plugin?' + new URLSearchParams(params).toString());
		requestNewClientListPromise();
		if (!response.ok) {
			setMessageError(`couldn't start plugin: ${response.status}: ${response.statusText}`);
		}
	};

	// Watch for new clients and autostart their plugins
	$effect(() => {
		let promises: Promise<void[]>[] = [];
		$clientList
			.filter((x) => x.is_new)
			.forEach((client) => {
				if (!client.identity) return;
				const pluginsToStart = bolt.autostart[client.identity];
				if (!pluginsToStart || pluginsToStart.length === 0) return;

				promises.push(
					Promise.all(
						pluginsToStart.map(async (pluginId) => {
							const meta = pluginList[pluginId];
							if (meta) {
								return startPlugin(client.uid, pluginId, meta.path || null);
							}
							console.warn(`Autostart plugin ${pluginId} not found in plugin list`);
						})
					)
				);
			});
		if (promises.length > 0 && $config.close_after_launch) {
			Promise.all(promises).then((_) => fetch('/close'));
		}
	});
</script>

<Modal bind:this={modal} class="h-[90%] w-[90%] text-center" onClose={close}>
	<div
		class="left-0 float-left h-full w-[min(180px,_50%)] overflow-x-hidden overflow-y-auto border-r-2 border-slate-300 pt-2 dark:border-slate-800"
	>
		<button
			class="mx-auto mb-2 w-[95%] select-none rounded-lg border-2 {selectedClientId !== null
				? 'border-blue-500 text-black dark:text-white'
				: 'border-black bg-blue-500 text-black'} p-2 font-bold hover:opacity-75"
			onclick={() => (selectedClientId = null)}
		>
			Manage Plugins
		</button>
		<hr class="p-1 dark:border-slate-700" />
		{#each $clientList as client}
			<button
				onclick={() => {
					selectedClientId = client.uid;
					messageText = null;
				}}
				class="m-1 h-[28px] w-[95%] select-none rounded-lg border-2 {selectedClientId === client.uid
					? 'border-black bg-blue-500 text-black'
					: 'border-blue-500 text-black dark:text-white'} hover:opacity-75"
			>
				{getCharacterNameById(client.identity)}
			</button>
			<br />
		{/each}
		{#if $clientList.length == 0}
			<p class="px-2 text-xs italic text-slate-500 dark:text-slate-400">
				(start an RS3 game client with plugins enabled and it will be listed here.)
			</p>
		{/if}
	</div>
	<div class="flex h-full flex-col pt-2">
		{#if bolt.hasBoltPlugins}
			<PluginInstallBar
				hasLibArchive={bolt.hasLibArchive}
				{pluginList}
				onInstallFromURL={addPluginFromUpdaterURL}
				onUpdate={onManagementPluginUpdate}
				{jsonFilePicker}
				{setMessageInfo}
				{setMessageError}
				{updatePluginById}
			/>

			{#if selectedClientId === null}
				<!-- Two-column layout for plugin management -->
				<div class="flex flex-1 overflow-hidden">
					<!-- Left: Plugin list -->
					<div class="w-64">
						<PluginManagementList
							{pluginList}
							selectedPluginId={selectedPlugin}
							onSelectPlugin={(id: string) => {
								selectedPlugin = id;
								messageText = null;
							}}
							onMetaChange={handlePluginMetaChange}
						/>
					</div>

					<!-- Right: Plugin details -->
					<div class="flex-1 overflow-y-auto">
						{#if selectedPlugin !== null && selectedPluginMeta !== null}
							<PluginManagementDetail
								pluginId={selectedPlugin}
								pluginMeta={selectedPluginMeta}
								pluginConfig={managementPlugin}
								onRemove={removePluginById}
								onUpdate={onManagementPluginUpdate}
								{setMessageInfo}
								{setMessageError}
								{updatePluginById}
							/>
						{:else if Object.keys(pluginList).length === 0}
							<div class="flex h-full items-center justify-center p-6">
								<p class="text-center">
									You have no plugins installed. Use the buttons above to install plugins.
								</p>
							</div>
						{:else}
							<div class="flex h-full items-center justify-center p-6">
								<p class="text-slate-600 dark:text-slate-400">Select a plugin to view details</p>
							</div>
						{/if}
					</div>
				</div>
			{:else}
				<!-- Client plugin list -->
				<PluginList
					clientId={selectedClientId}
					accountID={$clientList.find((c) => c.uid === selectedClientId)?.identity}
					{pluginList}
					activePlugins={$clientList.find((c) => c.uid === selectedClientId)?.plugins ?? []}
					{startPlugin}
					{stopPlugin}
				/>
			{/if}
		{/if}
		<div class="m-0 p-0 h-8 border-t border-slate-300 dark:border-slate-800 overflow-hidden">
			<div class="m-1 overflow-hidden">
				{#if messageText}
					{#if messageIsError}
						<p class="text-red-500">[error] {messageText}</p>
					{:else}
						<p>[info] {messageText}</p>
					{/if}
				{/if}
			</div>
		</div>
	</div>
	<div class="absolute bottom-2 left-2">
		<button
			class="m-0 cursor-pointer select-none border-none bg-transparent p-0 text-sm text-gray-500 underline"
			onclick={openAboutPlugins}>about plugins...</button
		>
	</div>
</Modal>
