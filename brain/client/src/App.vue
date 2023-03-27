<template>
  <ion-app v-disable-swipe-back>
    <ion-router-outlet swipeGesture="false" animated="false" />
  </ion-app>
</template>

<script setup lang="ts">
import { IonApp, IonRouterOutlet } from '@ionic/vue';
</script>
