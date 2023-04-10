<template>
	<ion-page>
		<ion-header>
			<ion-toolbar>
				<ion-buttons slot="start">
					<ion-back-button></ion-back-button>
				</ion-buttons>
				<ion-title>Telescope settings</ion-title>
			</ion-toolbar>
		</ion-header>
		<ion-content>

			<ion-list>
				<ion-list-header>
					<ion-label>API access</ion-label>
				</ion-list-header>
				<ion-item>
					<ion-input
						label="URL"
						label-placement="floating"
						placeholder="https://path-to/api"
						:value="APIPath"
						@ionChange="validateAPIPath"
						@ionBlur="validateAPIPath"
					></ion-input>
					<ion-badge slot="end" :color="apiPathColorCode">{{ apiPathShort }}</ion-badge>
				</ion-item>
				<ion-item @click="checkAPIConnection">
					<ion-label>
						Check connection
						<p>Status: {{ apiStatusMessage }}</p>
					</ion-label>
					<ion-badge slot="end" :color="apiStatusColorCode">{{ apiStatusShort }}</ion-badge>
				</ion-item>
			</ion-list>

		</ion-content>
	</ion-page>
</template>

<script lang="ts">
import { IonContent, IonHeader, IonPage, IonTitle, IonToolbar, IonBackButton, IonButtons, IonList, IonItem, IonLabel, IonListHeader, IonInput, IonBadge } from '@ionic/vue';
import { defineComponent } from 'vue';
import axios from 'axios';
import { getAPIPath, setAPIPath } from '@/plugins/config';

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
		IonList, IonItem, IonLabel, IonListHeader,
		IonInput,
		IonBadge
	},
	data() {
		return {
			isValidRestPath: true,
			isAPIOnline: null as any,
			APIPath: null as any,
			apiPathColorCode: "danger",
			apiPathShort: "",
			apiStatusColorCode: "warning",
			apiStatusMessage: "Not checked (click to check)",
			apiStatusShort: ""
		}
	},
	async beforeMount() {
		this.APIPath = await getAPIPath();
	},
	methods: {
		validateURL(url) {
			// eslint-disable-next-line
			return url.match(/^https?:\/\/\w+(\.\w+)*(:[0-9]+)?\/?(\/[.\w]*)*$/);
		},
		async validateAPIPath(ev) {
			const url = ev.target.value;

			if (this.validateURL(url)) {
				await setAPIPath(url);
				this.APIPath = url;
				this.isValidRestPath = true;
				// reloading so all components will reload with the correct URL
				window.location.reload()
			}
			else {
				this.isValidRestPath = false;
				this.apiPathShort = "Invalid URL";
			}
		},
		async checkAPIConnection() {
			if (!this.isValidRestPath) return;
			this.isAPIOnline = false;
			this.apiStatusColorCode = "medium";
			this.apiStatusMessage = "Checking..."
			this.apiStatusShort = ""
			axios
				.get(this.APIPath + '/ping', { timeout: 5000 })
				.then(reponse => {
					if (reponse.data.message == "pong") {
						this.isAPIOnline = true;
						this.apiStatusColorCode = "success";
						this.apiStatusShort = "Online"
					} else {
						this.isAPIOnline = false;
						this.apiStatusColorCode = "danger";
						this.apiStatusShort = "Offline";
					}
					this.apiStatusMessage = "received `" + reponse.data.message + "` from API"
				})
				.catch(error => {
					this.isAPIOnline = false ;
					this.apiStatusColorCode = "danger";
					this.apiStatusShort = "Offline";
					this.apiStatusMessage = error.message;
				});
		}
	}		
});
</script>

