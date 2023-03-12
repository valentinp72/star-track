<template>
  <ion-page v-disable-swipe-back>
    <ion-tabs>
      <ion-router-outlet></ion-router-outlet>
      <ion-tab-bar slot="bottom">
        <ion-tab-button tab="telescope" href="/tabs/telescope">
          <ion-icon aria-hidden="true" :icon="telescopeSharp" />
          <ion-label>Telescope</ion-label>
        </ion-tab-button>

        <ion-tab-button tab="objects" href="/tabs/objects">
          <ion-icon aria-hidden="true" :icon="planetSharp" />
          <ion-label>Objects</ion-label>
        </ion-tab-button>

        <ion-tab-button tab="settings" href="/tabs/settings">
          <ion-icon aria-hidden="true" :icon="cogOutline" />
          <ion-label>Settings</ion-label>
        </ion-tab-button>
      </ion-tab-bar>
    </ion-tabs>
  </ion-page>
</template>

<script setup lang="ts">
import { IonTabBar, IonTabButton, IonTabs, IonLabel, IonIcon, IonPage, IonRouterOutlet } from '@ionic/vue';
import { planetSharp, telescopeSharp, cogOutline } from 'ionicons/icons';
</script>
