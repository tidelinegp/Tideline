<script lang="ts">
	import Modal from '$lib/Components/CommonUI/Modal.svelte';
	import type { Session } from '$lib/Util/Interfaces';

	let modal: Modal;
	let sub: string;
	let session: Session;
	export function open(sub_: string, session_: Session) {
		sub = sub_;
		session = session_;
		modal.open();
	}
</script>

<Modal bind:this={modal} class="h-fit w-3/5 px-5 py-3 text-center border-2 border-solid border-black">
	<div>
		<h2 class="text-3xl font-extrabold">End Session</h2>
		<br />
		<p>Log out of {session.displayName}#{session.suffix}?</p>
		<div class="align-bottom">
			<button
				class="mx-1 mb-2 mt-5 w-[45%] rounded-lg bg-blue-500 p-2 font-bold text-black duration-200 hover:opacity-75"
				onclick={() => {
					modal.close();
				}}>No</button
			>
			<button
				class="mx-1 mb-2 mt-5 w-[45%] rounded-lg bg-emerald-500 p-2 font-bold text-black duration-200 hover:opacity-75"
				onclick={() => {
					const params = new URLSearchParams({ sub });
					fetch('/logout?'.concat(params.toString()));
					modal.close();
				}}>Yes</button
			>
		</div>
	</div>
</Modal>
