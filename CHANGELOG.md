# Changelog

All notable changes to Tideline, newest first. Tideline is a modified version of [Bolt](https://github.com/Adamcake/Bolt).

## 0.1.7
- Reorder your Jagex accounts: drag an account's header up or down the list (or press Alt+Up / Alt+Down on its name). The order is remembered.
- Flipping dashboard item tables: new "Margin x volume (24h)" column, a 3-day price trend line on every row, a Free-to-play / Members filter, a Columns menu to show or hide columns (including "Buy last seen" and "Sell last seen"), and Ctrl+K to jump to the item search.
- Settings, About now carries the Jagex Fan Content Policy credit wording.

## 0.1.6
- The first-run notice now says Tideline (a modified version of Bolt) and that it is not affiliated with Jagex, instead of calling itself Bolt.

## 0.1.5
- RuneLite now looks sharp on scaled displays (125%, 150% and so on). Tideline sets Windows' high-DPI compatibility option on its own Java copy before starting RuneLite. It is a per-user setting and needs no administrator rights.
- Cancelling the flipping dashboard download now starts over from 0% instead of resuming. Downloads no longer go through the Windows download cache.

## 0.1.4
- The dashboard download is fetched directly instead of through a Windows helper, so Cancel can stop it cleanly.

## 0.1.3
- Cancel on the dashboard download is more reliable: repeated clicks and progress checks are never answered from a saved reply, and a Cancel that arrives while the file is being checked still counts.

## 0.1.2
- "Download flipping dashboard" button for launcher-only installs, with a progress bar, percentage and a Cancel button.
- Setting to hide that button: Settings, Integrations, "Offer to download the flipping dashboard".
- The installer now ends Tideline's own processes before updating, so in-app updates no longer abort with "unable to close applications".

## 0.1.1
- Tideline uses its bundled Java first, so another Java (or a `JAVA_HOME` setting) on the PC can no longer break RuneLite.
- "Manage characters on jagex.com" link under the account list.

## 0.1.0
- First release.
- Multiple Jagex accounts and characters in one launcher, with RuneLite, HDOS and the official clients.
- Optional flipping dashboard (prices, flip finder and profit tracking), chosen during install.
- Signed automatic updates: the launcher only installs updates signed with Tideline's key.
- Colour themes, including a "Match Windows" option.
