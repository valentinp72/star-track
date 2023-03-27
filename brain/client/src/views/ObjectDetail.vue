<template>
	<ion-page>
		<ion-header>
			<ion-toolbar>
				<ion-buttons slot="start">
					<ion-back-button></ion-back-button>
				</ion-buttons>
				<ion-title>Details</ion-title>
			</ion-toolbar>
		</ion-header>

		<ion-content> Detail ID: {{ id }} </ion-content>
	</ion-page>
</template>

<script lang="ts">
import { IonContent, IonHeader, IonPage, IonTitle, IonToolbar, IonBackButton, IonButtons } from '@ionic/vue';
import { defineComponent } from 'vue';
import { useRoute } from 'vue-router';

export default defineComponent({
	name: 'ObjectDetail',
	components: {
		IonContent,
		IonHeader,
		IonPage,
		IonTitle,
		IonToolbar,
		IonBackButton,
		IonButtons
	},
	setup() {
		const route = useRoute();
		const { id } = route.params;
		return { id };
	},
});
</script>
