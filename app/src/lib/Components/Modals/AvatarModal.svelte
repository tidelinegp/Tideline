<script lang="ts">
	import Modal from '$lib/Components/CommonUI/Modal.svelte';
	import { GlobalState } from '$lib/State/GlobalState';
	import { saveConfig } from '$lib/Util/Config';

	const { config } = GlobalState;

	let modal: Modal;
	let sub = $state('');
	let name = $state('');
	let link = $state('');
	let error = $state('');
	let dragging = $state(false);

	const current = $derived($config.avatars?.[sub]);

	export function open(sub_: string, name_: string) {
		sub = sub_;
		name = name_;
		link = '';
		error = '';
		modal.open();
	}

	function setAvatar(value: string | null) {
		const next = { ...($config.avatars ?? {}) };
		if (value) {
			next[sub] = value;
		} else {
			delete next[sub];
		}
		$config.avatars = next;
		saveConfig();
	}

	// crops the picture to a square and shrinks it so the saved config stays small
	function fromFile(file: File | undefined) {
		error = '';
		if (!file) return;
		if (!file.type.startsWith('image/')) {
			error = 'That file is not an image.';
			return;
		}
		const reader = new FileReader();
		reader.onerror = () => (error = 'Could not read that file.');
		reader.onload = () => {
			const img = new Image();
			img.onerror = () => (error = 'Could not open that image.');
			img.onload = () => {
				const size = 96;
				const side = Math.min(img.width, img.height);
				const canvas = document.createElement('canvas');
				canvas.width = size;
				canvas.height = size;
				const ctx = canvas.getContext('2d');
				if (!ctx) return;
				ctx.drawImage(
					img,
					(img.width - side) / 2,
					(img.height - side) / 2,
					side,
					side,
					0,
					0,
					size,
					size
				);
				setAvatar(canvas.toDataURL('image/png'));
			};
			img.src = String(reader.result);
		};
		reader.readAsDataURL(file);
	}

	function useLink() {
		error = '';
		const url = link.trim();
		if (!/^https:\/\/\S+$/i.test(url)) {
			error = 'Enter a link that starts with https://';
			return;
		}
		setAvatar(url);
		link = '';
	}
</script>

<Modal bind:this={modal} class="avatar-modal">
	<div class="box">
		<h2>Account picture</h2>
		<p class="sub">for {name}</p>

		<div
			class="drop"
			class:over={dragging}
			role="group"
			aria-label="Drop an image here"
			ondragover={(e) => {
				e.preventDefault();
				dragging = true;
			}}
			ondragleave={() => (dragging = false)}
			ondrop={(e) => {
				e.preventDefault();
				dragging = false;
				fromFile(e.dataTransfer?.files?.[0]);
			}}
		>
			<div class="preview">
				{#if current}
					<img src={current} alt="" />
				{:else}
					<span>{(name.trim()[0] ?? '?').toUpperCase()}</span>
				{/if}
			</div>
			<div class="actions">
				<label class="btn">
					Choose image
					<input
						type="file"
						accept="image/*"
						hidden
						onchange={(e) => fromFile((e.currentTarget as HTMLInputElement).files?.[0])}
					/>
				</label>
				<span class="or">or drop an image here</span>
			</div>
		</div>

		<label class="row" for="avatar-link">Image link</label>
		<div class="link">
			<input
				id="avatar-link"
				type="text"
				placeholder="https://…"
				bind:value={link}
				onkeydown={(e) => e.key === 'Enter' && useLink()}
			/>
			<button class="btn" type="button" onclick={useLink}>Use</button>
		</div>

		{#if error}<p class="err">{error}</p>{/if}

		<div class="foot">
			<button class="btn" type="button" disabled={!current} onclick={() => setAvatar(null)}>
				Remove picture
			</button>
			<button class="btn primary" type="button" onclick={() => modal.close()}>Done</button>
		</div>
	</div>
</Modal>

<style>
	:global(dialog.avatar-modal) {
		width: min(440px, 90%);
	}
	.box {
		padding: 26px 28px 22px;
	}
	h2 {
		margin: 0;
		font: 700 20px var(--disp);
		letter-spacing: -0.02em;
	}
	.sub {
		margin: 2px 0 16px;
		color: var(--mut);
		font-size: 13px;
	}
	.drop {
		display: flex;
		align-items: center;
		gap: 16px;
		padding: 14px;
		border: 1px dashed var(--inpb);
		border-radius: 12px;
		background: var(--s2);
		margin-bottom: 16px;
	}
	.drop.over {
		border-color: var(--gold);
		background: color-mix(in srgb, var(--gold) 8%, var(--s2));
	}
	.preview {
		flex: none;
		width: 72px;
		height: 72px;
		border-radius: 18px;
		overflow: hidden;
		display: grid;
		place-items: center;
		background: var(--inp);
		border: 1px solid var(--inpb);
		font: 800 28px var(--disp);
		color: var(--gold);
	}
	.preview img {
		width: 100%;
		height: 100%;
		object-fit: cover;
	}
	.actions {
		display: grid;
		gap: 6px;
	}
	.or {
		color: var(--mut);
		font-size: 12.5px;
	}
	.row {
		display: block;
		margin-bottom: 6px;
		font: 650 12px var(--body);
		letter-spacing: 0.1em;
		text-transform: uppercase;
		color: var(--mut);
	}
	.link {
		display: flex;
		gap: 8px;
	}
	.link input {
		flex: 1;
		min-width: 0;
	}
	.btn {
		display: inline-block;
		background: var(--inp);
		border: 1px solid var(--inpb);
		color: var(--txt);
		border-radius: 10px;
		padding: 8px 14px;
		font: 600 13px var(--body);
		cursor: pointer;
		text-align: center;
	}
	.btn:hover:not(:disabled) {
		border-color: var(--gold);
		color: var(--gold);
	}
	.btn:disabled {
		opacity: 0.45;
		cursor: default;
	}
	.btn.primary {
		background: linear-gradient(90deg, var(--g2), var(--g1));
		color: var(--on-gold);
		border: 0;
	}
	.err {
		color: #f87171;
		font-size: 12.5px;
		margin: 10px 0 0;
	}
	.foot {
		display: flex;
		justify-content: space-between;
		margin-top: 20px;
	}
</style>
