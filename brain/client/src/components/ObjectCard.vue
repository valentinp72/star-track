<template>
	<ion-card>
		<img :src="`${image_url}`" height="150" width="1000" v-if="image_url !== undefined" />
		<ion-card-header>
			<ion-card-title>{{ name }}</ion-card-title>
			<ion-card-subtitle>{{ type }}</ion-card-subtitle>
		</ion-card-header>
		<ion-card-content>
			{{ description }}
		</ion-card-content>
	</ion-card>
</template>

<script setup lang="ts">
import { IonCard, IonCardContent, IonCardHeader, IonCardSubtitle, IonCardTitle } from '@ionic/vue';
defineProps({
	name: String,
	type: String,
	description: String,
	image_url: String,
});
</script>

<style scoped>
/* iOS places the subtitle above the title */
ion-card-header {
	display: flex;
	flex-flow: column-reverse;
}
ion-card-subtitle {
	text-transform: uppercase;
}
img {
	object-fit: cover;
}
</style>

