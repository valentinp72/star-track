<template>
	<ion-page>
		<ion-header >
			<ion-toolbar>
				<ion-title>Objects</ion-title>
			</ion-toolbar>
		</ion-header>
		<ion-content :fullscreen="true">
		<ion-header collapse="condense">
			<ion-toolbar>
				<ion-title size="large">Objects</ion-title>
			</ion-toolbar>
		</ion-header>
		<ObjectCard
			v-for="(object, index) in objects"
			:name="object.object_name"
			:type="object.additional_info.type"
			:description="object.additional_info.description"
			:image_url="object.additional_info.image"
			:index="index"
			:key="object.id"
		/>
		</ion-content>
	</ion-page>
</template>

<script lang="ts">
import { IonPage, IonHeader, IonToolbar, IonTitle, IonContent } from '@ionic/vue';
import { defineComponent } from 'vue';
import type {AxiosInstance} from 'axios'
/* import axios from "axios"; */
import ObjectCard from '@/components/ObjectCard.vue';

declare module '@vue/runtime-core' {
	interface ComponentCustomProperties {
		$axios: AxiosInstance
	}
}

export default defineComponent({
	name: 'TabObjects',
	components: {
		IonPage,
		IonHeader,
		IonToolbar,
		IonTitle,
		IonContent,
		ObjectCard
	},
	data() {
		return {
			objects: null
		}
	},
	beforeMount() {
		this.$axios
			.get('/planets')
			.then(reponse => {
				this.objects = reponse.data
			})
	}
});
</script>
