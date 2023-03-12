<template>
  <ion-page>
    <ion-header :translucent="true">
      <ion-toolbar>
        <ion-title>Telescope</ion-title>
      </ion-toolbar>
    </ion-header>
    <ion-content :fullscreen="true">
      <ion-header collapse="condense">
        <ion-toolbar>
          <ion-title size="large">Tab 1</ion-title>
        </ion-toolbar>
      </ion-header>
      <ExploreContainer name="Tab 1 page" />
		<ion-list>
			<ion-item>
				<ion-label>Latitude: {{ coords.latitude }}</ion-label>
			</ion-item>
			<ion-item>
				<ion-label>Longitude : {{ coords.longitude }}</ion-label>
			</ion-item>
			<ion-item>
				<ion-label>Accuracy: {{ coords.accuracy }}</ion-label>
			</ion-item>
			<ion-item>
				<ion-label>Timestamp: {{ coords.timestamp }}</ion-label>
			</ion-item>
		</ion-list>
    </ion-content>
  </ion-page>
</template>

<script lang="ts">
import { IonPage, IonHeader, IonToolbar, IonTitle, IonContent } from '@ionic/vue';
import { IonItem, IonLabel, IonList } from '@ionic/vue';
import ExploreContainer from '@/components/ExploreContainer.vue';
import { Geolocation } from '@capacitor/geolocation';
import { defineComponent, ref, onMounted } from 'vue';
export default defineComponent({
	components: { IonPage, IonHeader, IonContent, IonToolbar, IonTitle, ExploreContainer, IonItem, IonList, IonLabel },
	setup() {
		const coords = ref({latitude: 0, longitude: 0, accuracy: 0, timestamp: 0});
		onMounted(async () => {
			const response = await Geolocation.getCurrentPosition();
			if (response) {
				coords.value = {
					latitude: response.coords.latitude,
					longitude: response.coords.longitude,
					accuracy: response.coords.accuracy,
					timestamp: response.timestamp,
				};
			}
		});
		return { coords };
	}
});
</script>
