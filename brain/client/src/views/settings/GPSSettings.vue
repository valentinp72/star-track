<template>
	<ion-page>
		<ion-header>
			<ion-toolbar>
				<ion-buttons slot="start">
					<ion-back-button></ion-back-button>
				</ion-buttons>
				<ion-title>GPS location</ion-title>
			</ion-toolbar>
		</ion-header>
		<ion-content>
			<ion-list>
				<ion-list-header>
					<ion-label>Current coordinates</ion-label>
				</ion-list-header>
				<ion-item>
					<ion-label>Using: {{ using_gps ? 'GPS coordinates' : 'Manual coordinates' }}</ion-label>
				</ion-item>
				<ion-item>
					<ion-label>
						Current position:
						<p>Latitude: {{ global_coords.latitude }}</p>
						<p>Longitude: {{ global_coords.longitude }}</p>
						<p>Elevation: {{ global_coords.elevation == null ? 'Unknown' : global_coords.elevation + 'm' }}</p>
						<p>Accuracy: {{ global_coords.accuracy}}m</p>
						<p>Obtained: {{ pretty_timestamp(global_coords.timestamp) }}</p>
					</ion-label>
				</ion-item>
				<ion-button @click="send_telescope()" expand="block" color="medium">Send to the telescope</ion-button>
			</ion-list>

			<ion-list>
				<ion-list-header>
					<ion-label>GPS coordinates</ion-label>
				</ion-list-header>
				<ion-item>
					<ion-toggle :checked="using_gps" @ionChange="toggle_GPS($event.target.checked)">Enable GPS</ion-toggle>
				</ion-item>
				<ion-item>
					<ion-label>
						Permissions: {{ is_gps_allowed }}
					</ion-label>
				</ion-item>
			</ion-list>

			<ion-list>
				<ion-list-header>
					<ion-label>Manual coordinates</ion-label>
				</ion-list-header>
				<ion-item>
					<ion-input
						label="Latitude"
						label-placement="stacked"
						:value="manual_coords.latitude"
						:disabled='using_gps'
						@ionChange="validate_manual_coords($event.target.value, 'latitude')"
						@ionBlur="validate_manual_coords($event.target.value, 'latitude')"
					>
					</ion-input>
					<ion-badge slot="end" color="danger" v-if="!is_valid_manual_coords['latitude']">Invalid</ion-badge>
				</ion-item>
				<ion-item>
					<ion-input
						label="Longitude"
						label-placement="stacked"
						:value="manual_coords.longitude"
						:disabled='using_gps'
						@ionChange="validate_manual_coords($event.target.value, 'longitude')"
						@ionBlur="validate_manual_coords($event.target.value, 'longitude')"
					>
					</ion-input>
					<ion-badge slot="end" color="danger" v-if="!is_valid_manual_coords['longitude']">Invalid</ion-badge>
				</ion-item>
				<ion-item>
					<ion-input
						label="Elevation"
						label-placement="stacked"
						:value="manual_coords.elevation"
						:disabled='using_gps'
						@ionChange="validate_manual_coords($event.target.value, 'elevation')"
						@ionBlur="validate_manual_coords($event.target.value, 'elevation')"
					>
					</ion-input>
					<ion-badge slot="end" color="danger" v-if="!is_valid_manual_coords['elevation']">Invalid</ion-badge>
				</ion-item>
				<ion-item>
					<ion-note>
						Configured at: {{ pretty_timestamp(manual_coords.timestamp) }}
					</ion-note>
				</ion-item>
			</ion-list>
		</ion-content>
	</ion-page>
</template>

<script lang="ts">
import { IonContent, IonHeader, IonPage, IonTitle, IonToolbar, IonBackButton, IonButtons, IonList, IonItem, IonLabel, IonListHeader, IonInput, IonBadge, IonToggle, IonNote } from '@ionic/vue';
import { defineComponent } from 'vue';
import { getDefaultGPSCoords, getManualGPSCoords, setManualGPSCoords, getGPSType, setGPSType, getAPIPath } from '@/plugins/config';
import { Geolocation } from '@capacitor/geolocation';
import axios from 'axios';

export default defineComponent({
	name: 'ObjectDetail',
	components: {
		IonContent,
		IonHeader,
		IonPage,
		IonTitle,
		IonToolbar,
		IonBackButton,
		IonButtons,
		IonList, IonItem, IonLabel, IonListHeader, IonNote,
		IonInput,
		IonBadge,
		IonToggle
	},
	data() {
		return {
			using_gps: false,
			is_gps_allowed: 'unknown',
			gps_coords: getDefaultGPSCoords(),
			manual_coords: getDefaultGPSCoords(),
			global_coords: getDefaultGPSCoords(),
			is_valid_manual_coords: {
				latitude: true,
				longitude: true,
				elevation: true,
			}
		}
	},
	async beforeMount() {
		this.using_gps = await getGPSType() == 'gps';
		this.is_gps_allowed = await Geolocation.checkPermissions().then(perm => {
			return perm.location
		});
		this.manual_coords = await getManualGPSCoords();
		this.gps_coords = await this.get_from_GPS();
	},
	methods: {
		pretty_timestamp(timestamp) {
			const date = new Date(timestamp);
			return date.toLocaleDateString() + ' at ' + date.toLocaleTimeString();
		},
		async get_from_GPS() {
			const response = await Geolocation.getCurrentPosition();
			if (response) {
				return {
					latitude: response.coords.latitude,
					longitude: response.coords.longitude,
					elevation: response.coords.altitude,
					accuracy: response.coords.accuracy,
					timestamp: response.timestamp,
				}
			}
			else {
				return getDefaultGPSCoords();
			}
		},
		async toggle_GPS(toggled) {
			this.using_gps = toggled;
			if (toggled) {
				this.gps_coords = await this.get_from_GPS();
				this.global_coords = this.gps_coords;
				await setGPSType('gps');

			} else {
				this.global_coords = this.manual_coords;
				await setGPSType('manual');
			}
		},
		async validate_manual_coords(value, which) {
			const number_value = Number(value);
			if (isNaN(number_value) || value == null) {
				this.is_valid_manual_coords[which] = false;
			}
			else {
				this.is_valid_manual_coords[which] = true;
				this.manual_coords[which] = number_value;
				this.manual_coords.timestamp = Date.now();
				await setManualGPSCoords(this.manual_coords);
			}
		},
		async send_telescope() {
			const api_path = await getAPIPath();
			await axios
				.post(api_path + '/gps', {
					latitude: this.global_coords.latitude,
					longitude: this.global_coords.longitude,
					elevation: this.global_coords.elevation
			});
		}
	}		
});
</script>

<style scoped>
ion-button {
	margin: 20px;
}
</style>
