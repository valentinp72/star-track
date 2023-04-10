<template>
	<ion-page>
		<ion-header>
			<ion-toolbar>
				<ion-title>Settings</ion-title>
			</ion-toolbar>
		</ion-header>
		<ion-content :fullscreen="true">
			<ion-header collapse="condense">
				<ion-toolbar>
					<ion-title size="large">Settings</ion-title>
				</ion-toolbar>
			</ion-header>
			<ion-list :inset="true">
				<ion-item detail="true" router-link="/tabs/settings/telescope" >
					<ion-label>Telescope</ion-label>
				</ion-item>
				<ion-item detail="true" router-link="/tabs/settings/gps">
					<ion-label>GPS location</ion-label>
				</ion-item>
				<ion-item detail="true" router-link="/tabs/settings/security">
					<ion-label>Security</ion-label>
				</ion-item>
			</ion-list>
		</ion-content>
	</ion-page>
</template>

<script lang="ts">
import { IonPage, IonHeader, IonToolbar, IonTitle, IonList, IonItem, IonLabel, IonContent } from '@ionic/vue';
import { defineComponent } from 'vue';

export default defineComponent({
	components: { IonPage, IonHeader, IonToolbar, IonTitle, IonList, IonItem, IonLabel, IonContent },
	setup() {
		return { }
	}
});
</script>
