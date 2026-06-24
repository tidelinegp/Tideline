import type { BoltEnv } from '$lib/State/Bolt';

export class ParseUtils {
	// The bolt object from the C++ app has its values base64 encoded.
	// This will decode each value, and return the original structure
	static decodeBolt(encoded: BoltEnv): BoltEnv {
		const decodedObject: Record<string, string | string[]> = {};
		for (const _key in encoded) {
			const key = _key as keyof BoltEnv;
			const value = encoded[key];
			if (typeof value === 'string') {
				decodedObject[key] = atob(value);
			} else if (Array.isArray(value) && value.every((item) => typeof item === 'string')) {
				decodedObject[key] = value.map((item) => atob(item));
			} else {
				decodedObject[key] = value;
			}
		}

		return decodedObject as unknown as BoltEnv;
	}
}
