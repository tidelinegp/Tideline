<script lang="ts">
	import AccountDropdownContent from '$lib/Components/AccountDropdownContent.svelte';
	import Dropdown from '$lib/Components/CommonUI/Dropdown.svelte';
	import SettingsModal from './Modals/SettingsModal.svelte';
	import { GlobalState } from '$lib/State/GlobalState';
	import { Game } from '$lib/Util/Interfaces';

	const { config, sessions, initialized } = GlobalState;
	const subs = $derived(Object.keys($sessions));
	const selectedSub = $derived($config.selected.user_id);
	const selectedSession = $derived(selectedSub ? $sessions[selectedSub] : null);
	let settingsModal: SettingsModal;
</script>

<SettingsModal bind:this={settingsModal}></SettingsModal>

<div
	class="fixed top-0 flex h-16 w-screen border-b-2 border-slate-300 bg-slate-100 duration-200 dark:border-slate-800 dark:bg-slate-900"
>
	<div class="m-3 ml-9 font-bold">
		<button
			class="mx-1 w-20 rounded-lg border-2 border-blue-500 p-1 duration-200 hover:opacity-75"
			class:text-black={$config.selected.game === Game.rs3}
			class:bg-blue-500={$config.selected.game === Game.rs3}
			onclick={() => {
				$config.selected.game = Game.rs3;
			}}
		>
			RS3
		</button>
		<button
			class="mx-1 w-20 rounded-lg border-2 border-blue-500 bg-blue-500 p-1 text-black duration-200 hover:opacity-75"
			class:text-black={$config.selected.game === Game.osrs}
			class:bg-blue-500={$config.selected.game === Game.osrs}
			onclick={() => {
				$config.selected.game = Game.osrs;
			}}
		>
			OSRS
		</button>
	</div>
	<div class="m-2 ml-auto flex gap-2">
		<button
			class="h-10 w-10 rounded-full bg-blue-500 text-center duration-200 hover:rotate-45 hover:opacity-75"
			onclick={() => ($config.use_dark_theme = !$config.use_dark_theme)}
			disabled={!$initialized}
		>
			<img src="svgs/lightbulb-solid.svg" class="m-auto h-6 w-6" alt="Change Theme" />
		</button>
		<button
			class="h-10 w-10 rounded-full bg-blue-500 text-center duration-200 hover:rotate-45 hover:opacity-75"
			onclick={() => settingsModal.open()}
			disabled={!$initialized}
		>
			<img src="svgs/gear-solid.svg" class="m-auto h-6 w-6" alt="Settings" />
		</button>
		{#if subs.length > 0}
			<Dropdown align="center">
				<button
					class="h-11 w-48 rounded-lg border-2 border-slate-300 bg-inherit text-center font-bold text-black duration-200 hover:opacity-75 dark:border-slate-800 dark:text-slate-50"
					disabled={!$initialized}
				>
					{selectedSession?.displayName ?? 'No user selected'}
				</button>

				{#snippet content()}
					<div class="w-40">
						<AccountDropdownContent />
					</div>
				{/snippet}
			</Dropdown>
		{:else}
			<button
				class="h-11 w-48 rounded-lg border-2 border-slate-300 bg-inherit p-2 text-center font-bold text-black duration-200 hover:opacity-75 dark:border-slate-800 dark:text-slate-50"
				onclick={() => fetch('/login')}
				disabled={!$initialized}
			>
				{#if $initialized}
					{'Login'}
				{/if}
			</button>
		{/if}
	</div>
</div>
