<script lang="ts">
	import { onMount } from 'svelte';
	import NewsCard from './CommonUI/NewsCard.svelte';
	import { GlobalState } from '$lib/State/GlobalState';
	import { Game } from '$lib/Util/Interfaces';

	let { osrsPsa = $bindable(), rs3Psa = $bindable() } = $props();

	const RSS_URL = 'aHR0cHM6Ly9zZWN1cmUucnVuZXNjYXBlLmNvbS9tPW5ld3MvbGF0ZXN0X25ld3MucnNz';
	const DECODED_RSS = atob(RSS_URL);
	const { config } = GlobalState;

	let newsItems: Array<{ title: string; imageUrl: string; url: string; date: string }> = $state([]);
	let loading: boolean = $state(true);

	const CACHE_KEY_OSRS = 'bolt:news:feed-osrs';
	const CACHE_TS_KEY_OSRS = 'bolt:news:feed:ts-osrs';
	const CACHE_KEY_RS3 = 'bolt:news:feed-rs3';
	const CACHE_TS_KEY_RS3 = 'bolt:news:feed:ts-rs3';
	const CACHE_TTL = 15 * 60 * 1000; // 15 minutes (it was 6 hours, so new posts took far too long to show up)

	const parseAndSet = (xmlString: string) => {
		const parser = new DOMParser();
		const xmlDoc = parser.parseFromString(xmlString, 'application/xml');
		const items = xmlDoc.getElementsByTagName('item');
		const parsed: Array<{ title: string; imageUrl: string; url: string; date: string }> = [];
		for (let i = 0; i < items.length; i++) {
			const item = items[i];
			const title = item.getElementsByTagName('title')[0]?.textContent || 'No Title';
			const url = item.getElementsByTagName('link')[0]?.textContent || '#';
			const rawDate = item.getElementsByTagName('pubDate')[0]?.textContent || '';
			let date = rawDate;
			// Format date to show only the date portion (e.g. "Tue, 16 Dec 2025")
			const dateObj = new Date(rawDate);
			if (!isNaN(dateObj.getTime())) {
				const parts = dateObj.toUTCString().split(' ');
				date = parts.slice(0, 4).join(' ');
			}
			const enclosure = item.getElementsByTagName('enclosure')[0];
			const imageUrl = enclosure ? enclosure.getAttribute('url') || '' : '';
			parsed.push({ title, imageUrl, url, date });
		}
		parsed.sort((a, b) => new Date(b.date).getTime() - new Date(a.date).getTime());
		newsItems = parsed;
	};

	export const fetchNews = async (force = false) => {
		loading = true;
		const IS_OSRS = $config.selected.game === Game.osrs;
		const CACHE_KEY = IS_OSRS ? CACHE_KEY_OSRS : CACHE_KEY_RS3;
		const CACHE_TS_KEY = IS_OSRS ? CACHE_TS_KEY_OSRS : CACHE_TS_KEY_RS3;

		try {
			const now = Date.now();
			const rawCached = localStorage.getItem(CACHE_KEY);
			const cachedTs = parseInt(localStorage.getItem(CACHE_TS_KEY) || '0', 10);
			// Use cached if fresh
			if (!force && rawCached && now - cachedTs < CACHE_TTL) {
				parseAndSet(rawCached);
				loading = false;
				return;
			}

			// Otherwise fetch and cache
			let fetchUrl = DECODED_RSS;
			if (IS_OSRS) {
				fetchUrl += '?oldschool=1';
			}
			const response = await fetch(fetchUrl);
			const rssText = await response.text();

			// persist cache
			try {
				localStorage.setItem(CACHE_KEY, rssText);
				localStorage.setItem(CACHE_TS_KEY, String(now));
			} catch (e) {
				// localStorage may be full or unavailable; ignore caching errors
				console.warn('Unable to cache RSS feed:', e);
			}

			parseAndSet(rssText);
		} catch (error) {
			console.error('Error fetching or parsing RSS feed:', error);
			// fallback to cached data if available
			const rawCached = localStorage.getItem($config.selected.game === Game.osrs ? CACHE_KEY_OSRS : CACHE_KEY_RS3);
			if (rawCached) {
				parseAndSet(rawCached);
			}
		}

		loading = false;
	};

	let psaClass =
		'sticky mb-5 w-full animate-pulse rounded border border-amber-500 bg-slate-100 p-2 shadow-md duration-200 dark:bg-slate-900';

	onMount(async () => {
		await fetchNews();
		config.subscribe(() => {
			if ($config.fetch_rss_feeds) {
				fetchNews();
			}
		});
	});
</script>

<div class="min-h-0">
	<div class="sec-h">
		<h3>News</h3>
		<button type="button" class="refresh" title="Check for new posts" aria-label="Refresh news" onclick={() => fetchNews(true)}>
			<svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round"><path d="M20 11a8 8 0 1 0-2.3 5.7" /><path d="M20 4v7h-7" /></svg>
		</button>
	</div>
	<div>
		{#if loading}
			{#each Array(6)}
				<NewsCard loading={true} newsItem={undefined} />
			{/each}
		{:else}
			{#each newsItems as newsItem (newsItem.url)}
				<NewsCard {newsItem} />
			{/each}
		{/if}
	</div>
</div>

<style>
	.sec-h {
		padding: 14px 14px 8px;
		border-bottom: 1px solid var(--line);
		display: flex;
		align-items: center;
		justify-content: space-between;
	}
	.refresh {
		display: grid;
		place-items: center;
		width: 26px;
		height: 26px;
		border-radius: 8px;
		background: none;
		border: 0;
		color: var(--mut);
		cursor: pointer;
	}
	.refresh:hover {
		background: var(--hov);
		color: var(--txt);
	}
	.sec-h h3 {
		margin: 0;
		font: 650 17px var(--disp);
		letter-spacing: -0.01em;
	}
</style>
