import { createRouter, createWebHistory } from '@ionic/vue-router';
import { RouteRecordRaw } from 'vue-router';
import TabsPage from '../views/TabsPage.vue'

const routes: Array<RouteRecordRaw> = [
  {
    path: '/',
    redirect: '/tabs/telescope'
  },
  {
    path: '/tabs/',
    component: TabsPage,
    children: [
      {
        path: '',
        redirect: '/tabs/telescope'
      },
      {
        path: 'telescope',
        component: () => import('@/views/TabTelescope.vue')
      },
      {
        path: 'objects',
        component: () => import('@/views/TabObjects.vue'),
		children: [
			{
				path: 'Earth',
				component: () => import('@/views/TabTelescope.vue')
			}
		]
      },
      {
        path: 'settings',
        component: () => import('@/views/TabSettings.vue')
      }
    ]
  }
]

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes
})

export default router
