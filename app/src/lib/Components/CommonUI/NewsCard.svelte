<script lang="ts">
	interface NewsItem {
		title: string;
		imageUrl: string;
		url: string;
		date: string;
	}

	export let newsItem: NewsItem | undefined;
	export let loading: boolean = false;
</script>

{#if loading}
	<div class="row skeleton">
		<div class="thumb"></div>
		<div class="text"><div class="bar w"></div><div class="bar n"></div></div>
	</div>
{:else if newsItem}
	<a
		class="row"
		href={newsItem.url}
		target="_blank"
		rel="noopener noreferrer"
		onclick={(evt) => {
			evt.preventDefault();
			fetch('/open-external-url', { method: 'POST', body: newsItem.url });
		}}
	>
		<div class="thumb">
			{#if newsItem.imageUrl}
				<img src={newsItem.imageUrl} alt="" />
			{/if}
		</div>
		<div class="text">
			<b>{newsItem.title}</b>
			<small>{newsItem.date}</small>
		</div>
	</a>
{/if}

<style>
	.row {
		display: grid;
		grid-template-columns: 64px minmax(0, 1fr);
		gap: 14px;
		align-items: center;
		padding: 10px 14px;
		border-top: 1px solid var(--line);
		color: var(--txt);
		text-decoration: none;
	}
	.row:first-child {
		border-top: 0;
	}
	a.row:hover {
		background: var(--hov);
	}
	.thumb {
		width: 64px;
		height: 44px;
		border-radius: 8px;
		overflow: hidden;
		background: var(--inp);
		border: 1px solid var(--inpb);
	}
	.thumb img {
		width: 100%;
		height: 100%;
		object-fit: cover;
		display: block;
	}
	.text {
		min-width: 0;
	}
	b {
		display: block;
		font: 650 14.5px var(--body);
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	small {
		color: var(--mut);
		font-size: 12.5px;
	}
	.bar {
		height: 12px;
		border-radius: 6px;
		background: var(--inp);
		margin: 5px 0;
	}
	.bar.w {
		width: 70%;
	}
	.bar.n {
		width: 30%;
	}
</style>
