<template>
	<ion-page>
		<ion-header>
			<ion-toolbar>
				<ion-buttons slot="start">
					<ion-back-button></ion-back-button>
				</ion-buttons>
				<ion-title>Security</ion-title>
			</ion-toolbar>
		</ion-header>
		<ion-content>
			<ion-list>
				<ion-list-header>
					<ion-label>SSL Root certificate</ion-label>
				</ion-list-header>
				<ion-button download="Root_SSL_Certificate.crt" :href="download_SSL()" expand="block" color="medium">
					<ion-icon slot="start" :icon="downloadOutline"></ion-icon>
					Download the root certificate
				</ion-button>
			</ion-list>

			<ion-list>
				<ion-list-header>
					<ion-label>Solar protection</ion-label>
				</ion-list-header>
				<ion-item lines="none">
					<ion-toggle :checked="allow_sun" @ionChange="toggle_solar($event.target.checked)">
						Allow sun
					</ion-toggle>
				</ion-item>
				<ion-item>
					<ion-note>
						Only toggle this option if you have a solar filter 
						installed, or, if you it is nighttime and the sun 
						cannot be seen.
					</ion-note>
				</ion-item>
			</ion-list>
		</ion-content>
	</ion-page>
</template>

<script lang="ts">
import { IonContent, IonHeader, IonPage, IonTitle, IonToolbar, IonBackButton, IonButtons, IonButton, IonList, IonItem, IonLabel, IonListHeader, IonIcon, IonToggle, IonNote } from '@ionic/vue';
import { defineComponent } from 'vue';
import { getAPIPath } from '@/plugins/config';
import { downloadOutline } from 'ionicons/icons';
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
		IonButtons, IonButton,
		IonToggle,
		IonNote,
		IonList, IonItem, IonLabel, IonListHeader, IonIcon,
	},
	data() {
		return {
			api_path: "",
			allow_sun: false
		}
	},

	setup() {
		return { downloadOutline };
	},
	async beforeMount() {
		this.api_path = await getAPIPath();
		this.allow_sun = await axios.get(this.api_path + '/sun_allowed', { timeout: 5000 });
	},
	methods: {
		download_SSL() {
			return this.api_path + '/static/ssl_certificate.crt';
		},
		async toggle_solar(toggled) {
			this.allow_sun = toggled;
			await axios
				.post(this.api_path + '/sun_allowed', {
					toggle: toggled,
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
