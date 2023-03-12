<template>
  <meta name="theme-color" media="(prefers-color-scheme: light)" content="red" />
  <meta name="theme-color" media="(prefers-color-scheme: dark)" content="red" />
  <ion-app>
    <ion-router-outlet />
  </ion-app>
</template>

<script setup lang="ts">
import { IonApp, IonRouterOutlet } from '@ionic/vue';
</script>
