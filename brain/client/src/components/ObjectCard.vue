<template>
	<ion-card>
		<img src="" height="150" width="1000" />
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
defineProps({
	name: String,
	type: String,
	description: String,
	image_url: String,
});
</script>

<style scoped>
/* iOS places the subtitle above the title */
ion-card-header.ios {
	display: flex;
	flex-flow: column-reverse;
}
img {
	object-fit: cover;
}
</style>

