import { Preferences } from '@capacitor/preferences';

async function setter(key, value) {
	return Preferences.set({
		key: key,
		value: typeof value == "string" ? value : JSON.stringify(value)
	});
}

async function default_getter(key, default_value) {
	/* A getter function that set (and returns) the default value if the key
	 * is not found */
	const result = await Preferences.get({
		key: key
	});

	if (result.value !== null) {
		// key already existing
		try {
			return JSON.parse(result.value);
		}
		catch (e) {
			return result.value;
		}
	}
	else {
		// key not found, setting a default value
		await setter(key, default_value);
		return default_value;
	}
}

/******************************************************************************
 * parameter: API PATH                                                        *
 ******************************************************************************/
export async function getAPIPath() {
	return default_getter('api_path', 'https://telescopi/api')
}

export async function setAPIPath(path) {
	return setter('api_path', path)
}

/******************************************************************************
 * parameters: GPS                                                            *
 ******************************************************************************/
export function getDefaultGPSCoords() {
	return {
		// default coords are Paris, France
		latitude: 48.864716,
		longitude : 2.349014,
		accuracy: 100,
		timestamp: Date.now(),
		elevation: 35 as number | null
	}
}

export async function getManualGPSCoords() {
	return default_getter('manual_gps_coords', getDefaultGPSCoords())
}

export async function setManualGPSCoords(gps_coords) {
	return setter('manual_gps_coords', gps_coords)
}

export async function getGPSType() {
	return default_getter('gps_coords_type', 'manual')
}

export async function setGPSType(gps_type) {
	return setter('gps_coords_type', gps_type)
}

