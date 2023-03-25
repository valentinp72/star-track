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
		name="Saturn"
		type="Planet of the Solar System"
		description="Saturn is the sixth planet from the Sun and the second-largest in the Solar System, after Jupiter. It is a gas giant with an average radius of about nine and a half times that of Earth."
		image_url="https://malak:5090/static/images/saturn.jpg"
	/>
  </ion-content>
  </ion-page>
</template>

<script lang="ts">
import { IonPage, IonHeader, IonToolbar, IonTitle, IonContent } from '@ionic/vue';
import { defineComponent } from 'vue';
import ObjectCard from '@/components/ObjectCard.vue';

export default defineComponent({
	components: { IonPage, IonHeader, IonToolbar, IonTitle, IonContent, ObjectCard },
});
</script>
