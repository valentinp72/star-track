import { createApp } from 'vue'
import App from './App.vue'
import router from './router';
import axios from './plugins/axios'

import { IonicVue } from '@ionic/vue';

/* To disable swipe gesture for tabs */
import { DisableSwipeBackDirective } from 'v-disable-swipe-back';

/* Core CSS required for Ionic components to work properly */
import '@ionic/vue/css/core.css';

/* Basic CSS for apps built with Ionic */
import '@ionic/vue/css/normalize.css';
import '@ionic/vue/css/structure.css';
import '@ionic/vue/css/typography.css';

/* Optional CSS utils that can be commented out */
import '@ionic/vue/css/padding.css';
import '@ionic/vue/css/float-elements.css';
import '@ionic/vue/css/text-alignment.css';
import '@ionic/vue/css/text-transformation.css';
import '@ionic/vue/css/flex-utils.css';
import '@ionic/vue/css/display.css';

/* Theme variables */
import './theme/variables.css';

// Added by the CLI
import './registerServiceWorker';

const app = createApp(App)
	.directive('disable-swipe-back', DisableSwipeBackDirective)
	.use(IonicVue, {
		rippleEffect: false,
		swipeBackEnabled: false,
		mode: 'ios'
	})
	.use(router)
	.use(axios, {
		baseUrl: '/api',
	});
  
router.isReady().then(() => {
  app.mount('#app');
});
