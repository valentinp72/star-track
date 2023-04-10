import { Preferences } from '@capacitor/preferences';

export async function getAPIPath() {
	const result = await Preferences.get({ key: 'api_path' });
	let api_path = 'https://telescopi/api';

	if (result.value !== null) {
		api_path = result.value;
	}
	else {
		await Preferences.set({ key: 'api_path', value: api_path });
	}
	return api_path;
}

export async function setAPIPath(path) {
	await Preferences.set({ key: 'api_path', value: path });
}
