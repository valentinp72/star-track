<template>
	<router-link
		:to="'/tabs/objects/' + this.id" router-direction="back"
		class="router-link"
	>
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
	</router-link>
</template>

<script lang="ts">
import { IonCard, IonCardContent, IonCardHeader, IonCardSubtitle, IonCardTitle } from '@ionic/vue';
import { defineComponent } from 'vue';

export default defineComponent({
	name: 'ObjectCard',
	components: {
		IonCard, IonCardContent, IonCardHeader, IonCardSubtitle, IonCardTitle
	},
	props: {
		id: Number,
		name: String,
		type: String,
		description: String,
		image_url: String,
	}
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
.router-link {
	text-decoration: none;
	color: inherit;
}
</style>

