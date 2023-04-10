import { createRouter, createWebHistory } from '@ionic/vue-router';
import { RouteRecordRaw } from 'vue-router';
import TabsPage from '../views/TabsPage.vue'

const routes: Array<RouteRecordRaw> = [
	{
		path: '/',
		redirect: '/tabs/telescope',
	},
	{
		path: '/tabs/',
		component: TabsPage,
		children: [
			{
				path: '',
				redirect: 'telescope',
			},
			{
				path: 'telescope',
				component: () => import('@/views/TabTelescope.vue'),
			},
			{
				path: 'objects',
				component: () => import('@/views/TabObjects.vue'),
			},
			{
				path: 'objects/:id',
				component: () => import('@/views/objects/ObjectDetail.vue'),
			},
			{
				path: 'settings',
				component: () => import('@/views/TabSettings.vue'),
			},
			{
				path: 'settings/telescope',
				component: () => import('@/views/settings/TelescopeSettings.vue'),
			},
			{
				path: 'settings/gps',
				component: () => import('@/views/settings/GPSSettings.vue'),
			},
		]
	}
]

const router = createRouter({
history: createWebHistory(process.env.BASE_URL),
routes
})

export default router
