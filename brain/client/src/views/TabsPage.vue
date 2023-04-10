<template>
	<ion-page>
		<ion-tabs>
			<ion-router-outlet></ion-router-outlet>
			<ion-tab-bar slot="bottom">
				<ion-tab-button tab="telescope" href="/tabs/telescope">
					<ion-icon aria-hidden="true" :icon="telescopeSharp" />
					<ion-label>Telescope</ion-label>
				</ion-tab-button>

				<ion-tab-button tab="objects" href="/tabs/objects">
					<ion-icon aria-hidden="true" :icon="planetSharp" />
					<ion-label>Objects</ion-label>
				</ion-tab-button>

				<ion-tab-button tab="settings" href="/tabs/settings">
					<ion-icon aria-hidden="true" :icon="cogOutline" />
					<ion-label>Settings</ion-label>
				</ion-tab-button>
			</ion-tab-bar>
		</ion-tabs>
	</ion-page>
</template>

<script lang="ts">
import { IonTabBar, IonTabButton, IonTabs, IonLabel, IonIcon, IonPage, IonRouterOutlet, toastController } from '@ionic/vue';
import { planetSharp, telescopeSharp, cogOutline } from 'ionicons/icons';
import { defineComponent } from 'vue';

import axios from 'axios';
import { getAPIPath } from '@/plugins/config';

export default defineComponent({
	name: 'TabsPage',
	components: {
		IonTabBar, IonTabButton, IonTabs, IonLabel, IonIcon, IonPage, IonRouterOutlet
	},
	setup() {
		return { planetSharp, telescopeSharp, cogOutline }
	},
	data() {
		return {
			isAlive: true,
			toast: null as any
		}
	},
	methods: {
		async isBackendAlive() {
			try {
				const api_path = await getAPIPath();
				const res = await axios.get(api_path + '/ping', { timeout: 5000 });
				if (res.data.message == "pong") {
					return true
				}
				return false
			} catch (error) {
				return false
			}
		},
		async showErrorToast() {
			this.toast = await toastController.create({
				message: 'Cannot connect to the telescope API. Please fix this is Settings > Telescope.',
				position: 'bottom',
				color: 'danger',
				buttons: [{
					text: "Dismiss",
					role: "cancel"
				}],
				cssClass: 'tabs-bottom'
			});
			await this.toast.present();
		},
		checkWorkingBackend() {
			setInterval(async () => {
				const isAlive = await this.isBackendAlive();
				if (this.isAlive == true && isAlive == false) {
					// was working before, now not working: showing toast
					await this.showErrorToast();
				} else if (isAlive == true && this.isAlive == false) {
					// was not working, and now is working: dismissing toast
					if (this.toast) {
						await this.toast.dismiss();
					}
				}
				this.isAlive = isAlive;
			}, 15 * 1000); // every 15 seconds
		}
	},
	async mounted() {
		const isAlive = await this.isBackendAlive();
		if (isAlive == false) {
			await this.showErrorToast();
		}
		this.isAlive = isAlive;
		this.checkWorkingBackend();
	}
});
</script>

<style>
/* from https://github.com/ionic-team/ionic-framework/issues/17499#issuecomment-473769869 */
.md .tabs-bottom {
	transform: translateY(-56px) !important;
}
.ios .tabs-bottom {
	transform: translateY(-50px) !important;
}
.md .header-top {
	top: 56px !important;
}
.ios .header-top {
	top: 44px !important;
}
</style>
