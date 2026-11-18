<script lang="ts">
	// buttons for the frameless launcher window; handled by the native app
</script>

<div class="controls">
	<button type="button" aria-label="Minimize" onclick={() => fetch('/minimize')}>
		<svg width="12" height="12" viewBox="0 0 12 12"><path d="M1 6h10" stroke="currentColor" stroke-width="1.4" /></svg>
	</button>
	<button type="button" aria-label="Maximize or restore" onclick={() => fetch('/toggle-maximize')}>
		<svg width="12" height="12" viewBox="0 0 12 12"><rect x="1.5" y="1.5" width="9" height="9" rx="1.2" fill="none" stroke="currentColor" stroke-width="1.3" /></svg>
	</button>
	<button type="button" class="close" aria-label="Close" onclick={() => fetch('/close')}>
		<svg width="12" height="12" viewBox="0 0 12 12"><path d="M1.5 1.5l9 9M10.5 1.5l-9 9" stroke="currentColor" stroke-width="1.4" /></svg>
	</button>
</div>

<style>
	.controls {
		display: flex;
		-webkit-app-region: no-drag;
	}
	button {
		width: 46px;
		height: 36px;
		display: grid;
		place-items: center;
		background: none;
		border: 0;
		color: var(--mut);
		cursor: pointer;
		-webkit-app-region: no-drag;
	}
	button:hover {
		background: var(--hov);
		color: var(--txt);
	}
	.close:hover {
		background: #e5484d;
		color: #fff;
	}
</style>
