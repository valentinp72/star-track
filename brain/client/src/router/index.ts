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
				props: { replaceUrl: true }
			},
			{
				path: 'objects',
				component: () => import('@/views/TabObjects.vue'),
				props: { replaceUrl: true }
			},
			{
				path: 'objects/:id',
				component: () => import('@/views/ObjectDetail.vue'),
				props: { replaceUrl: true }
			},
			{
				path: 'settings',
				component: () => import('@/views/TabSettings.vue'),
				props: { replaceUrl: true }
			},
		]
	}
]

const router = createRouter({
history: createWebHistory(process.env.BASE_URL),
routes
})

export default router
