const fs = require('fs')

let server = {
	type: 'http'
}

if (fs.existsSync('../ssl/certificate.key') && fs.existsSync('../ssl/certificate.crt')) {
	server = {
		type: 'https',
		options: {
			key: fs.readFileSync('../ssl/certificate.key').toString(),
			cert: fs.readFileSync('../ssl/certificate.crt').toString()
		}
	};
}

module.exports = {
	pwa: {
		name: 'StarTrack',
		themeColor: '',
		msTileColor: '',
		appleMobileWebAppCapable: 'yes',
		appleMobileWebAppStatusBarStyle: 'black',
		iconPaths: {
			faviconSVG: null,
			favicon32: null,
			favicon16: null,
			appleTouchIcon: null,
			maskIcon: null,
			msTileImage: null
		}
	},
    devServer: {
		allowedHosts: 'all',
		server: server,
		proxy: {
			"/api": {
				"target": 'https://telescopi:5090',
				"pathRewrite": { '^/api': '' },
				"changeOrigin": true,
				"secure": true
			}
		},
	}
}

