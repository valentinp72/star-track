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
			:image_url="getImagePath(object)"
			:id="object.code"
			:index="index"
			:key="object.id"
		/>
		</ion-content>
	</ion-page>
</template>

<script lang="ts">
import { IonPage, IonHeader, IonToolbar, IonTitle, IonContent } from '@ionic/vue';
import { defineComponent } from 'vue';
import axios from 'axios';
import { getAPIPath } from '@/plugins/config';
import ObjectCard from '@/components/ObjectCard.vue';

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
			objects: [],
			api_path: null as any
		}
	},
	async beforeMount() {
		getAPIPath().then(api_path => {
			this.api_path = api_path;
			axios
				.get(this.api_path + '/planets')
				.then(reponse => {
					this.objects = reponse.data
				})
		});
	},
	methods: {
		getImagePath(object) {
			if (object.additional_info.image == undefined) {
				return undefined
			}
			else {
				return this.api_path + object.additional_info.image
			}
		}	
	}
});
</script>
