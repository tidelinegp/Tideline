<script lang="ts">
	import Modal from '$lib/Components/CommonUI/Modal.svelte';
	import { onMount } from 'svelte';
	import { bolt } from '$lib/State/Bolt';

	let modal: Modal;

	onMount(() => {
		modal.open();
	});
</script>

<Modal
	bind:this={modal}
	canSelfClose={false}
	class="w-3/5 text-center border-2 border-solid border-black px-8 py-2"
>
	{#each bolt.env.disclaimer as text}
		<p class="p-2">
			{text}
		</p>
	{/each}
	<button
		class="m-5 rounded-lg border-2 border-blue-500 p-2 select-none duration-200 hover:opacity-75"
		onclick={modal.close}>I Understand</button
	>
</Modal>
