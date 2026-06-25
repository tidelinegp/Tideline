<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';
	const { sessions, config } = GlobalState;
	const { logoutConfirm = $bindable() } = $props();
</script>

<div class="flex">
	<button
		class="mx-auto mr-2 rounded-lg bg-blue-500 p-2 font-bold text-black duration-200 hover:opacity-75"
		onclick={() => fetch('/login')}
	>
		Log In
	</button>
	<button
		class="mx-auto rounded-lg border-2 border-blue-500 p-2 font-bold duration-200 hover:opacity-75"
		disabled={!$config.selected.user_id}
		onclick={() => {
			const sub = $config.selected.user_id;
			if (!sub) return;
			const session = $sessions[sub];
			if (!session) return;
			logoutConfirm(sub, session);
		}}
	>
		Log Out
	</button>
</div>

<select
	class="mt-3 w-full cursor-pointer rounded-lg border-2 border-inherit bg-inherit p-2 text-center"
	bind:value={$config.selected.user_id}
>
	{#each Object.entries($sessions) as [sub, session]}
		<option value={sub} class="dark:bg-slate-900">{session.displayName}</option>
	{/each}
</select>
