<script lang="ts">
	import { bolt } from '$lib/State/Bolt';
	import { logger } from '$lib/Util/Logger';
	import { GlobalState } from '$lib/State/GlobalState';

	// opens local data on disk
	function openDataDir(): void {
		var xml = new XMLHttpRequest();
		xml.open('GET', '/browse-data');
		xml.onreadystatechange = () => {
			if (xml.readyState == 4) {
				logger.info(`Browse status: '${xml.responseText.trim()}'`);
			}
		};
		xml.send();
	}

	const { config } = GlobalState;

	const checkAnnouncementsDesc =
		"Allow checking for official announcements about game status. These will be displayed above the 'play' button. Disabling will reduce the number of web requests made when opening the launcher.";
	const closeAfterLaunchDesc = 'Close the launcher immediately after launching a game';
	const discardExpiredLoginsDesc =
		"Discard login sessions if they're no longer valid, prompting the player to sign-in again. If unchecked, the only way to discard a login will be by manually pressing the 'log out' button, even if your login no longer works.";
	const fetchRssFeedsDesc =
		'Enable fetching RSS feeds for news and updates. Disabling this will reduce the number of web requests made when opening the launcher.';
</script>

<div class="set">
	<h3>General</h3>

	<label class="opt" title={checkAnnouncementsDesc}>
		<input id="check_announcements" type="checkbox" bind:checked={$config.check_announcements} />
		<span>
			<b>Check game announcements</b>
			<small>Show official game status messages in the launcher.</small>
		</span>
	</label>
	<label class="opt" title={fetchRssFeedsDesc}>
		<input id="fetch_rss_feeds" type="checkbox" bind:checked={$config.fetch_rss_feeds} />
		<span>
			<b>Fetch news feeds</b>
			<small>Load the latest news on the home screen.</small>
		</span>
	</label>
	<label class="opt" title={closeAfterLaunchDesc}>
		<input id="close_after_launch" type="checkbox" bind:checked={$config.close_after_launch} />
		<span>
			<b>Close Bolt after launching a game</b>
			<small>Quit the launcher as soon as a client starts.</small>
		</span>
	</label>
	<label class="opt" title={discardExpiredLoginsDesc}>
		<input
			id="discard_expired_logins"
			type="checkbox"
			bind:checked={$config.discard_expired_sessions}
		/>
		<span>
			<b>Discard expired login sessions</b>
			<small>Ask you to sign in again when a saved login stops working.</small>
		</span>
	</label>

	<div class="foot">
		<span class="ver">Bolt version {bolt.boltVersion}</span>
		<button class="ghost" type="button" onclick={openDataDir}>Browse app data</button>
	</div>
</div>

<style>
	h3 {
		margin: 0 0 14px;
		font: 700 22px var(--disp);
		letter-spacing: -0.02em;
	}
	.opt {
		display: flex;
		gap: 14px;
		align-items: flex-start;
		padding: 12px 14px;
		border: 1px solid var(--line);
		border-radius: 12px;
		background: var(--s2);
		margin-bottom: 8px;
		cursor: pointer;
	}
	.opt:hover {
		border-color: var(--inpb);
	}
	.opt input {
		margin-top: 3px;
	}
	.opt b {
		display: block;
		font: 650 14.5px var(--body);
	}
	.opt small {
		color: var(--mut);
		font-size: 12.5px;
	}
	.foot {
		display: flex;
		align-items: center;
		gap: 12px;
		margin-top: 18px;
	}
	.ver {
		color: var(--mut);
		font-size: 13px;
		margin-right: auto;
	}
	.ghost {
		background: var(--inp);
		border: 1px solid var(--inpb);
		color: var(--txt);
		border-radius: 10px;
		padding: 8px 14px;
		font: 600 13px var(--body);
		cursor: pointer;
	}
	.ghost:hover {
		border-color: var(--gold);
		color: var(--gold);
	}
</style>
