import { mount } from 'svelte';
import './app.css';
import App from './App.svelte';

// iOS ignores user-scalable=no in some cases: block pinch zoom.
document.addEventListener('gesturestart', (e) => e.preventDefault());

mount(App, { target: document.getElementById('app') });
