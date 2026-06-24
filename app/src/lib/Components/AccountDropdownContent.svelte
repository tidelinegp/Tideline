<script lang="ts">
	import { GlobalState } from '$lib/State/GlobalState';
	const { sessions, config } = GlobalState;
</script>

<select
	class="w-full cursor-pointer rounded-lg border-2 border-inherit bg-inherit p-2 text-center"
	bind:value={$config.selected.user_id}
>
	{#each Object.entries($sessions) as [sub, session]}
		<option value={sub} class="dark:bg-slate-900">{session.displayName}</option>
	{/each}
</select>

<div class="mt-5 flex">
	<button
		class="mx-auto mr-2 rounded-lg bg-blue-500 p-2 font-bold text-black duration-200 hover:opacity-75"
		onclick={() => fetch('/login')}
	>
		Log In
	</button>
	<button
		class="mx-auto rounded-lg border-2 border-blue-500 p-2 font-bold duration-200 hover:opacity-75"
		disabled={!$config.selected.user_id}
		onclick={async () => {
			const userId = $config.selected.user_id;
			if (!userId) return;
			fetch('/logout', { method: 'POST', body: userId });
		}}
	>
		Log Out
	</button>
</div>
