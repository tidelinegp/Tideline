<script lang="ts">
	import Modal from '$lib/Components/CommonUI/Modal.svelte';
	import type { Session } from '$lib/Util/Interfaces';
	import { logger } from '$lib/Util/Logger';

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
					const params = new URLSearchParams({ session_id: session.sessionId });
					fetch('/logout?'.concat(params.toString())).then((x) => {
						if (x.ok) {
							logger.info(`logged out of ${session.displayName}#${session.suffix}`);
						} else {
							x.text().then((text) => (logger.error(`error logging out of ${session.displayName}#${session.suffix}: ${x.status}: ${text}`)));
						}
					})
					modal.close();
				}}>Yes</button
			>
		</div>
	</div>
</Modal>
